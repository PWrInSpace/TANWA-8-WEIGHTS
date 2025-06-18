#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"

#include "board_config.h"
#include "setup_task.h"
#include "ads1256.h"
#include "mcu_gpio_config.h"
#define TAG "APP"

extern board_config_t config;
ads1256_raw_data_t ads1256_data;
uint8_t ads1256_id = 0;

void app_main(void) {
    

    ESP_LOGI(TAG, "%s TANWA board starting", config.board_name);
    
    if(setup_task_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize setup task");
        return;
    }
    // ads1256_read_id2();

    while(1) {
        ESP_LOGI(TAG, "DUPA");
        // ads1256_read_id(ADS1256_DEVICE_1);
        // ads1256_read_id(ADS1256_DEVICE_2);
        // ads1256_read_id2();
        ads1256_read_id(ADS1256_DEVICE_1);
        ads1256_read_id(ADS1256_DEVICE_2);
        // gpio_set_level(15, 1); 
        // ESP_LOGI(TAG, "GPIO 15 set to HIGH - lvl: %d", gpio_get_level(15));
        // if(!ads1256_read_id(&ads1256_id)) {
        //     ESP_LOGE(TAG, "Failed to read ADS1256 ID");
        // }
        // led_toggle(&(config.status_led));
        // ads1256_get_raw_data(&ads1256_data);
        // if(ads1256_get_raw_data(&ads1256_data)) {
        //     ESP_LOGI(TAG, "ADS1256 Data: %d %d %d", ads1256_data.channel_1[0], ads1256_data.channel_1[1], ads1256_data.channel_1[2]);
        // } else {
        //     ESP_LOGE(TAG, "Failed to get data from ADS1256");
        // }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// #include "driver/gpio.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_log.h"

// #define CS_GPIO_1 GPIO_NUM_7
// #define CS_GPIO_2 GPIO_NUM_15
// static const char *TAG = "gpio_example";

// bool ads1256_pins_init(void)
// {
//    /*init lokalny gpio output*/
//    gpio_config_t io_conf = {
//        .mode = GPIO_MODE_INPUT_OUTPUT,
//        .pull_up_en = GPIO_PULLUP_DISABLE,
//        .pull_down_en = GPIO_PULLDOWN_DISABLE,
//        .intr_type = GPIO_INTR_DISABLE
//    };

//    gpio_num_t output_pins[] = {
//        CS_GPIO_1,
//        CS_GPIO_2
//    };

//    for (int i = 0; i < sizeof(output_pins)/sizeof(output_pins[0]); i++) {
//        io_conf.pin_bit_mask = 1ULL << output_pins[i];
//        gpio_config(&io_conf);
//    }

//    ESP_ERROR_CHECK(gpio_set_level(CS_GPIO_1, 1));  
//    ESP_ERROR_CHECK(gpio_set_level(CS_GPIO_2, 1)); 

   

//    /*Debug problemu z ustawianiem pin lvl na outpucie*/
//    if(gpio_get_level(CS_GPIO_1) == 1)
//    {
//        ESP_LOGI("ADS1256", "GPIO %d is set to HIGH (correctly configured)", CS_GPIO_1);
//    }
//    else
//    {
//        ESP_LOGE("ADS1256", "GPIO %d is not set to HIGH (check configuration)", CS_GPIO_1);
//    }
   
//    if(gpio_get_level(CS_GPIO_2) == 1)
//    {
//        ESP_LOGI("ADS1256", "GPIO %d is set to HIGH (correctly configured)", CS_GPIO_2);
//    }
//    else
//    {
//        ESP_LOGE("ADS1256", "GPIO %d is not set to HIGH (check configuration)", CS_GPIO_2);
//    }

//    return true;
// }

// void app_main(void)
// {
//     while (1) {
//         ads1256_pins_init();
//         vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second
//     }
// }
