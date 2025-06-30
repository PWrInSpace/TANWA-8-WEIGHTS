#include "ads1256.h"
#include "mcu_spi_config.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "ads1256_task.h"

uint8_t* buffer_readc_A;
uint8_t* buffer_readc_B;
uint8_t* buffer_readc_current;
uint32_t buffer_readc_index = 0;
SemaphoreHandle_t readc_A_mutex = NULL;
SemaphoreHandle_t readc_B_mutex = NULL;
SemaphoreHandle_t *current_mutex = NULL;

bool ads1256_task_init(void)
{
    buffer_readc_A = (uint8_t*)heap_caps_malloc(BUFFER_READC_SIZE * sizeof(uint8_t), MALLOC_CAP_DMA);
    buffer_readc_B = (uint8_t*)heap_caps_malloc(BUFFER_READC_SIZE * sizeof(uint8_t), MALLOC_CAP_DMA);
    buffer_readc_current = buffer_readc_A;

    readc_A_mutex = xSemaphoreCreateMutex();
    readc_B_mutex = xSemaphoreCreateMutex();
    current_mutex = &readc_A_mutex;

    if (!buffer_readc_A || !buffer_readc_B) {
        ESP_LOGE("ADS1256", "Failed to allocate memory for buffers");
        return false;
    }

    if(buffer_readc_A == NULL || buffer_readc_B == NULL)
    {
        ESP_LOGE("ADS1256", "Failed to allocate memory for buffers");
        return false;
    }

    if (!ads1256_pins_init()) {
        ESP_LOGE("ADS1256", "Failed to initialize ADS1256 pins");
        return false;
    }

    // if (!ads1256_init(ADS1256_DEVICE_1)) {
    //     ESP_LOGE("ADS1256", "Failed to initialize ADS1256 device 1");
    //     return false;
    // }

    if (!ads1256_init(ADS1256_DEVICE_2)) {
        ESP_LOGE("ADS1256", "Failed to initialize ADS1256 device 2");
        return false;
    }

    return true;
}

void ads1256_read_data_continuously(void*  pvParameters)
{
    ads1256_device_t* device = (ads1256_device_t*)pvParameters;
    uint8_t dummy_data[3] = {0x00, 0x00, 0x00}; 
    ads1256_raw_data_sample_t raw_data;

    if(!xSemaphoreTake(*current_mutex, portMAX_DELAY))
    {
        ESP_LOGE("ADS1256", "Failed to take mutex");
        free(device);
        vTaskDelete(NULL);
        return;
    }

    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // gpio_set_level(*device, 0);
        if(!_ads1256_spi_transmit_queued(dummy_data, sizeof(dummy_data), buffer_readc_current + buffer_readc_index, 3))
        {
            ESP_LOGE("ADS1256", "Failed to read data from ADS1256");
        }
        // gpio_set_level(*device, 1);
        buffer_readc_index+=3;

        if(buffer_readc_index % 1000 == 2) 
        {
            ESP_LOGI("ADS1256", "Buffered value[%d]: %d", buffer_readc_index, buffer_readc_current[buffer_readc_index - 1]);
        }

        if (buffer_readc_index >= BUFFER_READC_SIZE) {
            buffer_readc_index = 0; 
            if (*current_mutex == readc_A_mutex) {
                xSemaphoreGive(readc_A_mutex);
                current_mutex = &readc_B_mutex;
                buffer_readc_current = buffer_readc_B;
            } else {
                xSemaphoreGive(readc_B_mutex);
                current_mutex = &readc_A_mutex;
                buffer_readc_current = buffer_readc_A;
            }

            if(!xSemaphoreTake(*current_mutex, portMAX_DELAY))
            {
                ESP_LOGE("ADS1256", "Failed to take mutex after switching buffers");
                free(device);
                vTaskDelete(NULL);
                return;
            }

            ESP_LOGI("ADS1256", "Buffer switched to %s", (*current_mutex == readc_A_mutex) ? "A" : "B");
        }

        

    }

    free(device);
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
    else
    {
        ESP_LOGI("ADS1256", "Continuous read started on %s", ads1256_device_to_string(device));
    }

    vTaskDelay(pdMS_TO_TICKS(1)); 

    gpio_set_level(device, 1); 


    xTaskCreate(ads1256_read_data_continuously, "ads1256_task_readc", 4096, (void*)device_ptr, 10, &DRDY1_task);


}