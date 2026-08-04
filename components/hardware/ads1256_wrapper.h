#ifndef ADS1256_WRAPPER_H
#define ADS1256_WRAPPER_H

#include "ads1256.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

typedef struct ads1256_wrapper_t ads1256_wrapper_t;


typedef struct ads1256_channel_t
{
    ads1256_channel_e channel_hex;
    int32_t zero_offset;
    float factor;
    uint8_t OFC_REG[3];
    uint8_t FSC_REG[3];
} ads1256_channel_t;

typedef struct ads1256_data_t
{
    float weight[4];
} ads1256_data_t;

#define ADS1256_UPDATE_DATA_AVG_SAMPLES 25

ads1256_wrapper_t* ads1256_init(ads1256_pin_config_t* pin_config);
void ads1256_deinit(ads1256_wrapper_t* w);

bool ads1256_change_channel(ads1256_wrapper_t* w, uint8_t channel);
bool ads1256_change_channel_and_read(ads1256_wrapper_t* w, uint8_t channel, float* value);

bool ads1256_raw_data_to_value(ads1256_wrapper_t* w, uint8_t* data, float* value, uint8_t channel_num);

bool ads1256_set_zero_offset(ads1256_wrapper_t* w, int32_t zero_offset, uint8_t channel_num);

bool ads1256_tare(ads1256_wrapper_t* w);
bool ads1256_tare_all(ads1256_wrapper_t* w);
bool ads1256_calibrate_channel(ads1256_wrapper_t* w, uint8_t channel, float weight);

bool ads1256_get_data_struct_copy(ads1256_wrapper_t* w, ads1256_data_t* data);
void ads1256_update_data_struct(ads1256_wrapper_t* w, const ads1256_data_t* samples, size_t num_samples);

ads1256_t* ads1256_wrapper_get_dev(ads1256_wrapper_t* w);
bool ads1256_wrapper_set_sps(ads1256_wrapper_t* w, ads1256_sps_e sps);
void ads1256_wrapper_set_drdy_task(ads1256_wrapper_t* w, TaskHandle_t task);

void ads1256_print_data(ads1256_wrapper_t* w);
void ads1256_get_config_info(ads1256_wrapper_t* w);

TaskHandle_t ads1256_wrapper_get_drdy_task(ads1256_wrapper_t* w);
#endif