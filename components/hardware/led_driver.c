///===-----------------------------------------------------------------------------------------===//
///
/// Copyright (c) PWr in Space. All rights reserved.
/// Created: 27.01.2024 by Michał Kos
///
///===-----------------------------------------------------------------------------------------===//
#include "led_driver.h"
#include "esp_log.h"

#define TAG "LED"

led_status_t led_set_state(led_struct_t* led, led_state_t state) {
    if (led == NULL) {
        ESP_LOGE(TAG, "Invalid argument - NULL check failed");
        return LED_NULL_ARG;
    }
    if (state == LED_STATE_ON) {
        if (led->state == LED_STATE_ON) {
            ESP_LOGD(TAG, "Already ON");
            return LED_OK;
        } else {
            ESP_LOGD(TAG, "Turning ON");
            if (!led->_gpio_set_level(led->gpio_num, LED_STATE_ON)) {
                ESP_LOGE(TAG, "Failed to turn ON");
                return LED_GPIO_ERR;
            }
            led->state = LED_STATE_ON;
            return LED_OK;
        }
    } else if (state == LED_STATE_OFF) {
        if (led->state == LED_STATE_OFF) {
            ESP_LOGD(TAG, "Already OFF");
            return LED_OK;
        } else {
            ESP_LOGD(TAG, "Turning OFF");
            if (!led->_gpio_set_level(led->gpio_num, LED_STATE_OFF)) {
                ESP_LOGE(TAG, "Failed to turn OFF");
                return LED_GPIO_ERR;
            }
            led->state = LED_STATE_OFF;
            return LED_OK;
        }
    } else {
        ESP_LOGE(TAG, "Invalid argument - out of value range");
        return LED_INVALID_ARG;
    }
}

led_status_t led_toggle(led_struct_t* led) {
    if (led == NULL) {
        ESP_LOGE(TAG, "Invalid argument - NULL check failed");
        return LED_NULL_ARG;
    }
    if (led->state == LED_STATE_ON) {
        ESP_LOGD(TAG, "Toggling OFF");
        led_status_t ret = led_set_state(led, LED_STATE_OFF);
        if (ret != LED_OK) {
            ESP_LOGE(TAG, "Failed to toggle OFF");
            return ret;
        }
        return LED_OK;
    } else if (led->state == LED_STATE_OFF) {
        ESP_LOGD(TAG, "Toggling ON");
        led_status_t ret = led_set_state(led, LED_STATE_ON);
        if (ret != LED_OK) {
            ESP_LOGE(TAG, "Failed to toggle ON");
            return ret;
        }
        led->state = LED_STATE_ON;
        return LED_OK;
    } else {
        ESP_LOGE(TAG, "Invalid argument - out of value range");
        return LED_INVALID_ARG;
    }
}

led_status_t led_delay_blink(led_struct_t* led, uint32_t period_ms) {
    if (led == NULL) {
        ESP_LOGE(TAG, "Invalid argument - NULL check failed");
        return LED_NULL_ARG;
    }
    if (period_ms == 0) {
        ESP_LOGE(TAG, "Invalid argument - out of value range");
        return LED_INVALID_ARG;
    }
    ESP_LOGD(TAG, "Blinking with period %d ms", period_ms);
    led_status_t ret = led_toggle(led);
    if (ret != LED_OK) {
        ESP_LOGE(TAG, "Failed to toggle LED");
        return ret;
    }
    led->_delay(period_ms);
    ret = led_toggle(led);
    if (ret != LED_OK) {
        ESP_LOGE(TAG, "Failed to toggle LED");
        return ret;
    }
    return LED_OK;
}