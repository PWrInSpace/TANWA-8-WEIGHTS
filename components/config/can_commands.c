#include "can_commands.h"

#include "esp_log.h"
#include "ads1256.h"
#include "app_task.h"
#include <string.h>
#include "can_api.h"

// ##### local variables #####
static const char *TAG = "CAN_COMMANDS";
static bool only_readc_task = false;
// ##### local vatriables #####


// ##### help functions #####
bool valid_data_length(uint8_t length, uint8_t expected_length) {
    if (length != expected_length) {
        ESP_LOGE(TAG, "Invalid data length: expected %d, got %d", expected_length, length);
        return false;
    }
    return true;
}

bool valid_and_set_device(uint8_t device_num, ads1256_device_t *device) {
    if (device_num == 1) {
        *device = ADS1256_DEVICE_1;
    } else if (device_num == 2) {
        *device = ADS1256_DEVICE_2;
    } else {
        // ESP_LOGE(TAG, "Invalid device number: %d", device_num);
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
    * data[0] = dev_num (1 or 2)
    * data[1...2] = time (uint16_t, 1-600 seconds)
    */

    ads1256_device_t ads_device;
    uint16_t time;
    only_readc_task = true;

    if(!valid_data_length(length, 3)) {
        return ESP_ERR_INVALID_ARG;
    }

    if(!valid_and_set_device(data[0], &ads_device)) {
        return ESP_FAIL;
    }

    // if(!valid_and_set_time(&data[1], &time)) {
    //     return ESP_FAIL;
    // }
    memcpy(&time, &data[1], sizeof(uint16_t));

    start_readc_task(ads_device, time);
    return ESP_OK;
}

esp_err_t can_ads_tare(uint8_t *data, uint8_t length)
{
    /*
    * data length = 1 byte
    * data[0] = dev_num (1 or 2)
    */

    ads1256_device_t ads_device;

    if(!valid_data_length(length, 1)) {
        return ESP_ERR_INVALID_ARG;
    }

    if(!valid_and_set_device(data[0], &ads_device)) {
        return ESP_FAIL;
    }

    //odczytaj pomiar ze structa i ustaw nowy offset

    return ESP_OK;
}

esp_err_t can_set_ads_ch(uint8_t *data, uint8_t length)
{
    /*
    * data length = 2 bytes
    * data[0] = dev_num (1 or 2)
    * data[1] = channel_num (0-3)
    */

    ads1256_device_t ads_device;
    uint8_t channel_num;

    if(!valid_data_length(length, 2)) {
        return ESP_ERR_INVALID_ARG;
    }

    if(!valid_and_set_device(data[0], &ads_device)) {
        return ESP_FAIL;
    }

    if(!valid_and_set_channel(data[1], &channel_num)) {
        return ESP_FAIL;
    }

    if(!ads1256_change_channel(ads_device, channel_num)) {
        ESP_LOGE(TAG, "Failed to change channel on device %d", ads1256_device_to_number(ads_device));
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t can_set_ads_offset(uint8_t *data, uint8_t length)
{
    /*
    * data length = 6 bytes
    * data[0] = dev_num (1 or 2)
    * data[1] = channel_num (0-3)
    * data[2..5] = offset (int32_t)
    */

    ads1256_device_t ads_device;
    int32_t offset;
    uint8_t channel_num;

    if(!valid_data_length(length, 6)) {
        return ESP_ERR_INVALID_ARG;
    }

    if(!valid_and_set_device(data[0], &ads_device)) {
        return ESP_FAIL;
    }

    if(!valid_and_set_channel(data[1], &channel_num)) {
        return ESP_FAIL;
    }

    memcpy(&offset, &data[2], sizeof(offset));

    if(!ads1256_set_zero_offset(ads_device, offset, channel_num)) {
        ESP_LOGE(TAG, "Failed to set zero offset on device %d", ads1256_device_to_number(ads_device));
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t can_get_ads_ch_all_weight(uint8_t *data, uint8_t length)
{
    /*
    * data length = 1 byte
    * data[0] = dev_num (1 or 2)
    */

    ads1256_device_t ads_device;

    if(!valid_data_length(length, 1)) {
        return ESP_ERR_INVALID_ARG;
    }

    if(!valid_and_set_device(data[0], &ads_device)) {
        return ESP_FAIL;
    }

    ads1256_data_t ads_data;
    if(!ads1256_get_data_struct_copy(ads_device, &ads_data)) {
        ESP_LOGE(TAG, "Failed to get data for ADS1256_DEVICE_1");
        return ESP_FAIL;
    }

    uint8_t resp[16];
    memcpy(resp, &ads_data.weight[0], sizeof(float));
    memcpy(&resp[4], &ads_data.weight[1], sizeof(float));
    memcpy(&resp[8], &ads_data.weight[2], sizeof(float));
    memcpy(&resp[12], &ads_data.weight[3], sizeof(float));
    
    if(ads_device == ADS1256_DEVICE_1) {
        if(can_send_message(CAN_SEND_ADS1_ALL_CH_WEIGHT1, resp, sizeof(resp)/2) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to send CAN message for ADS1 channel 1 weight");
            return ESP_FAIL;
        }
        if(can_send_message(CAN_SEND_ADS1_ALL_CH_WEIGHT2, &resp[8], sizeof(resp)/2) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to send CAN message for ADS1 channel 2 weight");
            return ESP_FAIL;
        }
    } else if(ads_device == ADS1256_DEVICE_2) {
        if(can_send_message(CAN_SEND_ADS2_ALL_CH_WEIGHT1, resp, sizeof(resp)/2) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to send CAN message for ADS2 channel 1 weight");
            return ESP_FAIL;
        }
        if(can_send_message(CAN_SEND_ADS2_ALL_CH_WEIGHT2, &resp[8], sizeof(resp)/2) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to send CAN message for ADS2 channel 2 weight");
            return ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "Invalid device number: %d", data[0]);
        return ESP_ERR_INVALID_ARG;
    }
    
    return ESP_OK;
}

esp_err_t can_get_ads_ch_weight(uint8_t *data, uint8_t length)
{
    /*
    * data length = 2 bytes
    * data[0] = dev_num (1 or 2)
    * data[1] = channel_num (0-3)
    */

    // ads1256_device_t ads_device;
    // uint8_t channel_num;

    // ESP_LOGI(TAG, "can_get_ads_ch_weight called with data[0..7] =  %d %d %d %d %d %d %d %d", 
            //  data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
    // ESP_LOGI(TAG, "can_get_ads_ch_weight called");
    // if(!valid_data_length(length, 0)) {
    //     return ESP_ERR_INVALID_ARG;
    // }

    // if(!valid_and_set_device(data[0], &ads_device)) {
    //     return ESP_FAIL;
    // }

    // if(!valid_and_set_channel(data[1], &channel_num)) {
    //     return ESP_FAIL;
    // }


    ads1256_data_t ads_data;
    if(!ads1256_get_data_struct_copy(ADS1256_DEVICE_1, &ads_data)) {
        ESP_LOGE(TAG, "Failed to get data for ADS1256_DEVICE_1");
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
    if(!valid_data_length(length, 0)) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t resp[8];
    float r_weight = 0.0f;
    float n2o_weight = 0.0f;

    ads1256_data_t ads_data_r;  
    ads1256_data_t ads_data_n2o;

    if(!ads1256_get_data_struct_copy(ADS1256_DEVICE_1, &ads_data_r) || !ads1256_get_data_struct_copy(ADS1256_DEVICE_2, &ads_data_n2o)) {
        ESP_LOGE(TAG, "Failed to get data for ADS1256_DEVICE_1");
        return ESP_FAIL;
    }

    for(int i = 0; i < 4; i++) {
        r_weight += ads_data_r.weight[i];
        n2o_weight += ads_data_n2o.weight[i];
    }
    
    memcpy(resp, &n2o_weight, sizeof(n2o_weight));
    memcpy(&resp[4], &r_weight, sizeof(r_weight));
    esp_err_t err = can_send_message(CAN_SEND_WEIGHTS, resp, sizeof(resp));
    return err;
}

// ##### command handlers #####