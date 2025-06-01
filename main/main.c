#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"

#include "board_config.h"
#include "setup_task.h"

#define TAG "APP"

extern board_config_t config;

void app_main(void) {
    
    // CONFIGURE THE MESSAGE

    ESP_LOGI(TAG, "%s TANWA board starting", config.board_name);
    
    if(setup_task_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize setup task");
        return;
    }

    while(1) {
        led_toggle(&(config.status_led));
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}