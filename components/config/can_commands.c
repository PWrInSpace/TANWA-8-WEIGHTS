
#include "can_commands.h"

#include "esp_log.h"
#include "ads1256.h"
#include "app_task.h"
#include <string.h>
#include "can_api.h"

bool only_readc_task = false;
esp_err_t start_readc_command_handler(uint8_t *data, uint8_t length)
{
    only_readc_task = true;

    ads1256_device_t ads_device;
    uint8_t time;

    if (length < 2) {
        ESP_LOGE("CAN_COMMANDS", "Invalid data length for get_ads_single_channel_weight");
        return ESP_ERR_INVALID_ARG;
    }

    if(data[0] == 1) {ads_device = ADS1256_DEVICE_1;}
    else if(data[0] == 2) {ads_device = ADS1256_DEVICE_2;}
    else 
    {
        ESP_LOGE("CAN_COMMANDS", "podano zlego ads device : %d", data[0]);
        return ESP_FAIL;
    }

    time = data[1];
    start_readc_task(ads_device, 45);
    return ESP_OK;
}

esp_err_t get_ads_single_channel_weight(uint8_t *data, uint8_t length)
{

    if(only_readc_task)
    {
        return ESP_OK; 
    }

    ads1256_device_t ads_device;
    uint8_t channel_num;

    if (length < 2) {
        ESP_LOGE("CAN_COMMANDS", "Invalid data length for get_ads_single_channel_weight");
        return ESP_ERR_INVALID_ARG;
    }

    if(data[0] == 1) {ads_device = ADS1256_DEVICE_1;}
    else if(data[0] == 2) {ads_device = ADS1256_DEVICE_2;}
    else 
    {
        ESP_LOGE("CAN_COMMANDS", "podano zlego ads device : %d", data[0]);
        return ESP_FAIL;
    }
    channel_num = data[1]; //TODO channel num nie zzmienia channelu, tylko bierze jego kalibracje (raw z aktualnego channelu)
    // ESP_LOGI("CAN_COMMANDS", "Getting weight from device %d, channel %d", ads_device, channel_num);
    float weight = 0.0f;
    uint8_t raw_data;
    ads1256_get_raw_data(ads_device, &raw_data);
    ads1256_raw_data_to_weight(&raw_data, ads_device, &weight, channel_num - 1);
    ESP_LOGI("CAN_COMMANDS", "Weight from device %d [N], channel %d: %f", ads_device, channel_num, weight);

    uint8_t resp[4];
    memcpy(resp, &weight, sizeof(weight));
    esp_err_t err = can_send_message(0x3F20, resp, sizeof(resp));
    return err;

}

