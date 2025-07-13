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
#include "app_task.h"
#define TAG "APP"

extern board_config_t config;

void app_main(void) {
    

    ESP_LOGI(TAG, "%s TANWA board starting", config.board_name);


    if(setup_task_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize setup task");
        return;
    }

    //hamownia channel 2 na ads cs=15 cfg
    // uint8_t tst[3] = {0x9D, 0xF6, 0xFF};
    // uint8_t tst2[3] = {0x79, 0xBA, 0x49};

}

