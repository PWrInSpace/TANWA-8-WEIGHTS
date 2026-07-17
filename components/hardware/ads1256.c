#include "ads1256.h"
#include "mcu_spi_config.h"
#include "esp_log.h"
#include <string.h>

#define TAG "ads1256"


struct ads1256_t {
    ads1256_pin_config_t pins;
};

int ads1256_sps_hex_to_value(ads1256_sps_e sps) {
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
            return 205;
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

bool ads1256_reset(ads1256_t* ads)
{
    const uint8_t tx = RESET_COMMAND;
    bool result = ads1256_single_transmit(ads, &tx, sizeof(tx), NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    return result;
}

bool ads1256_wake_up(ads1256_t* ads)
{
    const uint8_t tx = WAKEUP_COMMAND;
    return ads1256_single_transmit(ads, &tx, sizeof(tx), NULL, 0);
}

bool ads1256_sync(ads1256_t* ads)
{
    const uint8_t tx = SYNC_COMMAND;
    return ads1256_single_transmit(ads, &tx, sizeof(tx), NULL, 0);
}

bool ads1256_self_cal(ads1256_t* ads)
{
    const uint8_t tx = SELFCAL_COMMAND;
    bool res = ads1256_single_transmit(ads, &tx, sizeof(tx), NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(600));
    return res;
}

bool ads1256_sysocal(ads1256_t* ads)
{
    const uint8_t tx = SYSOCAL_COMMAND;
    bool res = ads1256_single_transmit(ads, &tx, sizeof(tx), NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(600));
    return res;
}

bool ads1256_sysgcal(ads1256_t* ads)
{
    const uint8_t tx = SYSGCAL_COMMAND;
    bool res = ads1256_single_transmit(ads, &tx, sizeof(tx), NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(600));
    return res;
}

bool ads1256_get_raw_data(ads1256_t* ads, uint8_t* data)
{
    uint8_t tx_data = RDATA_COMMAND;

    if(!ads1256_single_transmit(ads, &tx_data, sizeof(tx_data), data, 3))
    {
        ESP_LOGE("ADS1256", "Failed to send RDATA command to ADS1256");
        return false;
    }

    return true;
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
    uint8_t ofc0, ofc1, ofc2, fsc0, fsc1, fsc2;

    if (!ads1256_read_register(ads, OFC0_REGISTER, &ofc0) ||
        !ads1256_read_register(ads, OFC1_REGISTER, &ofc1) ||
        !ads1256_read_register(ads, OFC2_REGISTER, &ofc2) ||
        !ads1256_read_register(ads, FSC0_REGISTER, &fsc0) ||
        !ads1256_read_register(ads, FSC1_REGISTER, &fsc1) ||
        !ads1256_read_register(ads, FSC2_REGISTER, &fsc2)) {
        ESP_LOGE(TAG, "Failed to read calibration registers");
        return false;
    }

    ESP_LOGI(TAG, "OFC: %02X %02X %02X", ofc0, ofc1, ofc2);
    ESP_LOGI(TAG, "FSC: %02X %02X %02X", fsc0, fsc1, fsc2);

    return true;
}

bool ads1256_set_sps(ads1256_t* ads, uint8_t sps_value)
{
    if (sps_value != DATA_RATE_REGISTER_30000SPS && sps_value != DATA_RATE_REGISTER_15000SPS &&
        sps_value != DATA_RATE_REGISTER_7500SPS && sps_value != DATA_RATE_REGISTER_3750SPS &&
        sps_value != DATA_RATE_REGISTER_2000SPS && sps_value != DATA_RATE_REGISTER_1000SPS &&
        sps_value != DATA_RATE_REGISTER_500SPS && sps_value != DATA_RATE_REGISTER_100SPS &&
        sps_value != DATA_RATE_REGISTER_50SPS && sps_value != DATA_RATE_REGISTER_25SPS &&
        sps_value != DATA_RATE_REGISTER_10SPS && sps_value != DATA_RATE_REGISTER_5SPS &&
        sps_value != DATA_RATE_REGISTER_2P5SPS) {
        ESP_LOGE(TAG, "Invalid SPS value: %d.", sps_value);

        return false;
    }

    bool result = ads1256_set_value(ads, DATA_RATE_REGISTER, sps_value);
    return result;
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

bool ads1256_start_continuous_read(ads1256_t* ads)
{
    uint8_t tx_data = RDATAC_COMMAND;
    return ads1256_single_transmit(ads, &tx_data, sizeof(tx_data), NULL, 0);
}

bool ads1256_stop_continuous_read(ads1256_t* ads)
{
    uint8_t tx_data = SDATAC_COMMAND;
    return ads1256_single_transmit(ads, &tx_data, sizeof(tx_data), NULL, 0);
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