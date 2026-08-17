#include "ads1256.h"
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
    ads1256_device_t device;
    TaskHandle_t* task_handle;
} ads1256_task_args_t;

bool ads1256_task_init(void)
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

    if (!ads1256_pins_init()) {
        ESP_LOGE("ADS1256", "Failed to initialize ADS1256 pins");
        return false;
    }

    if (!ads1256_init(ADS1256_DEVICE_1)) {
        ESP_LOGE("ADS1256", "Failed to initialize ADS1256 device 1");
        return false;
    }

    // if (!ads1256_init(ADS1256_DEVICE_2)) {
    //     ESP_LOGE("ADS1256", "Failed to initialize ADS1256 device 2");
    //     return false;
    // }

    ads1256_start_channel_task(ADS1256_DEVICE_1); //TODO: remove this line leater

    return true;
}

void ads1256_read_data_continuously(void*  pvParameters)
{

    ESP_LOGI("ADS1256", "Starting continuous read task for device 15");

    ads1256_task_args_t* args = (ads1256_task_args_t*)pvParameters;
    ads1256_device_t device = args->device;
    TaskHandle_t* task_handle = args->task_handle;
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
        *task_handle = NULL;
        vTaskDelete(NULL);
        return;
    }

    while (!readc_stop_flag)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if(!_ads1256_spi_transmit_queued(dummy_data, sizeof(dummy_data), buffer_readc_current[buffer_readc_index].data, 3))
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
        
            if(!xSemaphoreTake(current_mutex, portMAX_DELAY))
            {
                ESP_LOGE("ADS1256", "Failed to take mutex after switching buffers");
                vTaskDelete(NULL);
                return;
            }

            xTaskNotifyGive(sd_task);
        
            ESP_LOGI("ADS1256", "Switched to buffer %s", (current_mutex == readc_A_mutex) ? "A" : "B");
        }

    }

    xSemaphoreGive(current_mutex);

    new_filename_flag = true; 
    ESP_LOGI("ADS1256", "Stopping continuous read task for device %d", ads1256_device_to_number(device));

    ads1256_stop_continuous_read(device);
    *task_handle = NULL;
    vTaskDelete(NULL);

}


void ads1256_start_readc(ads1256_device_t device)
{
    ads1256_task_args_t* args = malloc(sizeof(ads1256_task_args_t));
    args->device = device;

    if(device != ADS1256_DEVICE_1) //TODO : remove this line later
    {
        ESP_LOGE("ADS1256", "SPI QUEUE DZIALA TYLKO DLA DEVICE_1");
        free(args);
        return;
    }


    if (device == ADS1256_DEVICE_1)
    {
        args->task_handle = &DRDY1_task;
    }
    else if (device == ADS1256_DEVICE_2)
    {
        args->task_handle = &DRDY2_task;
    }
    else 
    {
        ESP_LOGE("ADS1256", "Invalid device number: %d", ads1256_device_to_number(device));
        free(args);
        return;
    }

    if(read_mux_stop_flag == false)
    {
        read_mux_stop_flag = true;
        vTaskDelay(pdMS_TO_TICKS(50)); // Wait for any ongoing read_mux task to finish
    }

    delete_weight_sd_task();

    ads1256_change_channel(device, 0);
    ads1256_sync(device);
    ads1256_wake_up(device);

    ads1256_start_continuous_read(device);
    vTaskDelay(pdMS_TO_TICKS(1));

    if (*args->task_handle != NULL) {
        ESP_LOGE("ADS1256", "Task handle is not NULL, cannot create new task for device %d", ads1256_device_to_number(device));
        free(args);
        return;
    }

    if (xTaskCreate(ads1256_read_data_continuously, "ads1256_task_readc", 8192, args, 10, args->task_handle) != pdPASS) {
        ESP_LOGE("ADS1256", "Failed to create ADS1256 read task");
        free(args); 
    }
}


void ads1256_data_from_channels(void*  pvParameters)
{
    ads1256_task_args_t* args = (ads1256_task_args_t*)pvParameters;
    ads1256_device_t device = args->device;
    TaskHandle_t* task_handle = args->task_handle;
    free(args);

    ads1256_data_t data = {{0.0f, 0.0f, 0.0f, 0.0f}};
    ads1256_data_t sample_batch[ADS1256_UPDATE_DATA_AVG_SAMPLES];
    read_mux_stop_flag = false;
    uint8_t cur = 0;
    uint8_t batch_index = 0;

    ads1256_change_channel(device, 0);
    ads1256_sync(device);
    ads1256_wake_up(device);

    while (!read_mux_stop_flag)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        uint8_t raw_data[3] = {0, 0, 0};
        if (!ads1256_get_raw_data(device, raw_data) ||
            !ads1256_raw_data_to_value(device, raw_data, &data.weight[cur], cur)) {
            ESP_LOGE("ADS1256", "Failed to read channel %d", cur);
        }

        cur = (cur + 1) % 4;
        if (cur == 0) {
            sample_batch[batch_index++] = data;
            if (batch_index >= ADS1256_UPDATE_DATA_AVG_SAMPLES) {
                ads1256_update_data_struct(device, sample_batch, ADS1256_UPDATE_DATA_AVG_SAMPLES);
                batch_index = 0;
            }
        }

        ads1256_change_channel(device, cur);
        ads1256_sync(device);
        ads1256_wake_up(device);
    }

    *task_handle = NULL;
    vTaskDelete(NULL);
    
}


void ads1256_start_channel_task(ads1256_device_t device)
{
    ads1256_task_args_t* args = malloc(sizeof(ads1256_task_args_t));
    args->device = device;

    if (device == ADS1256_DEVICE_1)
    {
        args->task_handle = &DRDY1_task;
    }
    else if (device == ADS1256_DEVICE_2)
    {
        args->task_handle = &DRDY2_task;
    }
    else 
    {
        ESP_LOGE("ADS1256", "Invalid device number: %d", ads1256_device_to_number(device));
        free(args);
        return;
    }

    if (*args->task_handle != NULL) {
        ESP_LOGE("ADS1256", "Task handle is not NULL, cannot create new task for device %d", ads1256_device_to_number(device));
        free(args);
        return;
    }

    if(xTaskCreate(ads1256_data_from_channels, "ads1256_channel_task", 4096, args, 10, args->task_handle) != pdPASS)
    {
        ESP_LOGE("ADS1256", "Failed to create ADS1256 channel task for device %d", ads1256_device_to_number(device));
        free(args);
        return;
    }
}

void ads1256_suspend_task(ads1256_device_t device)
{
    TaskHandle_t *task_handle;

    if(device == ADS1256_DEVICE_1) {
        task_handle = &DRDY1_task;
    } else if(device == ADS1256_DEVICE_2) {
        task_handle = &DRDY2_task;
    } else {
        ESP_LOGE("ADS1256", "Invalid device number: %d", ads1256_device_to_number(device));
        return;
    }

    if (*task_handle != NULL) {
        vTaskSuspend(*task_handle);
        ESP_LOGI("ADS1256", "Suspended channel task for device %d", ads1256_device_to_number(device));
    } else {
        ESP_LOGE("ADS1256", "Task handle is NULL, cannot suspend task for device %d", ads1256_device_to_number(device));
    }
}

void ads1256_resume_task(ads1256_device_t device)
{
    TaskHandle_t *task_handle;

    if(device == ADS1256_DEVICE_1) {
        task_handle = &DRDY1_task;
    } else if(device == ADS1256_DEVICE_2) {
        task_handle = &DRDY2_task;
    } else {
        ESP_LOGE("ADS1256", "Invalid device number: %d", ads1256_device_to_number(device));
        return;
    }

    if (*task_handle != NULL) {
        vTaskResume(*task_handle);
        ESP_LOGI("ADS1256", "Resumed channel task for device %d", ads1256_device_to_number(device));
    } else {
        ESP_LOGE("ADS1256", "Task handle is NULL, cannot resume task for device %d", ads1256_device_to_number(device));
    }
}

void ads1256_delete_task(ads1256_device_t device)
{
    TaskHandle_t *task_handle;

    if(device == ADS1256_DEVICE_1) {
        task_handle = &DRDY1_task;
    } else if(device == ADS1256_DEVICE_2) {
        task_handle = &DRDY2_task;
    } else {
        ESP_LOGE("ADS1256", "Invalid device number: %d", ads1256_device_to_number(device));
        return;
    }

    if (*task_handle != NULL) {
       *task_handle = NULL;
        ESP_LOGI("ADS1256", "Deleted task for device %d", ads1256_device_to_number(device));
    } else {
        ESP_LOGE("ADS1256", "Task handle is NULL, cannot delete task for device %d", ads1256_device_to_number(device));
    }
}