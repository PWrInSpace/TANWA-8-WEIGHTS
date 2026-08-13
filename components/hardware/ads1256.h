#ifndef ADS1256_H
#define ADS1256_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

typedef struct {
    int cs_gpio;
    int drdy_gpio;
    int reset_gpio;
    int pwdn_gpio;
} ads1256_pin_config_t;

//Opaque driver handle
typedef struct ads1256_t ads1256_t;

#define STATUS_REGISTER_DEFAULT 0x00

#define MUX_REGISTER_ZERO_CHANNEL 0x01 // AIN0+ AIN1-
#define MUX_REGISTER_FIRST_CHANNEL 0x23 // AIN2+ AIN3- 
#define MUX_REGISTER_SECOND_CHANNEL 0x45  // AIN4+ AIN5-
#define MUX_REGISTER_THIRD_CHANNEL 0x67 // AIN6+ AIN7-

#define ADCON_REGISTER_SETUP 0x06 // Gain = 64 (max output ~ 10mV) clck off debug off

#define DATA_RATE_REGISTER_2P5SPS 0x03 // 2.5SPS
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
#define SYNC_COMMAND 0xFC // Synchronize
#define RESET_COMMAND 0xFE // Reset
#define STANDBY_COMMAND 0xFD // Standby

//registers
#define STATUS_REGISTER 0x00 // Status register
#define MUX_REGISTER 0x01 // Multiplexer register
#define ADCON_REGISTER 0x02 // ADC control register
#define DATA_RATE_REGISTER 0x03 // Data rate register
#define IO_REGISTER 0x04 // IO register
#define OFC0_REGISTER 0x05 // Offset calibration register 0
#define OFC1_REGISTER 0x06 // Offset calibration register 1
#define OFC2_REGISTER 0x07 // Offset calibration register 2
#define FSC0_REGISTER 0x08 // Full-scale calibration register 0
#define FSC1_REGISTER 0x09 // Full-scale calibration register 1
#define FSC2_REGISTER 0x0A // Full-scale calibration register 2


typedef enum {
    CHANNEL_0 = MUX_REGISTER_ZERO_CHANNEL,
    CHANNEL_1 = MUX_REGISTER_FIRST_CHANNEL,
    CHANNEL_2 = MUX_REGISTER_SECOND_CHANNEL,
    CHANNEL_3 = MUX_REGISTER_THIRD_CHANNEL
} ads1256_channel_e;

typedef enum 
{
    SPS_30000 = DATA_RATE_REGISTER_30000SPS,
    SPS_15000 = DATA_RATE_REGISTER_15000SPS,
    SPS_7500 = DATA_RATE_REGISTER_7500SPS,
    SPS_3750 = DATA_RATE_REGISTER_3750SPS,
    SPS_2000 = DATA_RATE_REGISTER_2000SPS,
    SPS_1000 = DATA_RATE_REGISTER_1000SPS,
    SPS_500 = DATA_RATE_REGISTER_500SPS,
    SPS_100 = DATA_RATE_REGISTER_100SPS,
    SPS_50 = DATA_RATE_REGISTER_50SPS,
    SPS_25 = DATA_RATE_REGISTER_25SPS,
    SPS_10 = DATA_RATE_REGISTER_10SPS,
    SPS_5 = DATA_RATE_REGISTER_5SPS,
    SPS_2P5 = DATA_RATE_REGISTER_2P5SPS
} ads1256_sps_e;

float ads1256_sps_hex_to_value(ads1256_sps_e sps);

bool ads1256_single_transmit(ads1256_t* ads,const uint8_t* tx_data, size_t tx_length,uint8_t* rx_data, size_t rx_length);
bool ads1256_set_value(ads1256_t* ads, uint8_t register_address, uint8_t value);
bool ads1256_read_register(ads1256_t* ads, uint8_t register_address, uint8_t* value);
bool ads1256_get_raw_data(ads1256_t* ads, uint8_t* data);
bool ads1256_read_id(ads1256_t* ads, uint8_t* id);
bool ads1256_read_cal_registers(ads1256_t* ads);
bool ads1256_set_sps(ads1256_t* ads, uint8_t sps_register_value);
bool ads1256_set_calibration_registers(ads1256_t* ads,const uint8_t ofc[3],const uint8_t fsc[3]);
bool ads1256_send_command(ads1256_t* ads, uint8_t command);
bool ads1256_send_command_delay(ads1256_t* ads, uint8_t command, uint32_t delay_ms);
const ads1256_pin_config_t* ads1256_get_pin_config(ads1256_t* ads);
ads1256_t* ads1256_create(const ads1256_pin_config_t* pin_config);
void ads1256_destroy(ads1256_t* ads);
#endif
