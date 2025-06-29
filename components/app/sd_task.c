#include "sd_task.h"
#include "sdcard.h"

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

void test(void *arg)
{
    const char *test_filename = "/sdcard/dupa.txt";
    const char *test_data = "Hello from ESP32 SD card! xddd";
    size_t data_len = strlen(test_data);
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Opóźnienie dla stabilności
        if (SD_write(&sd_card, test_filename, test_data, data_len)) {
            ESP_LOGI(TAG, "Data written successfully to %s", test_filename);
        } else {
            ESP_LOGE(TAG, "Failed to write data to %s", test_filename);
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // Opóźnienie dla stabilności
    }
}

void run_test_task(void) {
    ESP_LOGI("SD_TASK", "Starting SD card test task");
    xTaskCreatePinnedToCore(test, "test_task", 8192, NULL, 5, NULL, tskNO_AFFINITY);
}