#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"

#include "board_config.h"
#include "setup_task.h"
#include "ads1256.h"
#include "mcu_gpio_config.h"
#include "sd_task.h"
#include "ads1256_task.h"
#define TAG "APP"

extern board_config_t config;

void app_main(void) {
    

    ESP_LOGI(TAG, "%s TANWA board starting", config.board_name);
    
    if(setup_task_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize setup task");
        return;
    }
    vTaskDelay(pdMS_TO_TICKS(5000));
    ads1256_start_readc(ADS1256_DEVICE_1);
    // ads1256_read_id(ADS1256_DEVICE_2);
    // ads1256_start_channel_task(ADS1256_DEVICE_1);
    run_test_task();
}
