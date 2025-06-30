#include "sd_task.h"
#include "sdcard.h"
#include "esp_log.h"
#include "mcu_spi_config.h" //mutex_spi
#include "ads1256_task.h"

#define TAG "SD_TASK"
static sd_card_t sd_card;

esp_err_t sd_task_init(void) {

    sd_card_config_t config = {
        .spi_host = SDSPI_DEFAULT_HOST,
        .mount_point = MOUNT_POINT,
        .cs_pin = 21,
        .cd_pin = -1
    };

    if (!SD_init(&sd_card, &config)) {
        ESP_LOGE("SD_TASK", "Failed to initialize SD card");
        return ESP_FAIL;
    } else {
        ESP_LOGI("SD_TASK", "SD card initialized successfully");
    }

    return ESP_OK;

}


bool save_buffer_as_text(const char* path, uint8_t* buffer, size_t length) {
    if (xSemaphoreTake(mutex_spi, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE("SDCARD", "Failed to take SPI mutex");
        return false;
    }
    FILE* f = fopen(path, "a");  // tryb tekstowy
    if (!f) {
        ESP_LOGE("SDCARD", "Failed to open file %s for writing", path);
        return false;
    }

    for (size_t i = 0; i < length; i+= 3) {
        fprintf(f, "%d %d %d\n", buffer[i], buffer[i+1], buffer[i+2]);  // zapisz każdą wartość w osobnej linii
    }

    fclose(f);
    xSemaphoreGive(mutex_spi);
    ESP_LOGI("SDCARD", "Buffer saved as text to %s (%d values)", path, length);
    return true;
}


void save_ads1256_buffor_task(void *arg)
{
    ESP_LOGI(TAG, "Starting SD card save task");
    const char *test_filename = "/sdcard/dupa.txt";

    while (1)
    {
        if(!xSemaphoreTake(readc_A_mutex, portMAX_DELAY))
        {
            ESP_LOGE(TAG, "Failed to take readc_A_mutex");
            vTaskDelay(pdMS_TO_TICKS(1));
            continue;
        }
            ESP_LOGI(TAG, "readc_A_mutex taken successfully");

        if (sd_card.mounted) {
            ESP_LOGI(TAG, "SD card is mounted, saving data...");
            if (save_buffer_as_text(test_filename, buffer_readc_A, BUFFER_READC_SIZE)) {
                ESP_LOGI(TAG, "Data saved successfully to /sdcard/ads1256_data_A.bin");
            } else {
                ESP_LOGE(TAG, "Failed to save data to SD card");
            }
        } else {
            ESP_LOGW(TAG, "SD card is not mounted, skipping save operation");
        }

        xSemaphoreGive(readc_A_mutex);

        if(!xSemaphoreTake(readc_B_mutex, portMAX_DELAY))
        {
            ESP_LOGE(TAG, "Failed to take readc_B_mutex");
            vTaskDelay(pdMS_TO_TICKS(1));
            continue;
        }
        ESP_LOGI(TAG, "readc_B_mutex taken successfully");

        if (sd_card.mounted) {
            ESP_LOGI(TAG, "SD card is mounted, saving data...");
            if (save_buffer_as_text(test_filename, buffer_readc_B, BUFFER_READC_SIZE)) {
                ESP_LOGI(TAG, "Data saved successfully to /sdcard/ads1256_data_B.bin");
            } else {
                ESP_LOGE(TAG, "Failed to save data to SD card");
            }
        } else {
            ESP_LOGW(TAG, "SD card is not mounted, skipping save operation");
        }

        xSemaphoreGive(readc_B_mutex);

    }

}

void test(void *arg)
{
    const char *test_filename = "/sdcard/dupa.txt";
    const char *test_data = "Hello from ESP32 SD card! xddd";
    const uint8_t tst[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    size_t data_len = strlen(test_data);
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Opóźnienie dla stabilności
        if (SD_write(&sd_card, test_filename, (const char *)tst, 5)) {
            ESP_LOGI(TAG, "Data written successfully to %s", test_filename);
        } else {
            ESP_LOGE(TAG, "Failed to write data to %s", test_filename);
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // Opóźnienie dla stabilności
    }
}

void run_test_task(void) {
    ESP_LOGI("SD_TASK", "Starting SD card test task");
    xTaskCreate(save_ads1256_buffor_task, "test_task", 8192, NULL, 5, NULL);
}