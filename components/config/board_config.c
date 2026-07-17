///===-----------------------------------------------------------------------------------------===//
///
/// Copyright (c) PWr in Space. All rights reserved.
/// Created: 27.01.2024 by Szymon Rzewuski
///
///===-----------------------------------------------------------------------------------------===//
///
/// \file
/// This file contains declaration of the system console configuration, including initialization
/// and available commands for debugging/testing purposes.
///===-----------------------------------------------------------------------------------------===//

#include "board_config.h"

#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "mcu_gpio_config.h"
#include "mcu_twai_config.h"
#include "can_config.h"
#include "console_config.h"

#include "mcu_spi_config.h"
#include "ads1256_wrapper.h"
#include "sd_task.h"
#include "ads1256_task.h"
#include "timers_config.h"
#include "flash.h"


#define TAG "BOARD_CONFIG"

void _led_delay(uint32_t _ms) {
    vTaskDelay(_ms / portTICK_PERIOD_MS);
}

board_config_t config = {
    .board_name = "TANWA_BOARD", //CHANGE TO REAL BOARD NAME
    .status_led = {
        ._gpio_set_level = _mcu_gpio_set_level,
        ._delay = _led_delay,
        .gpio_num = CONFIG_GPIO_LED,
        .drive = LED_DRIVE_POSITIVE,
        .state = LED_STATE_OFF, 
    },
};

static ads1256_wrapper_t* ads1 = NULL;

ads1256_wrapper_t* board_get_ads1256(int id) {
    return id == 1 ? ads1 : NULL;
}

esp_err_t board_config_init(void) {

    esp_err_t err;
    
    err = mcu_gpio_init();

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "GPIO initialization failed");
        return err;
    }

    err = mcu_spi_init();

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SPI initialization failed");
        return err;
    }
    if(_ads1256_add_device() != true) {
        ESP_LOGE(TAG, "Failed to add ADS1256 device");
        return ESP_FAIL;
    }

    err = flash_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Flash/NVS initialization failed");
        return err;
    }

    ads1256_pin_config_t ads1_pins = {
        .cs_gpio   = 15,
        .drdy_gpio = 18,
        .reset_gpio = 17,
        .pwdn_gpio  = 16,
    };
    ads1 = ads1256_init(&ads1_pins);
    if (ads1 == NULL) {
        ESP_LOGE(TAG, "Failed to initialize ADS1256 device 1");
        return ESP_FAIL;
    }

    if (!timers_init()) {
        ESP_LOGE(TAG, "Failed to initialize timers");
        ads1256_deinit(ads1);
        ads1 = NULL;
        return ESP_FAIL;
    }

    if(!ads1256_task_init(ads1)) {
        ESP_LOGE(TAG, "Failed to initialize ADS1256 task");
        ads1256_deinit(ads1);
        ads1 = NULL;
        return ESP_FAIL;
    }


    err = mcu_twai_init();

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "TWAI initialization failed");
    }

    err = can_config_init();

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "CAN initialization failed");
    }

    err = console_config_init();

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Console initialization failed");
        return err;
    }

    // err = sd_task_init();

    // if (err != ESP_OK) {
    //     ESP_LOGE(TAG, "SD task initialization failed");
    //     return err;
    // }

    // run_weight_sd_task();


    
    return ESP_OK;

    //*********** ADD HARDWARE CONFIGURATION HERE ***********//

    
}