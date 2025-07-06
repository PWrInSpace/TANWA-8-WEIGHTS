#include "sd_task.h"
#include "sdcard.h"
#include "esp_log.h"
#include "mcu_spi_config.h" //mutex_spi
#include "ads1256_task.h"
#include "driver/sdmmc_host.h"

#define TAG "SD_TASK"
static sd_card_t sd_card;

esp_err_t sd_task_init(void) {

    sd_card_config_t config = {
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
bool empty_file(const char* path) {
    FILE* f = fopen(path, "wb");  // open for writing, truncate to zero length
    if (!f) {
        ESP_LOGE("SDCARD", "Failed to open %s for writing", path);
        return false;
    }
    fclose(f);
    ESP_LOGI("SDCARD", "File %s emptied successfully", path);
    return true;
}

bool print_file(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        ESP_LOGE("BIN", "Błąd otwierania pliku");
        return false;
    }
    
    uint8_t buf[64];
    size_t bytes_read;
    int index = 0;
    
    while ((bytes_read = fread(buf, 1, sizeof(buf), f)) > 0) {
        printf("sd_data: ");
        for (int i = 0; i < bytes_read; i++) {
            printf("%02X ", buf[i]);
        }
        printf("\n");
    }
    

    fclose(f);
    ESP_LOGI("SDCARD", "File %s printed successfully", path);
    return true;
}



bool save_buffer_as_binary(const char* path, readc_frame_t* buffer, size_t length) {
    FILE* f = fopen(path, "ab");  // append binary
    if (!f) {
        ESP_LOGE("SDCARD", "Failed to open %s for writing", path);
        return false;
    }

    size_t written = fwrite(buffer, 1, length, f);
    fclose(f);

    if (written != length) {
        ESP_LOGE("SDCARD", "Short write: %zu of %zu", written, length);
        return false;
    }

    ESP_LOGI("SDCARD", "Buffer saved as binary to %s (%d bytes)", path, length);
    return true;
}

void save_buffer(readc_frame_t *buffer, size_t length) {
    const char *test_filename = "/sdcard/dupa.txt";
    if (sd_card.mounted) {
        ESP_LOGI(TAG, "SD card is mounted, saving data...");
        if (save_buffer_as_binary(test_filename, buffer, length)) {
            ESP_LOGI(TAG, "Data saved successfully to %s", test_filename);
        } else {
            ESP_LOGE(TAG, "Failed to save data to SD card");
        }
    } else {
        ESP_LOGW(TAG, "SD card is not mounted, skipping save operation");
    }
}

void save_ads1256_buffor_task(void *arg)
{
    ESP_LOGI(TAG, "Starting SD card save task");

    while (!readc_stop_flag)
    {
        if (xSemaphoreTake(buffer_A_ready, portMAX_DELAY))
        {
            if(!xSemaphoreTake(readc_A_mutex, portMAX_DELAY))
            {
                ESP_LOGE(TAG, "Failed to take readc_A_mutex");
                vTaskDelay(pdMS_TO_TICKS(1));
                continue;
            }
                ESP_LOGI(TAG, "readc_A_mutex taken successfully");

            save_buffer(buffer_readc_A, BUFFER_READC_SAMPLES* sizeof(readc_frame_t));

            xSemaphoreGive(readc_A_mutex);
        }
        if (xSemaphoreTake(buffer_B_ready, portMAX_DELAY) == pdTRUE)
        {
            if(!xSemaphoreTake(readc_B_mutex, portMAX_DELAY))
            {
                ESP_LOGE(TAG, "Failed to take readc_B_mutex");
                vTaskDelay(pdMS_TO_TICKS(1));
                continue;
            }
            ESP_LOGI(TAG, "readc_B_mutex taken successfully");

            save_buffer(buffer_readc_B, BUFFER_READC_SAMPLES* sizeof(readc_frame_t));

            xSemaphoreGive(readc_B_mutex);
        }
    }

    if (xSemaphoreTake(buffer_A_ready, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        if(!xSemaphoreTake(readc_A_mutex, portMAX_DELAY))
        {
            ESP_LOGE(TAG, "Failed to take readc_A_mutex");
            vTaskDelay(pdMS_TO_TICKS(1));
            return;
        }
        ESP_LOGI(TAG, "readc_A_mutex taken successfully");

        save_buffer(buffer_readc_A, BUFFER_READC_SAMPLES* sizeof(readc_frame_t));


        xSemaphoreGive(readc_A_mutex);
    }

    if (xSemaphoreTake(buffer_B_ready, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        if(!xSemaphoreTake(readc_B_mutex, portMAX_DELAY))
        {
            ESP_LOGE(TAG, "Failed to take readc_B_mutex");
            vTaskDelay(pdMS_TO_TICKS(1));
            return;
        }
        ESP_LOGI(TAG, "readc_B_mutex taken successfully");

        save_buffer(buffer_readc_B, BUFFER_READC_SAMPLES* sizeof(readc_frame_t));

        xSemaphoreGive(readc_B_mutex);
    }

    ESP_LOGI(TAG, "SD card save task completed");
    vTaskDelete(NULL);

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

void run_readc_sd_task(void)
{
    ESP_LOGI("SD_TASK", "Starting SD card readc task");
    xTaskCreatePinnedToCore(save_ads1256_buffor_task, "save_ads1256_buffor_task", 8192, NULL, 5, NULL, 1);
}
void run_test_task(void) {
    ESP_LOGI("SD_TASK", "Starting SD card test task");
    xTaskCreate(save_ads1256_buffor_task, "test_task", 8192, NULL, 5, NULL);
}