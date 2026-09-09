#include "can_commands.h"

#include "esp_log.h"
#include "ads1256_wrapper.h"
#include "app_task.h"
#include "board_config.h"
#include <string.h>
#include "can_api.h"

// ##### local variables #####
static const char *TAG = "CAN_COMMANDS";
static bool only_readc_task = false;
// ##### local variables #####


// ##### help functions #####
bool valid_data_length(uint8_t length, uint8_t expected_length) {
    if (length != expected_length) {
        ESP_LOGE(TAG, "Invalid data length: expected %d, got %d", expected_length, length);
        return false;
    }
    return true;
}

// bool valid_and_set_time(uint8_t* time_tab, uint16_t *time) {
//     if (time_tab == NULL || time == NULL) {
//         ESP_LOGE(TAG, "Invalid time pointer");
//         return false;
//     }
//     *time = (time_tab[0] << 8) | time_tab[1];

//     if(*time < 1 || *time > 600) {
//         ESP_LOGE(TAG, "Invalid time value: %d", *time);
//         return false;
//     }
//     return true;
// }

bool valid_and_set_channel(uint8_t channel_num, uint8_t *channel) {
    if (channel_num > 3) {
        ESP_LOGE(TAG, "Invalid channel number: %d", channel_num);
        return false;
    }
    *channel = channel_num;
    return true;
}
// ##### help functions #####

// ##### command handlers #####

esp_err_t can_get_status(uint8_t *data, uint8_t length)
{
    if(!valid_data_length(length, 0)) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t resp[3] = {0x00, 0x00, 0x00};
    //dodac to co trzeba czyli temp0 temp1 I_curr

    esp_err_t err = can_send_message(CAN_SEND_STATUS, resp, sizeof(resp));
    return err;
}

esp_err_t can_get_board_data(uint8_t *data, uint8_t length)
{

    if(!valid_data_length(length, 0)) {
        return ESP_ERR_INVALID_ARG;
    }

    //przygotuj structa i tablice uint8_t

    // esp_err_t err = can_send_message(CAN_SEND_BOARD_DATA, tablica, sizeof(resp));
    // return err;

    return ESP_OK; //aby sie kompilowalo 

}

esp_err_t can_start_measure(uint8_t *data, uint8_t length)
{
    /*
    * data length = 3 bytes
    * data[0] = device ID (only 1 is supported)
    * data[1...2] = time (uint16_t, 1-600 seconds)
    */

    uint16_t time;
    only_readc_task = true;

    if(!valid_data_length(length, 3)) {
        return ESP_ERR_INVALID_ARG;
    }

    ads1256_wrapper_t* w = board_get_ads1256(data[0]);
    if (w == NULL) {
        ESP_LOGE(TAG, "Device %d not found", data[0]);
        return ESP_FAIL;
    }

    // if(!valid_and_set_time(&data[1], &time)) {
    //     return ESP_FAIL;
    // }
    memcpy(&time, &data[1], sizeof(uint16_t));

if (!start_readc_task(w, time)) {
    ESP_LOGE(TAG, "Failed to start measurement");
    return ESP_FAIL;
}

return ESP_OK;
}

esp_err_t can_ads_tare(uint8_t *data, uint8_t length)
{
    /*
    * data length = 1 byte
    * data[0] = device ID (only 1 is supported)
    */

    if(!valid_data_length(length, 1)) {
        return ESP_ERR_INVALID_ARG;
    }

    ads1256_wrapper_t* w = board_get_ads1256(data[0]);
    if (w == NULL) {
        ESP_LOGE(TAG, "Device %d not found", data[0]);
        return ESP_FAIL;
    }

    if (!ads1256_tare_all(w)) {
        ESP_LOGE(TAG, "Tare failed on device %d", data[0]);
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t can_set_ads_ch(uint8_t *data, uint8_t length)
{
    /*
    * data length = 2 bytes
    * data[0] = device ID (only 1 is supported)
    * data[1] = channel_num (0-3)
    */

    uint8_t channel_num;

    if(!valid_data_length(length, 2)) {
        return ESP_ERR_INVALID_ARG;
    }

    ads1256_wrapper_t* w = board_get_ads1256(data[0]);
    if (w == NULL) {
        ESP_LOGE(TAG, "Device %d not found", data[0]);
        return ESP_FAIL;
    }

    if(!valid_and_set_channel(data[1], &channel_num)) {
        return ESP_FAIL;
    }

    if(!ads1256_change_channel(w, channel_num)) {
        ESP_LOGE(TAG, "Failed to change channel on device %d", data[0]);
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t can_set_ads_offset(uint8_t *data, uint8_t length)
{
    /*
    * data length = 6 bytes
    * data[0] = device ID (only 1 is supported)
    * data[1] = channel_num (0-3)
    * data[2..5] = offset (int32_t)
    */

    int32_t offset;
    uint8_t channel_num;

    if(!valid_data_length(length, 6)) {
        return ESP_ERR_INVALID_ARG;
    }

    ads1256_wrapper_t* w = board_get_ads1256(data[0]);
    if (w == NULL) {
        ESP_LOGE(TAG, "Device %d not found", data[0]);
        return ESP_FAIL;
    }

    if(!valid_and_set_channel(data[1], &channel_num)) {
        return ESP_FAIL;
    }

    memcpy(&offset, &data[2], sizeof(offset));

    if(!ads1256_set_zero_offset(w, offset, channel_num)) {
        ESP_LOGE(TAG, "Failed to set zero offset on device %d", data[0]);
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t can_get_ads_ch_all_weight(uint8_t *data, uint8_t length)
{
    /*
    * data length = 1 byte
    * data[0] = device ID (only 1 is supported)
    */

    if(!valid_data_length(length, 1)) {
        return ESP_ERR_INVALID_ARG;
    }

    ads1256_wrapper_t* w = board_get_ads1256(data[0]);
    if (w == NULL) {
        ESP_LOGE(TAG, "Device %d not found", data[0]);
        return ESP_FAIL;
    }

    ads1256_data_t ads_data;
    if(!ads1256_get_data_struct_copy(w, &ads_data)) {
        ESP_LOGE(TAG, "Failed to get data");
        return ESP_FAIL;
    }

    uint8_t resp[16];
    memcpy(resp, &ads_data.weight[0], sizeof(float));
    memcpy(&resp[4], &ads_data.weight[1], sizeof(float));
    memcpy(&resp[8], &ads_data.weight[2], sizeof(float));
    memcpy(&resp[12], &ads_data.weight[3], sizeof(float));

    if(can_send_message(CAN_SEND_ADS1_ALL_CH_WEIGHT1, resp, sizeof(resp)/2) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send CAN message for ADS1 channel 1 weight");
        return ESP_FAIL;
    }
    if(can_send_message(CAN_SEND_ADS1_ALL_CH_WEIGHT2, &resp[8], sizeof(resp)/2) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send CAN message for ADS1 channel 2 weight");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t can_get_ads_ch_weight(uint8_t *data, uint8_t length)
{
    /*
    * Sends all four channels from board device ID 1.
    */

    (void)data;
    (void)length;

    ads1256_data_t ads_data;
    if(!ads1256_get_data_struct_copy(board_get_ads1256(1), &ads_data)) {
        ESP_LOGE(TAG, "Failed to get data");
        return ESP_FAIL;
    }

    float weight0 = ads_data.weight[0];
    float weight1 = ads_data.weight[1];
    float weight2 = ads_data.weight[2];
    float weight3 = ads_data.weight[3];


    uint8_t resp0[6];
    uint8_t resp1[6];
    uint8_t resp2[6];
    uint8_t resp3[6];
    memcpy(resp0, &weight2, sizeof(weight0));
    resp0[4] = 1;
    resp0[5] = 0;
    esp_err_t err = can_send_message(CAN_SEND_ADS_CH_WEIGHT, resp0, sizeof(resp0));
    memcpy(resp1, &weight2, sizeof(weight0));
    resp1[4] = 1;
    resp1[5] = 1;
    err = can_send_message(CAN_SEND_ADS_CH_WEIGHT, resp1, sizeof(resp1));
    memcpy(resp2, &weight0, sizeof(weight0));
    resp2[4] = 1;
    resp2[5] = 2;
    err = can_send_message(CAN_SEND_ADS_CH_WEIGHT, resp2, sizeof(resp2));
    memcpy(resp3, &weight1, sizeof(weight0));
    resp3[4] = 1;
    resp3[5] = 3;
    err = can_send_message(CAN_SEND_ADS_CH_WEIGHT, resp3, sizeof(resp3));
  //  ESP_LOGI(TAG, "resp3: %d %d %d %d %d %d", resp3[0], resp3[1], resp3[2], resp3[3], resp3[4], resp3[5]);
    return err;
}

esp_err_t can_get_weights(uint8_t *data, uint8_t length)
{
    if (!valid_data_length(length, 0)) {
        return ESP_ERR_INVALID_ARG;
    }

    ads1256_data_t ads_data;
    if (!ads1256_get_data_struct_copy(board_get_ads1256(1), &ads_data)) {
        ESP_LOGE(TAG, "Failed to get ADS1 data");
        return ESP_FAIL;
    }

    float rocket_weight = ads_data.weight[0] + ads_data.weight[1];
    float n2o_weight = ads_data.weight[2] + ads_data.weight[3];

    uint8_t resp[8];
    memcpy(resp, &n2o_weight, sizeof(n2o_weight));
    memcpy(&resp[4], &rocket_weight, sizeof(rocket_weight));
    return can_send_message(CAN_SEND_WEIGHTS, resp, sizeof(resp));
}

// ##### command handlers #####