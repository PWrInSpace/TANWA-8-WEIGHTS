#include "ads1256.h"
#include "mcu_spi_config.h"
#include "mcu_gpio_config.h"
#include "esp_log.h"
#include "driver/gpio.h"

#define TAG "ads1256"

TaskHandle_t DRDY1_task = NULL;
TaskHandle_t DRDY2_task = NULL;
ads1256_channel_t ads1256_channels_dev1[4]  = {
    {CHANNEL_0, 0, 1.0f, {0x3B, 0xF1, 0xFF}, {0x0A, 0x2E, 0x2F}},
    {CHANNEL_1, 1935, -148.9f, {0x2C, 0xF6, 0xFF}, {0xCB, 0xBB, 0x49}}, //hamownia
    {CHANNEL_2, 0, 1.0f, {0x48, 0xF1, 0xFF}, {0x9B, 0x31, 0x2F}},
    {CHANNEL_3, 0, 1.0f, {0x48, 0xF1, 0xFF}, {0x9B, 0x31, 0x2F}}
};

ads1256_channel_t ads1256_channels_dev2[4] = {
    {CHANNEL_0, 0, 1.0f, {0, 0, 0}, {0, 0, 0}},
    {CHANNEL_1, 0, 1.0f, {0, 0, 0}, {0, 0, 0}},
    {CHANNEL_2, 0, 1.0f, {0, 0, 0}, {0, 0, 0}},
    {CHANNEL_3, 0, 1.0f, {0, 0, 0}, {0, 0, 0}}
};


ads1256_config_t ads1256_config_dev1 = {
    .device = ADS1256_DEVICE_1,
    .channels = ads1256_channels_dev1,
    .active_channel = 1,
    .sps = SPS_1000
};

ads1256_config_t ads1256_config_dev2 = {
    .device = ADS1256_DEVICE_2,
    .channels = ads1256_channels_dev2,
    .active_channel = 0,
    .sps = SPS_1000 
};


char* ads1256_device_to_string(ads1256_device_t device) {
    switch (device) {
        case ADS1256_DEVICE_1:
            return "ADS1256_DEVICE_1";
        case ADS1256_DEVICE_2:
            return "ADS1256_DEVICE_2";
        default:
            return "UNKNOWN_DEVICE";
    }
}

bool install_isr_service()
{
    esp_err_t res = gpio_install_isr_service(0);
    if (res != ESP_OK && res != ESP_ERR_INVALID_STATE) {
        ESP_LOGE("ISR", "GPIO ISR service install failed!");
        return false;
    }
    return true; 
}

bool rdy_gpio1_attach_isr(void (*handler)(void*), void* arg) {
    esp_err_t res = gpio_isr_handler_add(DRDY_GPIO_1, handler, arg);
    if (res != ESP_OK) {
        ESP_LOGE("ISR", "Failed to attach ISR to DRDY_GPIO_1!");
        return false;
    }

    ESP_LOGI("ISR", "ISR attached to DRDY_GPIO_1");
    return true;
}

bool rdy_gpio2_attach_isr(void (*handler)(void*), void* arg) {
    esp_err_t res = gpio_isr_handler_add(DRDY_GPIO_2, handler, arg);
    if (res != ESP_OK) {
        ESP_LOGE("ISR", "Failed to attach ISR to DRDY_GPIO_2!");
        return false;
    }

    ESP_LOGI("ISR", "ISR attached to DRDY_GPIO_2");
    return true;
}

void IRAM_ATTR gpio1_isr_handler(void* arg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (DRDY1_task != NULL) {
        xTaskNotifyFromISR(DRDY1_task, 0, eNoAction, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void IRAM_ATTR gpio2_isr_handler(void* arg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (DRDY2_task != NULL) {
        xTaskNotifyFromISR(DRDY2_task, 0, eNoAction, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

bool setup_isr1() {
    if(rdy_gpio1_attach_isr(gpio1_isr_handler, NULL))
    {
        ESP_LOGI(TAG, "GPIO ISR setup successful");
        return true;
    }
    else
    {
        ESP_LOGE(TAG, "GPIO ISR setup failed");
        return false;
    }
}

bool setup_isr2() {
    if(rdy_gpio2_attach_isr(gpio2_isr_handler, NULL))
    {
        ESP_LOGI(TAG, "GPIO ISR setup successful");
        return true;
    }
    else
    {
        ESP_LOGE(TAG, "GPIO ISR setup failed");
        return false;
    }
}

bool ads1256_single_transmit(ads1256_device_t device, const uint8_t* tx_data, size_t tx_length)
{
    if (tx_data == NULL || tx_length == 0) {
        ESP_LOGE(TAG, "Invalid transmit data or length");
        return false;
    }

    if(!_ads1256_spi_transmit(tx_data, tx_length, NULL, 0)) {
        ESP_LOGE(TAG, "Failed to transmit data to ADS1256");
        return false;
    }
    return true;

}

bool ads1256_set_value(uint8_t register_address, uint8_t value, ads1256_device_t device)
{
    bool res = true;
    uint8_t tx[3] = {WREG_COMMAND | register_address, 0x00, value};

    gpio_set_level(device,0);
    res = res && _ads1256_spi_transmit(tx, sizeof(tx), NULL, 0);
    esp_rom_delay_us(1);
    gpio_set_level(device,1);

    return res;
}

bool ads1256_self_cal(ads1256_device_t device)
{
    const uint8_t tx = SELFCAL_COMMAND;
    gpio_set_level(device,0);
    bool res = ads1256_single_transmit(device, &tx, sizeof(tx));
    vTaskDelay(pdMS_TO_TICKS(1000)); 
    gpio_set_level(device,1);

    return res;
}

bool ads1256_sysocal(ads1256_device_t device)
{
    const uint8_t tx = SYSOCAL_COMMAND;
    gpio_set_level(device,0);
    bool res = ads1256_single_transmit(device, &tx, sizeof(tx));
    vTaskDelay(pdMS_TO_TICKS(1000)); 
    gpio_set_level(device,1);
    return res;
}

bool ads1256_sysgcal(ads1256_device_t device)
{
    const uint8_t tx = SYSGCAL_COMMAND;
    gpio_set_level(device,0);
    bool res = ads1256_single_transmit(device, &tx, sizeof(tx));
    vTaskDelay(pdMS_TO_TICKS(1000)); 
    gpio_set_level(device,1);
    return res;
}

bool ads1256_read_register(ads1256_device_t device, uint8_t register_address, uint8_t* value)
{
    bool res = true;
    uint8_t tx_data1[2] = {RREG_COMMAND | register_address, 0x00};
    uint8_t dummy_data = 0x00;

    gpio_set_level(device,0);
    res = res &&_ads1256_spi_transmit(tx_data1, sizeof(tx_data1), NULL, 0);
    esp_rom_delay_us(7);
    res = res &&_ads1256_spi_transmit(&dummy_data, 1, value, 1);
    esp_rom_delay_us(1);
    gpio_set_level(device,1);

    return true;
}

bool ads1256_read_cal_registers(ads1256_device_t device)
{
    uint8_t ofc0, ofc1, ofc2, fsc0, fsc1, fsc2;

    if (!ads1256_read_register(device, OFC0_REGISTER, &ofc0) ||
        !ads1256_read_register(device, OFC1_REGISTER, &ofc1) ||
        !ads1256_read_register(device, OFC2_REGISTER, &ofc2) ||
        !ads1256_read_register(device, FSC0_REGISTER, &fsc0) ||
        !ads1256_read_register(device, FSC1_REGISTER, &fsc1) ||
        !ads1256_read_register(device, FSC2_REGISTER, &fsc2)) {
        ESP_LOGE(TAG, "Failed to read calibration registers");
        return false;
    }

    ESP_LOGI(TAG, "OFC: %02X %02X %02X", ofc0, ofc1, ofc2);
    ESP_LOGI(TAG, "FSC: %02X %02X %02X", fsc0, fsc1, fsc2);

    return true;
}

bool ads1256_reset(ads1256_device_t device)
{
    const uint8_t tx = RESET_COMMAND;

    gpio_set_level(device,0);
    bool result = ads1256_single_transmit(device, &tx, sizeof(tx));
    vTaskDelay(pdMS_TO_TICKS(100)); 
    gpio_set_level(device,1);
    return result;
}

bool ads1256_wake_up(ads1256_device_t device)
{
    const uint8_t tx = WAKEUP_COMMAND;
    gpio_set_level(device, 0); 
    bool result = ads1256_single_transmit(device, &tx, sizeof(tx));
    esp_rom_delay_us(1);
    gpio_set_level(device, 1);
    return result;
}

bool ads1256_sync(ads1256_device_t device)
{
    const uint8_t tx = SYNC_COMMAND;
    gpio_set_level(device, 0);
    bool result = ads1256_single_transmit(device, &tx, sizeof(tx));
    esp_rom_delay_us(1);
    gpio_set_level(device, 1);
    return result;
}

bool ads1256_change_channel(ads1256_device_t device, uint8_t channel)
{
    if( channel > 3) {
        ESP_LOGE(TAG, "Invalid channel number: %d. Must be between 0 and 3.", channel);
        return false;
    }

    ads1256_config_t *config = (device == ADS1256_DEVICE_1) ? &ads1256_config_dev1 : &ads1256_config_dev2;


    bool result = ads1256_set_value(MUX_REGISTER, config->channels[channel].channel_num, device);

    result = result && ads1256_set_calibration_registers(device, config->channels[channel].OFC_REG, config->channels[channel].FSC_REG);
    if(result) { config->active_channel = channel;}
    return result;


}

bool ads1256_set_sps(ads1256_device_t device, uint8_t sps_value)
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

    bool result = ads1256_set_value(DATA_RATE_REGISTER, sps_value, device);
    if (result) {
        ESP_LOGI(TAG, "Data rate set to %d SPS on %s", sps_value, ads1256_device_to_string(device));
    } else {
        ESP_LOGE(TAG, "Failed to set data rate on %s", ads1256_device_to_string(device));
    }
    return result;
}

bool ads1256_set_calibration_registers(ads1256_device_t device, const uint8_t* OFC_REGISTER, const uint8_t* FSC_REGISTER)
{
    if (OFC_REGISTER == NULL || FSC_REGISTER == NULL) {
        ESP_LOGE(TAG, "Invalid calibration registers");
        return false;
    }

    bool result = true;

    uint8_t tx[8] = {WREG_COMMAND | OFC0_REGISTER, 0x05, 
                     OFC_REGISTER[0], OFC_REGISTER[1], OFC_REGISTER[2],
                     FSC_REGISTER[0], FSC_REGISTER[1], FSC_REGISTER[2]};

    gpio_set_level(device, 0);
    result = result && _ads1256_spi_transmit(tx, sizeof(tx), NULL, 0);
    esp_rom_delay_us(1);
    gpio_set_level(device, 1);

    if (!result) {
        ESP_LOGE(TAG, "Failed to set calibration registers on %s", ads1256_device_to_string(device));
    }
    return result;
}

bool ads1256_pins_init(void)
 {
    /*init lokalny gpio output*/
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_num_t output_pins[] = {
        CS_GPIO_1,
        CS_GPIO_2,
        RESET_GPIO_1,
        RESET_GPIO_2,
        PWDN_GPIO_1,
        PWDN_GPIO_2
    };

    for (int i = 0; i < sizeof(output_pins)/sizeof(output_pins[0]); i++) {
        io_conf.pin_bit_mask = 1ULL << output_pins[i];
        gpio_config(&io_conf);
    }

    ESP_ERROR_CHECK(gpio_set_level(CS_GPIO_1, 1));  
    ESP_ERROR_CHECK(gpio_set_level(CS_GPIO_2, 0)); 
    ESP_ERROR_CHECK(gpio_set_level(RESET_GPIO_1, 1)); 
    ESP_ERROR_CHECK(gpio_set_level(RESET_GPIO_2, 0)); 
    ESP_ERROR_CHECK(gpio_set_level(PWDN_GPIO_1, 1)); 
    ESP_ERROR_CHECK(gpio_set_level(PWDN_GPIO_2, 1)); 

    /*init lokalny gpio input*/

    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_NEGEDGE;

    gpio_num_t input_pins[] = {
        DRDY_GPIO_1,
        DRDY_GPIO_2
    };

    for (int i = 0; i < sizeof(input_pins)/sizeof(input_pins[0]); i++) {
        io_conf.pin_bit_mask = 1ULL << input_pins[i];
        gpio_config(&io_conf);
    }

    gpio_intr_enable(DRDY_GPIO_1);
    gpio_intr_enable(DRDY_GPIO_2);

    if(install_isr_service() == false)
    {
        ESP_LOGE("ADS1256", "Failed to install ISR service");
        return false;
    }
    if(setup_isr1() == false)
    {
        ESP_LOGE("ADS1256", "Failed to setup ISR for DRDY_GPIO_1");
        return false;
    }
    if(setup_isr2() == false)
    {
        ESP_LOGE("ADS1256", "Failed to setup ISR for DRDY_GPIO_2");
        return false;
    }
    ESP_LOGI("ADS1256", "ADS1256 GPIO pins initialized successfully");
    return true;
}

bool ads1256_init(ads1256_device_t device)
{
    ads1256_config_t* config = NULL;
    if (device == ADS1256_DEVICE_1) { config = &ads1256_config_dev1; }
    else if (device == ADS1256_DEVICE_2) { config = &ads1256_config_dev2; }
    else {
        ESP_LOGE(TAG, "Invalid device: %s", ads1256_device_to_string(device));
        return false;
    }

    if(ads1256_reset(device))
    {
        ESP_LOGI("ADS1256", "ADS1256 reset successfully");
    }
    else
    {
        ESP_LOGE("ADS1256", "Failed to reset ADS1256");
        return false;
    }
    if(ads1256_set_value(STATUS_REGISTER, STATUS_REGISTER_DEFAULT, device))
    {
        ESP_LOGI("ADS1256", "ADS1256 status register set successfully");
    }
    else
    {
        ESP_LOGE("ADS1256", "Failed to set ADS1256 status register");
        return false;
    }
    if(ads1256_set_value(ADCON_REGISTER, ADCON_REGISTER_SETUP, device))
    {
        ESP_LOGI("ADS1256", "ADS1256 ADCON register set successfully");
    }
    else
    {
        ESP_LOGE("ADS1256", "Failed to set ADS1256 ADCON register");
        return false;
    }
    if(!ads1256_change_channel(device, config->active_channel))
    {
        ESP_LOGE("ADS1256", "Failed to change channel on %s", ads1256_device_to_string(device));
        return false;
    }
    else
    {
        ESP_LOGI("ADS1256", "Channel changed to %d on %s", config->channels[config->active_channel].channel_num, ads1256_device_to_string(device));
    }
    if(!ads1256_set_sps(device, config->sps))
    {
        ESP_LOGE("ADS1256", "Failed to set SPS on %s", ads1256_device_to_string(device));
        return false;
    }
    else
    {
        ESP_LOGI("ADS1256", "SPS set to %d on %s", config->sps, ads1256_device_to_string(device));
    }


    return true;
}

bool ads1256_get_raw_data(ads1256_device_t device, uint8_t* data)
{
    uint8_t tx_data = RDATA_COMMAND; 
    uint8_t dummy_data[3] = {0x00, 0x00, 0x00}; 
    gpio_set_level(device,0);
    
    if(!ads1256_single_transmit(device, &tx_data, sizeof(tx_data)))
    {
        ESP_LOGE("ADS1256", "Failed to send RDATA command to ADS1256");
        return false;
    }

    esp_rom_delay_us(7); 

    if(!_ads1256_spi_transmit(dummy_data, sizeof(dummy_data), data, 3))
    {
        ESP_LOGE("ADS1256", "Failed to read data from ADS1256");
        return false;
    }

    gpio_set_level(device,1);

    return true;
}

bool ads1256_raw_data_to_value(uint8_t* data, double* value, uint8_t channel_num)
{
    if (data == NULL || value == NULL) {
        ESP_LOGE(TAG, "Invalid data or value pointer");
        return false;
    }

    int32_t raw_value = (data[0] << 16) | (data[1] << 8) | data[2];
    if (raw_value & 0x800000) {
        raw_value |= 0xFF000000; // sign-extend if negative
    }

    int32_t diff = raw_value - ads_cal_zero_offset[channel_num]; // zero offset

    *value = (double)diff/ads_cal_factor[channel_num];

    return true;
}

bool ads1256_read_id(ads1256_device_t device)
{
    uint8_t tx_data[2] = {0x12, 0x00};
    uint8_t dummy_data = 0x00;
    uint8_t rx_data = 0x00;
    gpio_set_level(device,0);

    if(!_ads1256_spi_transmit(tx_data, sizeof(tx_data), NULL, 0))
    {
        ESP_LOGE("ADS1256", "Failed to read ID from ADS1256");
        return false;
    }
    esp_rom_delay_us(7);

    if(!_ads1256_spi_transmit(&dummy_data, 1, &rx_data, 1))
    {
        ESP_LOGE("ADS1256", "Failed to read ID from ADS1256");
        return false;
    }

    gpio_set_level(device,1);

    uint8_t id = rx_data >> 4; 
    ESP_LOGI("ADS1256", "ID: %d on %s", id, ads1256_device_to_string(device));
    return true;
}

void ads1256_raw_data_to_signed_value(uint8_t* data, int32_t* value)
{
    if (data == NULL || value == NULL) {
        ESP_LOGE(TAG, "Invalid data or value pointer");
        return;
    }

    *value = (data[0] << 16) | (data[1] << 8) | data[2];
    if (*value & 0x800000) {
        *value |= 0xFF000000; // sign-extend if negative
    }
}
void ads1256_raw_data_to_weight(uint8_t* data, ads1256_device_t device, float* weight, uint8_t charnel_num)
{
    if (data == NULL || weight == NULL) {
        ESP_LOGE(TAG, "Invalid data or weight pointer");
        return;
    }

    int32_t raw_value;
    ads1256_raw_data_to_signed_value(data, &raw_value);
    ads1256_config_t* config = (device == ADS1256_DEVICE_1) ? &ads1256_config_dev1 : &ads1256_config_dev2;
    ads1256_channel_t *channel = &config->channels[charnel_num];

    int32_t diff = raw_value - channel->zero_offset;
    *weight = (float)diff / channel->factor;
}

void ads1256_raw_mux_data_to_single_weight(uint8_t* data, ads1256_device_t device, float* weight, uint8_t* channel_num, uint8_t channel_count)
{
    if (data == NULL || weight == NULL || channel_num == NULL || channel_count == 0) {
        ESP_LOGE(TAG, "Invalid data, weight, channel_num or channel_count pointer");
        return;
    }

    float weight_tab[channel_count];

    // ads1256_config_t* config = (device == ADS1256_DEVICE_1) ? &ads1256_config_dev1 : &ads1256_config_dev2;

    for (int i = 0; i < channel_count; i++) {
        ads1256_raw_data_to_weight(&data[i], device, &weight_tab[i], channel_num[i]);
        *weight += weight_tab[i];
    }
    *weight /= channel_count;
}
