#ifndef ADS1256_H
#define ADS1256_H

#include <stdint.h>
#include <stdbool.h>

#define CS_GPIO_1 15 //TODO: zabrac z configu GPIO
#define CS_GPIO_2 7
#define DRDY_GPIO_1 18
#define DRDY_GPIO_2 35

#define STATUS_REGISTER_DEFAULT 0x00

#define MUX_REGISTER_FIRST_CHANNEL 0x01 // AIN0+ AIN1-
#define MUX_REGISTER_SECOND_CHANNEL 0x23 // AIN2+ AIN3- 
#define MUX_REGISTER_THIRD_CHANNEL 0x45  // AIN4+ AIN5-
#define MUX_REGISTER_FOURTH_CHANNEL 0x67 // AIN6+ AIN7-

#define ADCON_REGISTER 0x06 // Gain = 64 (max output ~ 10mV) clck off debug off

#define DATA_RATE_REGISTER_5SPS 0x13 // 5SPS (samples per second)
#define DATA_RATE_REGISTER_10SPS 0x23 // 10SPS 
#define DATA_RATE_REGISTER_25SPS 0x43 // 25SPS 
#define DATA_RATE_REGISTER_50SPS 0x63 // 50SPS
#define DATA_RATE_REGISTER_100SPS 0x82// 100SPS 
#define DATA_RATE_REGISTER_500SPS 0x92// 100SPS
#define DATA_RATE_REGISTER_1000SPS 0xA1// 100SPS 
#define DATA_RATE_REGISTER_2000SPS 0xB0// 100SPS 
#define DATA_RATE_REGISTER_3750SPS 0xC0// 100SPS
#define DATA_RATE_REGISTER_7500SPS 0xD0// 100SPS 
#define DATA_RATE_REGISTER_15000SPS 0xE0// 100SPS 
#define DATA_RATE_REGISTER_30000SPS 0xF0// 100SPS 

//Commands
#define WAKEUP_COMMAND 0x00 // Wake up
#define RDATA_COMMAND 0x01 // Read data
#define RDATAC_COMMAND 0x03 // Read data continuously
#define SDATAC_COMMAND 0x0F // Stop reading data continuously
#define RREG_COMMAND 0x10 // Read register
#define WREG_COMMAND 0x50 // Write register
#define SELFCAL_COMMAND 0xF0 // Self-calibration
#define SELFOCAL_COMMAND 0xF1 // Self-offset calibration
#define SELFGCAL_COMMAND 0xF2 // Self-gain calibration
#define SYSOCAL_COMMAND 0xF3 // System offset calibration
#define SYSGCAL_COMMAND 0xF4 // System gain calibration
#define RESET_COMMAND 0xFE // Reset
#define STANDBY_COMMAND 0xFD // Standby


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
typedef enum ads1256_device_t
{
    ADS1256_DEVICE_1 = CS_GPIO_1,
    ADS1256_DEVICE_2 = CS_GPIO_2
} ads1256_device_t;

typedef struct ads1256_raw_data_t
{
    uint8_t channel_1[3]; 
}ads1256_raw_data_t;


bool ads1256_init(ads1256_device_t device);

bool ads1256_get_raw_data(ads1256_raw_data_t* data);

bool ads1256_read_id(ads1256_device_t device);

bool ads1256_pins_init(void);
bool ads1256_read_id2();
#endif