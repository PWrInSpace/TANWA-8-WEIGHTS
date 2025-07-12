#include "ads1256.h"
#include "mcu_spi_config.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "ads1256_task.h"
#include "esp_timer.h"

volatile bool readc_stop_flag = false;
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

    return true;
}

void ads1256_read_data_continuously(void*  pvParameters)
{
    ads1256_device_t* device = (ads1256_device_t*)pvParameters;
    uint8_t dummy_data[3] = {0x00, 0x00, 0x00}; 
    int64_t start_time_us = esp_timer_get_time();
    buffer_readc_index = 0;
    readc_stop_flag = false;

    if(!xSemaphoreTake(current_mutex, portMAX_DELAY))
    {
        ESP_LOGE("ADS1256", "Failed to take mutex");
        // free(device);
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
        buffer_readc_current[buffer_readc_index].time = (uint16_t)(((esp_timer_get_time() - start_time_us)/1000) & 0xFFFF);

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
                // free(device);
                vTaskDelete(NULL);
                return;
            }
        
            ESP_LOGI("ADS1256", "Switched to buffer %s", (current_mutex == readc_A_mutex) ? "A" : "B");
        }

    }

    xSemaphoreGive(current_mutex);


    // free(device);
    vTaskDelete(NULL);

}


void ads1256_start_readc(ads1256_device_t device)
{
    uint8_t tx_data = RDATAC_COMMAND;

    ads1256_device_t* device_ptr = malloc(sizeof(ads1256_device_t));
    if (device_ptr == NULL) {
        ESP_LOGE("ADS1256", "Failed to allocate memory for device");
        return;
    }

    *device_ptr = device;

    gpio_set_level(device, 0); 
    if(ads1256_single_transmit(device, &tx_data, sizeof(tx_data)) == false)
    {
        ESP_LOGE("ADS1256", "Failed to start continuous read on ADS1256");
    }

    vTaskDelay(pdMS_TO_TICKS(1)); 

    gpio_set_level(device, 1); 


    xTaskCreate(ads1256_read_data_continuously, "ads1256_task_readc", 4096, NULL, 10, &DRDY1_task);


}