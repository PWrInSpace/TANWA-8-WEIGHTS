#ifndef ADS1256_H
#define ADS1256_H

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/queue.h"

#define QUEUE_LENGTH 20


#define CS_GPIO_1 15 //TODO: zabrac z configu GPIO
#define CS_GPIO_2 8 //drut TODO usunac
#define RESET_GPIO_1 17
#define RESET_GPIO_2 36
#define PWDN_GPIO_1 16
#define PWDN_GPIO_2 37
#define DRDY_GPIO_1 18
#define DRDY_GPIO_2 35


// #define CS_GPIO_1 12 //TODO: zabrac z configu GPIO
// #define CS_GPIO_2 2
// #define RESET_GPIO_1 5
// #define RESET_GPIO_2 14
// #define PWDN_GPIO_1 13
// #define PWDN_GPIO_2 21

// slot_config.clk = 12;
// slot_config.cmd = 2;
// slot_config.d0  = 5;
// slot_config.d1  = 14;
// slot_config.d2  = 13;
// slot_config.d3  = 21;

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

typedef enum ads1256_device_t
{
    ADS1256_DEVICE_1 = CS_GPIO_1,
    ADS1256_DEVICE_2 = CS_GPIO_2
} ads1256_device_t;

/*
{OFC0, OFC1, OFC2, FSC0, FSC1, FSC2, zero_offset}
*/
extern const uint8_t ads_cal_reg[8][6];
extern const int32_t ads_cal_zero_offset[8];
extern const double ads_cal_factor[8];
extern TaskHandle_t DRDY1_task;
extern TaskHandle_t DRDY2_task;

typedef struct ads1256_raw_data_t
{
    uint8_t channel_0[3];
    uint8_t channel_1[3];
    uint8_t channel_2[3];
}ads1256_raw_data_t;

typedef struct ads1256_sig_data_t
{
    int32_t channel_0;
    int32_t channel_1;
    int32_t channel_2;
}ads1256_sig_data_t;

typedef struct ads1256_channel_t
{
    ads1256_channel_e channel_num; // hexadecimal channel number
    int32_t zero_offset; // Zero offset for the channel
    double factor; // Calibration factor for the channel    
    uint8_t OFC_REG[3]; // Offset calibration registers
    uint8_t FSC_REG[3]; // Full-scale calibration registers
}ads1256_channel_t;

typedef struct ads1256_config_t
{
    ads1256_device_t device; // Device identifier
    ads1256_channel_t* channels; // Channels configuration
    uint8_t active_channel;
    ads1256_sps_e sps; // Samples per second setting
} ads1256_config_t;    

extern QueueHandle_t ads1256_queue_1;

char* ads1256_device_to_string(ads1256_device_t device);
bool ads1256_single_transmit(ads1256_device_t device, const uint8_t* tx_data, size_t tx_length);
bool ads1256_init(ads1256_device_t device);
bool ads1256_get_raw_data(ads1256_device_t device, uint8_t* data);
bool ads1256_read_id(ads1256_device_t device);
bool ads1256_change_channel(ads1256_device_t device, uint8_t channel);
bool ads1256_pins_init(void);
void ads1256_raw_data_to_signed_value(uint8_t* data, int32_t* value);
bool ads1256_sync(ads1256_device_t device);
bool ads1256_wake_up(ads1256_device_t device);
void ads1256_read_data_continuously_test_task(void);
bool ads1256_read_cal_registers(ads1256_device_t device);
bool ads1256_self_cal(ads1256_device_t device);
bool ads1256_reset(ads1256_device_t device);
bool ads1256_set_sps(ads1256_device_t device, uint8_t sps_value);
bool ads1256_set_calibration_registers(ads1256_device_t device, const uint8_t* OFC_REGISTER, const uint8_t* FSC_REGISTER);
void ads1256_raw_mux_data_to_single_weight(uint8_t* data, ads1256_device_t device, float* weight, uint8_t* channel_num, uint8_t channel_count);
void ads1256_raw_data_to_weight(uint8_t* data, ads1256_device_t device, float* weight, uint8_t charnel_num);
#endif
