#include "sd_task.h"
#include "sdcard.h"
#include "esp_log.h"
#include "mcu_spi_config.h" //mutex_spi
#include "ads1256_task.h"
#include "driver/sdmmc_host.h"
#include <dirent.h>

#define TAG "SD_TASK"
static sd_card_t sd_card;
TaskHandle_t sd_task = NULL;
TaskHandle_t weight_sd_task = NULL;
volatile bool new_filename_flag = false;


void get_next_log_filename(char *out_name, size_t max_len)
{
    DIR *dir = opendir(MOUNT_POINT);
    struct dirent *entry;
    int max_index = 0;

    if (dir == NULL) {
        printf("Failed to open dir\n");
        snprintf(out_name, max_len, "%s/log_001.txt", MOUNT_POINT);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            int index;
            if (sscanf(entry->d_name, "LOG_%03d.txt", &index) == 1) {
                if (index > max_index) {
                    max_index = index;
                }
            }
        }
    }
    closedir(dir);

    snprintf(out_name, max_len, "%s/log_%03d.txt", MOUNT_POINT, max_index + 1);
}

esp_err_t sd_task_init(void) {

    

    sd_card_config_t config = {
        .mount_point = MOUNT_POINT,
    };

    if (!SD_init(&sd_card, &config)) {
        ESP_LOGE("SD_TASK", "Failed to initialize SD card");
        return ESP_FAIL;
    } else {
        ESP_LOGI("SD_TASK", "SD card initialized successfully");
    }

    if(!run_readc_sd_task())
    {
        ESP_LOGE("SD_TASK", "Failed to start readc SD task");
        return ESP_FAIL;
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

bool save_header_as_text(const char* path, const char* header) {
    FILE* f = fopen(path, "a");  // append text
    if (!f) {
        ESP_LOGE("SDCARD", "Failed to open %s for writing", path);
        return false;
    }
    fprintf(f, "Time,Channel1,Channel2,Channel3,Channel4");
    fclose(f);
    return true;
}

bool save_weight_as_text(const char* path, float weight[], int number_of_channels, uint32_t time_ms) {
    FILE* f = fopen(path, "a");  // append text
    if (!f) {
        ESP_LOGE("SDCARD", "Failed to open %s for writing", path);
        return false;
    }
    fprintf(f, "%lu", time_ms);
    int channel;
    for(channel = 0; channel < number_of_channels; channel++) {
        fprintf(f, ",%f", weight[channel]);
    }
    
    fclose(f);

    ESP_LOGI("SDCARD", "Weight saved as text to %s: %f", path, weight);
    return true;
}


void save_buffer(const char* path, readc_frame_t *buffer, size_t length) {
    ESP_LOGI(TAG, "Saving buffer to 4%s", path);
    if (sd_card.mounted) {
        ESP_LOGI(TAG, "SD card is mounted, saving data...");
        if (save_buffer_as_binary(path, buffer, length)) {
            ESP_LOGI(TAG, "Data saved successfully to %s", path);
        } else {
            ESP_LOGE(TAG, "Failed to save data to SD card");
        }
    } else {
        ESP_LOGW(TAG, "SD card is not mounted, skipping save operation");
    }
}

void save_weight_task(void *arg)
{
    char file_path[64];
     get_next_log_filename(file_path, sizeof(file_path));
     ESP_LOGI(TAG, "Saving weight data to %s", file_path);

     save_header_as_text(file_path, "Weight Data\n");
    int64_t timer_start = esp_timer_get_time();
     while (1)
     {
        ads1256_data_t data;
        if(ads1256_get_data_struct_copy(ADS1256_DEVICE_1, &data))
        {       
            timer_current = esp_timer_get_time();     
            save_weight_as_text(file_path, data.weight, 4, (uint32_t)((timer_current - timer_start)/1000));
        } 
    vTaskDelay(pdMS_TO_TICKS(1000));    

     }
}  

void run_weight_sd_task()
{
    ESP_LOGI("SD_TASK", "Starting SD card weight save task");
    BaseType_t result = xTaskCreate(
        save_weight_task,
        "save_weight_task",
        4096,
        NULL,
        5,
        &weight_sd_task
    );

    if (result != pdPASS) {
        ESP_LOGE("SD_TASK", "Failed to create save_weight_task");
    }
}

void delete_weight_sd_task()
{
    if (weight_sd_task != NULL) {
        vTaskDelete(weight_sd_task);
        weight_sd_task = NULL;
        ESP_LOGI("SD_TASK", "Weight save task deleted successfully");
    } else {
        ESP_LOGW("SD_TASK", "Weight save task is not running");
    }
}

void save_ads1256_buffor_task(void *arg)
{
    char file_path[64];
    get_next_log_filename(file_path, sizeof(file_path));
    ESP_LOGI(TAG, "Saving ADS1256 buffer to %s", file_path);
    
    ESP_LOGI(TAG, "Starting SD card save task");

    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if(new_filename_flag)
        {
            new_filename_flag = false;
            get_next_log_filename(file_path, sizeof(file_path));
            ESP_LOGI(TAG, "New filename set: %s", file_path);
        }
        
        if (xSemaphoreTake(buffer_A_ready, pdMS_TO_TICKS(0)) == pdTRUE)
        {
            if(!xSemaphoreTake(readc_A_mutex, portMAX_DELAY))
            {
                ESP_LOGE(TAG, "Failed to take readc_A_mutex");
                continue;
            }
            ESP_LOGI(TAG, "Saving buffer A to SD card");
            save_buffer(file_path, buffer_readc_A, sizeof(readc_frame_t) * BUFFER_READC_SAMPLES);
            xSemaphoreGive(readc_A_mutex);
        }
        else if (xSemaphoreTake(buffer_B_ready, pdMS_TO_TICKS(0)) == pdTRUE)
        {
            if(!xSemaphoreTake(readc_B_mutex, portMAX_DELAY))
            {
                ESP_LOGE(TAG, "Failed to take readc_B_mutex");
                continue;
            }
            ESP_LOGI(TAG, "Saving buffer B to SD card");
            save_buffer(file_path, buffer_readc_B, sizeof(readc_frame_t) * BUFFER_READC_SAMPLES);
            xSemaphoreGive(readc_B_mutex);
        }
        else
        {
            ESP_LOGE(TAG, "No buffers ready to save");
        }

    }


    ESP_LOGI(TAG, "Stopping SD card save task");

    vTaskDelete(NULL);


}

bool run_readc_sd_task()
{
    delete_weight_sd_task(); 

    ESP_LOGI("SD_TASK", "Starting SD card readc task");
    BaseType_t result = xTaskCreate(
        save_ads1256_buffor_task,
        "save_ads1256_buffor_task",
        8192,
        NULL,
        5,
        &sd_task
    );

    if (result != pdPASS) {
        ESP_LOGE("SD_TASK", "Failed to create save_ads1256_buffor_task");
        return false;
    }

    return true;
}