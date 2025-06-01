#include "can_config.h"
#include "can_api.h"
#include "can_commands.h"

#include "esp_log.h"
#include "esp_err.h"

#include "driver/twai.h"

#define TAG "CAN_CONFIG"

can_command_t can_commands[] = {
    // Example command registration
    //{CAN_TEMPLATE_MESSAGE_ID, example_command_handler},
    // Add your CAN commands here
};

esp_err_t can_config_init(void) {
    esp_err_t err;

    // Register CAN commands
    err = can_register_commands(can_commands, sizeof(can_commands) / sizeof(can_commands[0]));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "CAN command registration failed");
        return err;
    }

    // Initialize CAN driver
    err = can_task_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "CAN driver initialization failed");
        return err;
    }

    // Start CAN driver
    err = can_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "CAN driver start failed");
        return err;
    }

    return ESP_OK;
}