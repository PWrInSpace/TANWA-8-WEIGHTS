#include "ads1256.h"
#include "mcu_spi_config.h"
#include "mcu_gpio_config.h"
#include "esp_log.h"
bool ads1256_set_value(uint8_t register_address, uint8_t value)
{
    const uint8_t WREG = 0x50;
    gpio_set_direction(7, GPIO_MODE_OUTPUT);
    uint8_t tx_data1[1] = {WREG | register_address};
    if(!_ads1256_spi_transmit(tx_data1, sizeof(tx_data1), NULL, 0))
    {
        ESP_LOGE("ADS1256", "Failed to set value in ADS1256 register");
        return false;
    }
    uint8_t tx_data2[1] = {0x00}; 
    _ads1256_spi_transmit(tx_data2, sizeof(tx_data2), NULL, 0);
    uint8_t tx_data3[1] = {value}; 
    _ads1256_spi_transmit(tx_data3, sizeof(tx_data3), NULL, 0);
    esp_rom_delay_us(1);
    gpio_set_direction(7, GPIO_MODE_INPUT);
    return true;
}

bool ads1256_init(void)
{
    ads1256_set_value(0x00, STATUS_REGISTER_DEFAULT);
    ads1256_set_value(0x01, MUX_REGISTER_FIRST_CHANNEL2);
    ads1256_set_value(0x02, ADCON_REGISTER);
    ads1256_set_value(0x03, DATA_RATE_REGISTER_100SPS);
    return true;
}
bool ads1256_get_raw_data(ads1256_raw_data_t* data)
{
    uint8_t tx_data[1] = { RDATA_COMMAND};  // Komenda + dummy bajty
    uint8_t rx_data[1] = {0};  // Odbierzemy również 4 bajty
    gpio_set_direction(7, GPIO_MODE_OUTPUT);
    
    if(!_ads1256_spi_transmit(tx_data, sizeof(tx_data), rx_data, sizeof(rx_data)))
    {
        ESP_LOGE("ADS1256", "Failed to read data from ADS1256");
        return false;
    }
    esp_rom_delay_us(7);

    uint8_t tx_data2[3] = {0x00, 0x00, 0x00}; // Dummy bytes to read data
    uint8_t rx_data2[3] = {0};
    if(!_ads1256_spi_transmit(tx_data2, sizeof(tx_data2), rx_data2, sizeof(rx_data2)))
    {
        ESP_LOGE("ADS1256", "Failed to read data from ADS1256");
        return false;
    }
    esp_rom_delay_us(1);

    gpio_set_direction(7, GPIO_MODE_INPUT);
    
    data->channel_1[0] = rx_data2[1];
    data->channel_1[1] = rx_data2[2];
    data->channel_1[2] = rx_data2[3];
    float weight = 0.0f;
    unsigned int value = (rx_data2[1] << 16) | (rx_data2[2] << 8) | rx_data2[3];
    ESP_LOGI("ADS1256", "Value: %f", ((float)value / 8388608.0f) * 39.0625f/(10.0f/300.0f)); // 39.0625f is the scale factor for 24-bit ADC, 10.0f/300.0f is the gain factor
    // weight = value - 15372803; 
    // ESP_LOGI("ADS1256", "Weight: %f", weight);

    return true;
}

bool ads1256_read_id(uint8_t* id)
{
    gpio_set_direction(7, GPIO_MODE_OUTPUT);
    uint8_t tx_data1[2] = {0x12, 0x00};
    uint8_t rx_data1[2] = {0};
    if(!_ads1256_spi_transmit(tx_data1, sizeof(tx_data1), rx_data1, sizeof(rx_data1)))
    {
        ESP_LOGE("ADS1256", "Failed to read ID from ADS1256");
        return false;
    }
    esp_rom_delay_us(7);
    uint8_t txdata2[1] = {0x00}; // Dummy byte to read ID
    uint8_t rx_data2[1] = {0};

    vTaskDelay(pdMS_TO_TICKS(10)); 
    if(!_ads1256_spi_transmit(txdata2, sizeof(txdata2), rx_data2, sizeof(rx_data2)))
    {
        ESP_LOGE("ADS1256", "Failed to read ID from ADS1256");
        return false;
    }
    esp_rom_delay_us(1);
    gpio_set_direction(7, GPIO_MODE_INPUT);
    *id = rx_data2[0];
    ESP_LOGI("ADS1256", "ADS1256 GAIN: %d", *id);
    return true;
}


//zero 15372803
//gdy 1 kg 16374531
//wspolczynnik = 1/(16374531 - 15372803) = 0.00006103515625