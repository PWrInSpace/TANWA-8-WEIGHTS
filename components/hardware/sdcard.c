// Copyright 2022 PWrInSpace. Kuba

#include "sdcard.h"

#include <string.h>
#include <sys/stat.h>

#include "sd_test_io.h"

#include "esp_log.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#define TAG "SDCARD"

// const char* names[] = {"CLK ", "MOSI", "MISO", "CS  "};
// const int pins[] = {CONFIG_SPI_SCK,
//                     CONFIG_SPI_MOSI,
//                     CONFIG_SPI_MISO,
//                     CONFIG_SD_CS};
// const int pin_count = sizeof(pins)/sizeof(pins[0]);

// pin_configuration_t pin_config = {
//     .names = names,
//     .pins = pins,
// };

bool SD_init(sd_card_t *sd_card, sd_card_config_t *cfg) {
  // sd_card->spi_host = cfg->spi_host;
  // sd_card->card_detect_pin = cfg->cd_pin;
  sd_card->mount_point = cfg->mount_point;


  // Options for mounting the filesystem
  if (SD_mount(sd_card) == false) {
    return false;
  }

  return true;
}

bool SD_file_exists(const char *file_name) {
  struct stat st;
  if (stat(file_name, &st) == 0) {
    return true;
  }

  return false;
}

bool SD_mount(sd_card_t *sd_card) {

  esp_err_t res;

  sdmmc_host_t host = SDMMC_HOST_DEFAULT();

  // KONFIGURACJA PINÓW TWOICH
  sdmmc_slot_config_t slot_config = {
    .clk = 21,
    .cmd = 14,
    .d0  = 47,
    .d1  = 48,
    .d2  = 12,
    .d3  = 13,
    .cd  = -1,   // brak Card Detect
    .wp  = -1,   // brak Write Protect
    .width = 4,
};

  // WŁĄCZENIE WEWNĘTRZNYCH PULLUP
  slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

  // Montowanie FAT
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 5,
      .allocation_unit_size = 16 * 1024
  };

  // sdmmc_card_t *card;
  // res = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config,
  //                               &mount_config, &card);

  // if (res != ESP_OK) {
  //     ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(res));
  //     return false;
  // }
  res = esp_vfs_fat_sdmmc_mount(sd_card->mount_point, &host, &slot_config,
                                &mount_config, &sd_card->card);
  if (res != ESP_OK) {
    ESP_LOGE("SD_INIT", "esp_vfs_fat_sdmmc_mount failed: %s (0x%x)", esp_err_to_name(res), res);
    if (res == ESP_FAIL) {
      ESP_LOGE(TAG,
               "Failed to mount filesystem. "
               "If you want the card to be formatted, set the"
               "CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
    } else {
      ESP_LOGE(TAG,
               "Failed to initialize the card (%s). "
               "Make sure SD card lines have pull-up resistors in place.",
               esp_err_to_name(res));
              //  check_sd_card_pins(&pin_config, pin_count);
    }
    return false;
  }
  ESP_LOGI(TAG, "SD card mounted");
  sd_card->mounted = true;
  
  return true;
}

bool SD_unmount(sd_card_t *sd_card) {
  if (sd_card->mounted == false) {
    return true;
  }

  esp_err_t res;
  res = esp_vfs_fat_sdcard_unmount(sd_card->mount_point, sd_card->card);
  if (res != ESP_OK) {
    ESP_LOGE(TAG, "UNMOUNT ERROR\n");
    return false;
  }
  sd_card->mounted = false;
  return true;
}

bool SD_remount(sd_card_t *sd_card) {
  bool res;
  res = SD_unmount(sd_card);
  if (res == false) {
    return false;
  }

  res = SD_mount(sd_card);
  return res;
}

bool SD_write(sd_card_t *sd_card, const char *path, const char *data,
              size_t length) {
  esp_err_t res;

  if (sd_card->mounted == false) {
    if (SD_mount(sd_card) == false) {
      return false;
    }
  }

  res = sdmmc_get_status(sd_card->card);
  if (res != ESP_OK) {
    ESP_LOGE(TAG, "CARD ERROR, REMOOUNTING...");
    SD_remount(sd_card);
  }

  FILE *file = fopen(path, "a");
  if (file == NULL) {
    ESP_LOGE(TAG, "Base path %s", sd_card->mount_point);
    ESP_LOGE(TAG, "FILE OPEN ERROR %s", path);
    return false;
  }

  int written_bytes = 0;
  written_bytes = fprintf(file, data, sd_card->card->cid.name);
  fclose(file);

  if (written_bytes < 1) {
    ESP_LOGE(TAG, "UNABLE TO WRITE DATA TO SD CARD");
    return false;
  }

  return true;
}

bool SD_is_ok(sd_card_t *sd_card) {
  esp_err_t res = sdmmc_get_status(sd_card->card);
  if (res != ESP_OK) {
    ESP_LOGE(TAG, "SD error status %s", esp_err_to_name(res));
    return false;
  }

  return true;
}

bool SD_card_detect(sd_card_t *sd_card) {
  if (gpio_get_level(sd_card->card_detect_pin) == 0) {
    return true;
  }

  return false;
}

bool create_path_to_file(char *file_path, size_t size) {
  char *path = (char *)calloc(size, sizeof(char));
  int ret = 0;
  for (int i = 0; i < 1000; ++i) {
    ret = snprintf(path, size, "%s%d.txt", file_path, i);
    if (ret == size) {
      free(path);
      return false;
    }

    if (SD_file_exists(path) == false) {
      memcpy(file_path, path, size);
      free(path);
      return true;
    }
  }

  free(path);
  return false;
}
