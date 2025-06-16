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

    while(1) {
        ESP_LOGI(TAG, "DUPA");
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
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

// #include "driver/gpio.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_log.h"

// #define PIN GPIO_NUM_7
// static const char *TAG = "gpio_example";

// void app_main(void)
// {
//     while (1) {
//         // Ustaw pin jako wyjście i na 1
//         gpio_set_direction(PIN, GPIO_MODE_INPUT);
//         gpio_set_level(PIN, 1);

//         int level = gpio_get_level(PIN);
//         ESP_LOGI(TAG, "Pin set as INPUT, read level: %d", level);
//         vTaskDelay(1500 / portTICK_PERIOD_MS);

//         // Ustaw pin jako wyjście i na 0
//         // gpio_set_level(PIN, 0);

//         level = gpio_get_level(PIN);
//         ESP_LOGI(TAG, "Pin set as INPUT, read level: %d", level);
//         vTaskDelay(1500 / portTICK_PERIOD_MS);
//     }
// }
