#ifndef ADS1256_H
#define ADS1256_H

#include <stdint.h>
#include <stdbool.h>

#define CS_GPIO_1 15 //TODO: zabrac z configu GPIO
#define CS_GPIO_2 7

#define STATUS_REGISTER_DEFAULT 0x00
#define MUX_REGISTER_FIRST_CHANNEL 0x01 // AIN0+ AIN1-
#define MUX_REGISTER_FIRST_CHANNEL2 0x23 // AIN0+ AIN1-
#define ADCON_REGISTER 0x06 // Gain = 64 (max output ~ 10mV) clck off debug off
#define DATA_RATE_REGISTER_100SPS 0x82// 100SPS (samples per second)

//Commands
#define RDATA_COMMAND 0x01 // Read data


// typedef struct ads1256_config_t
// {
//     uint8_t status_register; // Default 0x00
//     uint8_t mux_register; // First channel AIN0+ AIN1- 0x01
//     uint8_t adcon_register; // Gain = 64 (max output ~ 10mV) clck off debug off 0x06
//     uint8_t data_rate_register; // 100SPS (samples per second) 0x82
    
// }ads1256_config_t;

// typedef struct ads1256_device_t
// {
//     ads1256_config_t config; // Configuration of ADS1256
//     uint8_t id; // ID of ADS1256

//     bool initialized; // Flag to check if ADS1256 is initialized
//     bool reset; // Flag to check if ADS1256 is reset
// }

typedef struct ads1256_raw_data_t
{
    uint8_t channel_1[3]; 
}ads1256_raw_data_t;


bool ads1256_init(void);

bool ads1256_get_raw_data(ads1256_raw_data_t* data);

bool ads1256_read_id(uint8_t* id);

#endif