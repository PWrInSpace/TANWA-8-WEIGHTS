#include "ads1256_wrapper.h"
#include "mcu_spi_config.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "ads1256_task.h"
#include "esp_timer.h"
#include "sd_task.h"

volatile bool readc_stop_flag = false;
volatile bool read_mux_stop_flag = false;
readc_frame_t* buffer_readc_A;
readc_frame_t* buffer_readc_B;
readc_frame_t* buffer_readc_current;

uint32_t buffer_readc_index = 0;

SemaphoreHandle_t readc_A_mutex = NULL;
SemaphoreHandle_t readc_B_mutex = NULL;
SemaphoreHandle_t buffer_A_ready = NULL;
SemaphoreHandle_t buffer_B_ready = NULL;
SemaphoreHandle_t current_mutex = NULL;
SemaphoreHandle_t current_sync = NULL;

typedef struct ads1256_task_args_t {
    ads1256_wrapper_t* w;
} ads1256_task_args_t;

bool ads1256_task_init(ads1256_wrapper_t* w)
{
    buffer_readc_A = (readc_frame_t*)heap_caps_malloc(BUFFER_READC_SAMPLES * sizeof(readc_frame_t), MALLOC_CAP_DMA);
    buffer_readc_B = (readc_frame_t*)heap_caps_malloc(BUFFER_READC_SAMPLES * sizeof(readc_frame_t), MALLOC_CAP_DMA);
    buffer_readc_current = buffer_readc_A;

    readc_A_mutex = xSemaphoreCreateMutex();
    readc_B_mutex = xSemaphoreCreateMutex();

    buffer_A_ready = xSemaphoreCreateBinary();
    buffer_B_ready = xSemaphoreCreateBinary();

    current_mutex = readc_A_mutex;
    current_sync = buffer_A_ready;

    if(buffer_readc_A == NULL || buffer_readc_B == NULL)
    {
        ESP_LOGE("ADS1256", "Failed to allocate memory for buffers");
        return false;
    }

    if (!ads1256_start_channel_task(w)) {
        ESP_LOGE("ADS1256", "Failed to start channel task");
        return false;
    }
    return true;
}

void ads1256_read_data_continuously(void*  pvParameters)
{

    ESP_LOGI("ADS1256", "Starting continuous read task for device 15");

    ads1256_task_args_t* args = (ads1256_task_args_t*)pvParameters;
    ads1256_wrapper_t* w = args->w;
    free(args);

    uint8_t dummy_data[3] = {0x00, 0x00, 0x00};
    int64_t start_time_us = esp_timer_get_time();
    buffer_readc_index = 0;
    buffer_readc_current = buffer_readc_A;
    current_mutex = readc_A_mutex;
    current_sync = buffer_A_ready;

    readc_stop_flag = false;

    if(!xSemaphoreTake(current_mutex, portMAX_DELAY))
    {
        ESP_LOGE("ADS1256", "Failed to take mutex");
        ads1256_wrapper_set_drdy_task(w, NULL);
        vTaskDelete(NULL);
        return;
    }

    while (!readc_stop_flag)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        uint8_t cs_pin = ads1256_get_pin_config(ads1256_wrapper_get_dev(w))->cs_gpio;
        if(!_ads1256_spi_transmit_queued(cs_pin, dummy_data, sizeof(dummy_data), buffer_readc_current[buffer_readc_index].data, 3))
        {
            ESP_LOGE("ADS1256", "Failed to read data from ADS1256");
        }
        buffer_readc_current[buffer_readc_index].time = (uint32_t)((esp_timer_get_time() - start_time_us)/1000);

        buffer_readc_index++;

        if(buffer_readc_index % 333 == 2)
        {
            ESP_LOGI("ADS1256", "Buffered value[%d]: %d", buffer_readc_index, buffer_readc_current[buffer_readc_index].data[2]);
        }
        if (buffer_readc_index >= BUFFER_READC_SAMPLES) {
            buffer_readc_index = 0;

            if (current_mutex == readc_A_mutex) {
                xSemaphoreGive(readc_A_mutex);
                xSemaphoreGive(buffer_A_ready);

                current_mutex = readc_B_mutex;
                current_sync = buffer_B_ready;
                buffer_readc_current = buffer_readc_B;
            } else {

                xSemaphoreGive(readc_B_mutex);
                xSemaphoreGive(buffer_B_ready);

                current_mutex = readc_A_mutex;
                current_sync = buffer_A_ready;
                buffer_readc_current = buffer_readc_A;
            }

            if (!xSemaphoreTake(current_mutex, portMAX_DELAY)) {
                ESP_LOGE("ADS1256","Failed to take mutex after switching buffers");
                ads1256_wrapper_set_drdy_task(w, NULL);
                vTaskDelete(NULL);
                return;
            }

            xTaskNotifyGive(sd_task);

            ESP_LOGI("ADS1256", "Switched to buffer %s", (current_mutex == readc_A_mutex) ? "A" : "B");
        }

    }

    xSemaphoreGive(current_mutex);

    new_filename_flag = true;
    ESP_LOGI("ADS1256", "Stopping continuous read task");

    ads1256_send_command(ads1256_wrapper_get_dev(w), SDATAC_COMMAND);
    ads1256_wrapper_set_drdy_task(w, NULL);
    vTaskDelete(NULL);

}


bool ads1256_start_readc(ads1256_wrapper_t* w)
{
    if (w == NULL) {
        return false;
    }

    if (ads1256_wrapper_get_drdy_task(w) != NULL) {
        if (!ads1256_stop_task_and_wait(w, pdMS_TO_TICKS(2000))) {
            ESP_LOGE("ADS1256", "Failed to stop existing task");
            return false;
        }
    }

    delete_weight_sd_task();

    ads1256_task_args_t* args = malloc(sizeof(*args));
    if (args == NULL) {
        ESP_LOGE("ADS1256", "Failed to allocate task arguments");
        return false;
    }

    args->w = w;

    ads1256_change_channel(w, 0);
    ads1256_send_command(ads1256_wrapper_get_dev(w), SYNC_COMMAND);
    ads1256_send_command(ads1256_wrapper_get_dev(w), WAKEUP_COMMAND);
    if (!ads1256_send_command(ads1256_wrapper_get_dev(w), RDATAC_COMMAND)) {
        ESP_LOGE("ADS1256", "Failed to enable continuous mode");
        free(args);
        return false;
    }

    TaskHandle_t task_handle = NULL;

    if (xTaskCreate(
            ads1256_read_data_continuously,
            "ads1256_task_readc",
            8192,
            args,
            10,
            &task_handle) != pdPASS) {
        ESP_LOGE("ADS1256", "Failed to create readc task");
        ads1256_send_command(
            ads1256_wrapper_get_dev(w), SDATAC_COMMAND);
        free(args);
        return false;
    }

    ads1256_wrapper_set_drdy_task(w, task_handle);
    return true;
}


void ads1256_data_from_channels(void*  pvParameters)
{
    ads1256_task_args_t* args = (ads1256_task_args_t*)pvParameters;
    ads1256_wrapper_t* w = args->w;
    free(args);

    ads1256_data_t data = {{0.0f, 0.0f, 0.0f, 0.0f}};
    ads1256_data_t sample_batch[ADS1256_UPDATE_DATA_AVG_SAMPLES];
    read_mux_stop_flag = false;
    uint8_t cur = 0;
    uint8_t batch_index = 0;

    ads1256_change_channel(w, 0);
    ads1256_send_command(ads1256_wrapper_get_dev(w), SYNC_COMMAND);
    ads1256_send_command(ads1256_wrapper_get_dev(w), WAKEUP_COMMAND);

    while (!read_mux_stop_flag)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        uint8_t raw_data[3] = {0, 0, 0};
        if (!ads1256_get_raw_data(ads1256_wrapper_get_dev(w), raw_data) ||
            !ads1256_raw_data_to_value(w, raw_data, &data.weight[cur], cur)) {
            ESP_LOGE("ADS1256", "Failed to read channel %d", cur);
        }

        cur = (cur + 1) % 4;
        if (cur == 0) {
            sample_batch[batch_index++] = data;
            if (batch_index >= ADS1256_UPDATE_DATA_AVG_SAMPLES) {
                ads1256_update_data_struct(w, sample_batch, ADS1256_UPDATE_DATA_AVG_SAMPLES);
                batch_index = 0;
            }
        }

        ads1256_change_channel(w, cur);
        ads1256_send_command(ads1256_wrapper_get_dev(w), SYNC_COMMAND);
        ads1256_send_command(ads1256_wrapper_get_dev(w), WAKEUP_COMMAND);
    }

    ads1256_wrapper_set_drdy_task(w, NULL);
    vTaskDelete(NULL);
}


bool ads1256_start_channel_task(ads1256_wrapper_t* w)
{
    if (w == NULL) {
        return false;
    }

    if (ads1256_wrapper_get_drdy_task(w) != NULL) {
        ESP_LOGE("ADS1256", "Task already running");
        return false;
    }

    ads1256_task_args_t* args = malloc(sizeof(*args));
    if (args == NULL) {
        ESP_LOGE("ADS1256", "Failed to allocate task arguments");
        return false;
    }

    args->w = w;

    TaskHandle_t task_handle = NULL;
    if(xTaskCreate(ads1256_data_from_channels, "ads1256_channel_task", 4096, args, 10, &task_handle) != pdPASS)
    {
        ESP_LOGE("ADS1256", "Failed to create ADS1256 channel task");
        free(args);
        return false;
    }

    ads1256_wrapper_set_drdy_task(w, task_handle);
    return true;
}

bool ads1256_stop_task_and_wait(ads1256_wrapper_t* w, TickType_t timeout)
{
    if (w == NULL) {
        ESP_LOGE("ADS1256", "ADS1256 wrapper is NULL");
        return false;
    }

    TaskHandle_t task_handle = ads1256_wrapper_get_drdy_task(w);

    if (task_handle == NULL) {
        return true;
    }

    read_mux_stop_flag = true;
    readc_stop_flag = true;

    xTaskNotifyGive(task_handle);

    TickType_t start = xTaskGetTickCount();
    while (ads1256_wrapper_get_drdy_task(w) != NULL) {
        vTaskDelay(pdMS_TO_TICKS(10));

        if ((xTaskGetTickCount() - start) >= timeout) {
            ESP_LOGE("ADS1256", "Task did not stop within timeout");
            return false;
        }

        task_handle = ads1256_wrapper_get_drdy_task(w);
        if (task_handle != NULL) {
            xTaskNotifyGive(task_handle);
        }
    }

    ESP_LOGI("ADS1256", "Task stopped gracefully");
    return true;
}

void ads1256_delete_task(ads1256_wrapper_t* w)
{
    if (w == NULL) {
        ESP_LOGE("ADS1256", "ADS1256 wrapper is NULL");
        return;
    }

    if (ads1256_wrapper_get_drdy_task(w) == NULL) {
        ESP_LOGW("ADS1256", "No task running");
        return;
    }

    if (!ads1256_stop_task_and_wait(w, pdMS_TO_TICKS(5000))) {
        ESP_LOGE("ADS1256", "CRITICAL: Task did not stop after 5s, "
                 "system may be in inconsistent state");
    }
}