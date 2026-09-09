#include "can_config.h"
#include "can_api.h"
#include "can_commands.h"

#include "esp_log.h"
#include "esp_err.h"

#include "driver/twai.h"

#define TAG "CAN_CONFIG"

can_command_t can_commands[] = {
    {CAN_START_MEASURE, can_start_measure},
    {CAN_GET_ADS_CH_WEIGHT, can_get_ads_ch_weight},
    {CAN_GET_WEIGHTS, can_get_weights},
    {CAN_ADS_TARE, can_ads_tare},
    {CAN_GET_BOARD_DATA, can_get_board_data},
    {CAN_GET_STATUS, can_get_status},
    {CAN_SET_ADS_CH, can_set_ads_ch},
    {CAN_SET_ADS_OFFSET, can_set_ads_offset},
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