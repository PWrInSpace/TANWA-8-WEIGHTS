#include "ads1256.h"
#include "mcu_spi_config.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>
#define TAG "ads1256"


struct ads1256_t {
    ads1256_pin_config_t pins;
};

float ads1256_sps_hex_to_value(ads1256_sps_e sps) {
    switch (sps) {
        case DATA_RATE_REGISTER_30000SPS:
            return 30000;
        case DATA_RATE_REGISTER_15000SPS:
            return 15000;
        case DATA_RATE_REGISTER_7500SPS:
            return 7500;
        case DATA_RATE_REGISTER_3750SPS:
            return 3750;
        case DATA_RATE_REGISTER_2000SPS:
            return 2000;
        case DATA_RATE_REGISTER_1000SPS:
            return 1000;
        case DATA_RATE_REGISTER_500SPS:
            return 500;
        case DATA_RATE_REGISTER_100SPS:
            return 100;
        case DATA_RATE_REGISTER_50SPS:
            return 50;
        case DATA_RATE_REGISTER_25SPS:
            return 25;
        case DATA_RATE_REGISTER_10SPS:
            return 10;
        case DATA_RATE_REGISTER_5SPS:
            return 5;
        case DATA_RATE_REGISTER_2P5SPS:
            return 2.5;
        default:
            ESP_LOGE(TAG, "Invalid SPS value");
            return -1;
    }
}

bool ads1256_single_transmit(ads1256_t* ads, const uint8_t* tx_data, size_t tx_length, uint8_t* rx_data, size_t rx_length)
{
    if ((tx_data == NULL || tx_length == 0) || (rx_data == NULL && rx_length > 0)) {
        ESP_LOGE(TAG, "Invalid transmit data or length");
        return false;
    }

    ads1256_spi_transmit_t ads_transmit = {
        .tx_data = tx_data,
        .tx_len = tx_length,
        .cs_pin = ads->pins.cs_gpio,
        .rx_enabled = (rx_length > 0)
    };

    if(!_ads1256_spi_transmit(&ads_transmit, rx_data, rx_length)) {
        ESP_LOGE(TAG, "Failed to transmit data to ADS1256");
        return false;
    }
    return true;
}

bool ads1256_set_value(ads1256_t* ads, uint8_t register_address, uint8_t value)
{
    uint8_t tx[3] = {WREG_COMMAND | register_address, 0x00, value};
    bool res = ads1256_single_transmit(ads, tx, sizeof(tx), NULL, 0);
    return res;
}

bool ads1256_read_register(ads1256_t* ads, uint8_t register_address, uint8_t* value)
{
    uint8_t tx_data[2] = {RREG_COMMAND | register_address, 0x00};
    return ads1256_single_transmit(ads, tx_data, sizeof(tx_data), value, 1);
}

bool ads1256_get_raw_data(ads1256_t* ads, uint8_t* data)
{
    const uint8_t tx_data = RDATA_COMMAND;
    return ads1256_single_transmit(ads, &tx_data, sizeof(tx_data), data, 3);
}
bool ads1256_read_id(ads1256_t* ads, uint8_t* id)
{

    if(!ads1256_read_register(ads, STATUS_REGISTER, id))
    {
        return false;
    }

    *id = *id >> 4;
    return true;
}

bool ads1256_read_cal_registers(ads1256_t* ads)
{
    static const uint8_t cal_regs[] = {
        OFC0_REGISTER, OFC1_REGISTER, OFC2_REGISTER,
        FSC0_REGISTER, FSC1_REGISTER, FSC2_REGISTER
    };
    uint8_t values[sizeof(cal_regs)];

    for (size_t i = 0; i < sizeof(cal_regs); i++) {
        if (!ads1256_read_register(ads, cal_regs[i], &values[i])) {
            ESP_LOGE(TAG, "Failed to read calibration register 0x%02X", cal_regs[i]);
            return false;
        }
    }

    ESP_LOGI(TAG, "OFC: %02X %02X %02X", values[0], values[1], values[2]);
    ESP_LOGI(TAG, "FSC: %02X %02X %02X", values[3], values[4], values[5]);

    return true;
}

bool ads1256_set_sps(ads1256_t* ads, uint8_t sps_value)
{
    if (ads1256_sps_hex_to_value(sps_value) < 0) {
        ESP_LOGE(TAG, "Invalid SPS value: 0x%02X", sps_value);
        return false;
    }

    return ads1256_set_value(ads, DATA_RATE_REGISTER, sps_value);
}

bool ads1256_set_calibration_registers(ads1256_t* ads, const uint8_t ofc[3], const uint8_t fsc[3])
{
    if (ofc == NULL || fsc == NULL) {
        ESP_LOGE(TAG, "Invalid calibration registers");
        return false;
    }

    uint8_t tx[8] = {WREG_COMMAND | OFC0_REGISTER, 0x05,
                     ofc[0], ofc[1], ofc[2],
                     fsc[0], fsc[1], fsc[2]};


    bool result = ads1256_single_transmit(ads, tx, sizeof(tx), NULL, 0);

    return result;
}

static uint32_t ads1256_default_delay(uint8_t cmd) {
    switch (cmd) {
        case RESET_COMMAND:    return 100;
        case SELFCAL_COMMAND:
        case SELFOCAL_COMMAND:
        case SELFGCAL_COMMAND:
        case SYSOCAL_COMMAND:
        case SYSGCAL_COMMAND:  return 600;
        default:               return 0; 
    }
}

bool ads1256_send_command(ads1256_t* ads, uint8_t command)
{
    const uint8_t tx = command;
    bool result = ads1256_single_transmit(ads, &tx, sizeof(tx), NULL, 0);

    if (result) {
        uint32_t delay_ms = ads1256_default_delay(command);
        if (delay_ms > 0) {
            vTaskDelay(pdMS_TO_TICKS(delay_ms));
        }
    }

    return result;
}

bool ads1256_send_command_delay(ads1256_t* ads, uint8_t command, uint32_t delay_ms)
{
    const uint8_t tx = command;
    bool result = ads1256_single_transmit(ads, &tx, sizeof(tx), NULL, 0);

    if (result && delay_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }

    return result;
}

const ads1256_pin_config_t* ads1256_get_pin_config(ads1256_t* ads)
{
    return &ads->pins;
}

ads1256_t* ads1256_create(const ads1256_pin_config_t* pin_config)
{
    ads1256_t* ads = malloc(sizeof(ads1256_t));
    if (ads) {
        ads->pins = *pin_config;
    }
    return ads;
}

void ads1256_destroy(ads1256_t* ads)
{
    free(ads);
}