#ifndef ADS1256_H
#define ADS1256_H

#include <stdint.h>
#include <stdbool.h>

#define STATUS_REGISTER_DEFAULT 0x00
#define MUX_REGISTER_FIRST_CHANNEL 0x01 // AIN0+ AIN1-
#define MUX_REGISTER_FIRST_CHANNEL2 0x23 // AIN0+ AIN1-
#define ADCON_REGISTER 0x06 // Gain = 64 (max output ~ 10mV) clck off debug off
#define DATA_RATE_REGISTER_100SPS 0x82// 100SPS (samples per second)

//Commands
#define RDATA_COMMAND 0x01 // Read data

typedef struct ads1256_raw_data_t
{
    uint8_t channel_1[3]; 
}ads1256_raw_data_t;


bool ads1256_init(void);

bool ads1256_get_raw_data(ads1256_raw_data_t* data);

bool ads1256_read_id(uint8_t* id);

#endif