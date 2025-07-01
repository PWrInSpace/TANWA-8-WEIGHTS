#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"

#include "board_config.h"
#include "setup_task.h"
#include "ads1256.h"
#include "mcu_gpio_config.h"
#include "sd_task.h"
#include "ads1256_task.h"
#define TAG "APP"

extern board_config_t config;

void app_main(void) {
    

    ESP_LOGI(TAG, "%s TANWA board starting", config.board_name);
    sd_task_init();
    // vTaskDelay(pdMS_TO_TICKS(5000)); // Delay to ensure SD card is ready
    void* task_handle = NULL;
    if(setup_task_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize setup task");
        return;
    }
    vTaskDelay(pdMS_TO_TICKS(3000)); // Delay to ensure SD card is ready
    ads1256_start_readc(ADS1256_DEVICE_1);
    vTaskDelay(pdMS_TO_TICKS(3000)); // Delay to ensure SD card is ready
    save_ads1256_buffor_task(task_handle);

    // vTaskDelay(pdMS_TO_TICKS(5000));
    // ads1256_start_readc(ADS1256_DEVICE_1);
    // ads1256_read_id(ADS1256_DEVICE_2);
    // ads1256_start_channel_task(ADS1256_DEVICE_1);
    // run_test_task();
}


// #include <stdio.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"

// #include "esp_log.h"
// #include "driver/sdmmc_host.h"
// #include "driver/sdmmc_defs.h"
// #include "esp_vfs_fat.h"
// #include "sdmmc_cmd.h"


// static const char *TAG = "SDMMC_APP";

// void app_main(void)
// {
//     esp_err_t ret;

//     sdmmc_host_t host = SDMMC_HOST_DEFAULT();

//     // KONFIGURACJA PINÓW TWOICH
//     sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
//     slot_config.width = 4;
//     slot_config.clk = 13;  // twój CLK
//     slot_config.cmd = 2;   // twój CMD
//     slot_config.d0  = 12;  // twój D0
//     slot_config.d1  = 5;   // twój D1
//     slot_config.d2  = 14;  // twój D2
//     slot_config.d3  = 21;  // twój D3

//     // WŁĄCZENIE WEWNĘTRZNYCH PULLUP
//     slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

//     // Montowanie FAT
//     esp_vfs_fat_sdmmc_mount_config_t mount_config = {
//         .format_if_mount_failed = false,
//         .max_files = 5,
//         .allocation_unit_size = 16 * 1024
//     };

//     sdmmc_card_t *card;
//     ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config,
//                                   &mount_config, &card);

//     if (ret != ESP_OK) {
//         ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
//         return;
//     }

//     // ESP_LOGI(TAG, "SD card mounted OK");

//     // // TEST ZAPISU
//     // FILE *f = fopen("/sdcard/test.txt", "w");
//     // if (f == NULL) {
//     //     ESP_LOGE(TAG, "Failed to open file for writing");
//     // } else {
//     //     fprintf(f, "Hello from ESP32-S3!\n");
//     //     fclose(f);
//     //     ESP_LOGI(TAG, "File written successfully");
//     // }

//     // // DEMONTUJ
//     // esp_vfs_fat_sdcard_unmount("/sdcard", card);
//     // ESP_LOGI(TAG, "SD card unmounted");
// }
