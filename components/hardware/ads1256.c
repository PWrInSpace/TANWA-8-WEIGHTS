#include "ads1256.h"
#include "mcu_spi_config.h"
#include "mcu_gpio_config.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include <string.h>
#include "flash.h"

#define TAG "ads1256"

TaskHandle_t DRDY1_task = NULL;
TaskHandle_t DRDY2_task = NULL;

SemaphoreHandle_t data_dev1_mutex = NULL;
SemaphoreHandle_t data_dev2_mutex = NULL;





ads1256_channel_t ads1256_channels_dev1[4]  = { //dzialanie z com xd
    {CHANNEL_0, 0, 1.0f, {0x33, 0xF6, 0xFF}, {0xD1, 0xBA, 0x49}},
    // {CHANNEL_1, 1935, -148.9f, {0x2C, 0xF6, 0xFF}, {0xCB, 0xBB, 0x49}}, //hamownia
    {CHANNEL_1, 0, 1.0f, {0x33, 0xF6, 0xFF}, {0xD1, 0xBA, 0x49}}, //hamownia
    {CHANNEL_2, -5000, -3.01f, {0x2C, 0xF6, 0xFF}, {0xCB, 0xBB, 0x49}}, //matka channel 1(2) xd
    {CHANNEL_3, -4570, -2.9833f, {0xCF, 0xFE, 0xFF}, {0x3B, 0xAF, 0x49}}
};

//(odczyt - a)/b 
// 0 kg --> 13604
// 55.1kg --> 170000  3.085
// 61.1kg --> 189500  3.101
// 

ads1256_channel_t ads1256_channels_dev2[4] = {
    {CHANNEL_0, 0, 1.0f, {0x48, 0xF1, 0xFF}, {0x9B, 0x31, 0x2F}},
    {CHANNEL_1, 0, 1.0f, {0x48, 0xF1, 0xFF}, {0x9B, 0x31, 0x2F}},
    {CHANNEL_2, 0, 1.0f, {0x48, 0xF1, 0xFF}, {0x9B, 0x31, 0x2F}},
    {CHANNEL_3, -3400, -32.5f, {0x33, 0xF6, 0xFF}, {0xD1, 0xBA, 0x49}}
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

ads1256_data_t ads1256_data_dev1;
ads1256_data_t ads1256_data_dev2;

bool get_channel_config(ads1256_config_t* config, ads1256_channel_t* channel_config, uint8_t channel_num) {
    if (channel_config == NULL || channel_num > 3) {
        ESP_LOGE(TAG, "Invalid channel configuration or channel number");
        return false;
    }

    *channel_config = config->channels[channel_num];
    return true;
}

bool get_zero_offset_calibration(ads1256_channel_t* channel_config , int32_t* zero_offset)
{

    if(channel_config == NULL || zero_offset == NULL) {
        ESP_LOGE(TAG, "Invalid channel configuration or zero offset pointer");
        return false;
    }

    *zero_offset = channel_config->zero_offset;

    return true;

}

bool set_zero_offset_calibration(ads1256_channel_t* channel_config, int32_t zero_offset)
{
    if(channel_config == NULL) {
        ESP_LOGE(TAG, "Invalid channel configuration pointer");
        return false;
    }

    channel_config->zero_offset = zero_offset;

    return true;
}

bool get_factor_calibration(ads1256_channel_t* channel_config, float* factor)
{
    if(channel_config == NULL || factor == NULL) {
        ESP_LOGE(TAG, "Invalid channel configuration or factor pointer");
        return false;
    }

    *factor = channel_config->factor;

    return true;
}

bool get_hex_channel_num(ads1256_channel_t* channel_config, uint8_t* channel_num)
{
    if(channel_config == NULL || channel_num == NULL) {
        ESP_LOGE(TAG, "Invalid channel configuration or channel number pointer");
        return false;
    }

    *channel_num = channel_config->channel_hex;

    return true;
}

bool get_active_channel(ads1256_config_t* config, uint8_t* active_channel)
{
    if(config == NULL || active_channel == NULL) {
        ESP_LOGE(TAG, "Invalid configuration or active channel pointer");
        return false;
    }

    *active_channel = config->active_channel;

    return true;
}

int ads1256_device_to_number(ads1256_device_t device) {
    switch (device) {
        case ADS1256_DEVICE_1:
            return 1;
        case ADS1256_DEVICE_2:
            return 2;
        default:
            return -1;
    }
}

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

bool valid_and_set_dev_config(ads1256_device_t device, ads1256_config_t** config) {
    if (device == ADS1256_DEVICE_1) {
        *config = &ads1256_config_dev1;
    } else if (device == ADS1256_DEVICE_2) {
        *config = &ads1256_config_dev2;
    } else {
        ESP_LOGE(TAG, "Invalid device number: %d", device);
        return false;
    }
    return true;
}

bool ads1256_set_zero_offset(ads1256_device_t device, int32_t zero_offset, uint8_t channel_num)
{
    if (channel_num > 3) {
        ESP_LOGE(TAG, "Invalid channel number: %d", channel_num);
        return false;
    }

    ads1256_config_t* config;
    if (!valid_and_set_dev_config(device, &config)) {
        ESP_LOGE(TAG, "Invalid device number: %d", ads1256_device_to_number(device));
        return false;
    }

    ads1256_channel_t channel_config;
    if (!get_channel_config(config, &channel_config, channel_num)) {
        ESP_LOGE(TAG, "Failed to get channel configuration");
        return false;
    }

    return set_zero_offset_calibration(&channel_config, zero_offset);
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

bool rdy_gpio_attach_isr(void (*handler)(void*), void* arg) {
    uint8_t ads_dev = (uint8_t)(uintptr_t)arg;
    uint8_t drdy_gpio;
    if(ads_dev == ADS1256_DEVICE_1) {
        drdy_gpio = DRDY_GPIO_1;
    } else if(ads_dev == ADS1256_DEVICE_2) {
        drdy_gpio = DRDY_GPIO_2;
    } else {
        ESP_LOGE("ISR", "Invalid GPIO number for ADS1256 device");
        return false;
    }

    esp_err_t res = gpio_isr_handler_add(drdy_gpio, handler, arg);
    if (res != ESP_OK) {
        ESP_LOGE("ISR", "Failed to attach ISR to %d GPIO", drdy_gpio);
        return false;
    }

    ESP_LOGI("ISR", "ISR attached to GPIO %d for device %d", drdy_gpio, ads1256_device_to_number(ads_dev));
    return true;
}

void IRAM_ATTR gpio_isr_handler(void* arg) {
    ads1256_device_t device = (ads1256_device_t)arg;
    TaskHandle_t DRDY_task = (device == ADS1256_DEVICE_1) ? DRDY1_task : DRDY2_task;
    if (DRDY_task == NULL) {
        return;
    }

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR(DRDY_task, 0, eNoAction, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

bool setup_isr() {
    if(!rdy_gpio_attach_isr(gpio_isr_handler, (void*)ADS1256_DEVICE_1))
    {
        ESP_LOGI(TAG, "GPIO ISR setup failed for device %d", ads1256_device_to_number(ADS1256_DEVICE_1));
        return false;
    }

    if(!rdy_gpio_attach_isr(gpio_isr_handler, (void*)ADS1256_DEVICE_2))
    {
        ESP_LOGI(TAG, "GPIO ISR setup failed for device %d", ads1256_device_to_number(ADS1256_DEVICE_2));
        return false;
    }

    return true;
}


bool ads1256_single_transmit(ads1256_device_t device, const uint8_t* tx_data, size_t tx_length, uint8_t* rx_data, size_t rx_length)
{
    if ((tx_data == NULL || tx_length == 0) || (rx_data == NULL && rx_length > 0)) {
        ESP_LOGE(TAG, "Invalid transmit data or length");
        return false;
    }

    ads1256_spi_transmit_t ads_transmit = {
        .tx_data = tx_data,
        .tx_len = tx_length,
        .cs_pin = (device == ADS1256_DEVICE_1) ? CS_GPIO_1 : CS_GPIO_2,
        .rx_enabled = (rx_length > 0)
    };

    if(!_ads1256_spi_transmit(&ads_transmit, rx_data, rx_length)) {
        ESP_LOGE(TAG, "Failed to transmit data to ADS1256");
        return false;
    }
    return true;
}

bool ads1256_set_value(uint8_t register_address, uint8_t value, ads1256_device_t device)
{
    uint8_t tx[3] = {WREG_COMMAND | register_address, 0x00, value};
    bool res = ads1256_single_transmit(device, tx, sizeof(tx), NULL, 0);
    return res;

}

bool ads1256_self_cal(ads1256_device_t device)
{
    const uint8_t tx = SELFCAL_COMMAND;
    bool res = ads1256_single_transmit(device, &tx, sizeof(tx), NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(600)); 
    return res;
}

bool ads1256_sysocal(ads1256_device_t device)
{
    const uint8_t tx = SYSOCAL_COMMAND;
    bool res = ads1256_single_transmit(device, &tx, sizeof(tx), NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(600)); 
    return res;
}

bool ads1256_sysgcal(ads1256_device_t device)
{
    const uint8_t tx = SYSGCAL_COMMAND;
    bool res = ads1256_single_transmit(device, &tx, sizeof(tx), NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(600)); 
    return res;
}

bool ads1256_read_register(ads1256_device_t device, uint8_t register_address, uint8_t* value)
{
    uint8_t tx_data[2] = {RREG_COMMAND | register_address, 0x00};
    return ads1256_single_transmit(device, tx_data, sizeof(tx_data), value, 1);
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
    bool result = ads1256_single_transmit(device, &tx, sizeof(tx), NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(100)); 
    return result;
}

bool ads1256_wake_up(ads1256_device_t device)
{
    const uint8_t tx = WAKEUP_COMMAND;
    return ads1256_single_transmit(device, &tx, sizeof(tx), NULL, 0);
}

bool ads1256_sync(ads1256_device_t device)
{
    const uint8_t tx = SYNC_COMMAND;
    return ads1256_single_transmit(device, &tx, sizeof(tx), NULL, 0);
}

bool ads1256_change_channel(ads1256_device_t device, uint8_t channel)
{
    if( channel > 3) {
        ESP_LOGE(TAG, "Invalid channel number: %d. Must be between 0 and 3.", channel);
        return false;
    }

    ads1256_config_t* config;
    if(!valid_and_set_dev_config(device, &config)) {
        return false;
    }

    if(config->active_channel == channel) {
        return true;
    }

    bool result = true;

    result &= ads1256_set_value(MUX_REGISTER, config->channels[channel].channel_hex, device);

    if(result) { config->active_channel = channel;}

    result &= ads1256_set_calibration_registers(device, config->channels[channel].OFC_REG, config->channels[channel].FSC_REG);

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
    return result;
}

bool ads1256_set_calibration_registers(ads1256_device_t device, const uint8_t* OFC_REGISTER, const uint8_t* FSC_REGISTER)
{
    if (OFC_REGISTER == NULL || FSC_REGISTER == NULL) {
        ESP_LOGE(TAG, "Invalid calibration registers");
        return false;
    }

    uint8_t tx[8] = {WREG_COMMAND | OFC0_REGISTER, 0x05, 
                     OFC_REGISTER[0], OFC_REGISTER[1], OFC_REGISTER[2],
                     FSC_REGISTER[0], FSC_REGISTER[1], FSC_REGISTER[2]};


    bool result = ads1256_single_transmit(device, tx, sizeof(tx), NULL, 0);

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
    ESP_ERROR_CHECK(gpio_set_level(CS_GPIO_2, 1)); 
    ESP_ERROR_CHECK(gpio_set_level(RESET_GPIO_1, 1)); 
    ESP_ERROR_CHECK(gpio_set_level(RESET_GPIO_2, 0)); //TODO gdzy bedzie nowa plytka to na 1
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

    if(!install_isr_service())
    {
        ESP_LOGE("ADS1256", "Failed to install ISR service");
        return false;
    }
    if(!setup_isr())
    {
        ESP_LOGE("ADS1256", "Failed to setup ISR");
        return false;
    }
    ESP_LOGI("ADS1256", "ADS1256 GPIO pins initialized successfully");
    return true;
}

bool ads1256_init(ads1256_device_t device)
{
    ads1256_config_t* config;
    data_dev1_mutex = xSemaphoreCreateMutex();
    data_dev2_mutex = xSemaphoreCreateMutex();


    if(!valid_and_set_dev_config(device, &config))
    {
        ESP_LOGE("ADS1256", "Invalid device configuration for device %d", ads1256_device_to_number(device));
        return false;
    }

    data_config_t cfg;
    if (flash_read(&cfg) == ESP_OK) {
        if(device == ADS1256_DEVICE_1) {
            ads1256_channels_dev1[0].zero_offset = cfg.weight_cfg.zero_offset_1;
            ads1256_channels_dev1[0].factor      = cfg.weight_cfg.factor_1;
            
            ads1256_channels_dev1[1].zero_offset = cfg.weight_cfg.zero_offset_2;
            ads1256_channels_dev1[1].factor      = cfg.weight_cfg.factor_2;
            
            ads1256_channels_dev1[2].zero_offset = cfg.weight_cfg.zero_offset_3;
            ads1256_channels_dev1[2].factor      = cfg.weight_cfg.factor_3;
            
            ads1256_channels_dev1[3].zero_offset = cfg.weight_cfg.zero_offset_4;
            ads1256_channels_dev1[3].factor      = cfg.weight_cfg.factor_4;
            ESP_LOGI("ADS1256", "Loaded weight calibration from NVS");
        }
    } else {
        ESP_LOGE("ADS1256", "Failed to load weight calibration from NVS, using defaults!");
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
        ESP_LOGE("ADS1256", "Failed to change channel on dev: %d", ads1256_device_to_number(device));
        return false;
    }
    else
    {
        ESP_LOGI("ADS1256", "Channel changed to %d on dev: %d", config->channels[config->active_channel].channel_hex, ads1256_device_to_number(device));
    }

    if(!ads1256_set_sps(device, config->sps))
    {
        ESP_LOGE("ADS1256", "Failed to set SPS on dev: %d", ads1256_device_to_number(device));
        return false;
    }
    else
    {
        ESP_LOGI("ADS1256", "SPS set to %d on dev: %d", config->sps, ads1256_device_to_number(device));
    }

    return true;
}

bool ads1256_get_raw_data(ads1256_device_t device, uint8_t* data)
{
    uint8_t tx_data = RDATA_COMMAND; 
    
    if(!ads1256_single_transmit(device, &tx_data, sizeof(tx_data), data, 3))
    {
        ESP_LOGE("ADS1256", "Failed to send RDATA command to ADS1256");
        return false;
    }

    return true;
}

bool ads1256_raw_data_to_value(ads1256_device_t dev, uint8_t* data, float* value, uint8_t channel_num)
{
    if (data == NULL || value == NULL) {
        ESP_LOGE(TAG, "Invalid data or value pointer");
        return false;
    }

    ads1256_config_t* config;

    if (!valid_and_set_dev_config(dev, &config)) {
        ESP_LOGE(TAG, "Invalid device configuration for device %d", ads1256_device_to_number(dev));
        return false;
    }

    bool res = true;
    ads1256_channel_t channel;
    int32_t zero_offset;
    float factor;

    int32_t raw_value = (data[0] << 16) | (data[1] << 8) | data[2];

    if (raw_value & 0x800000) {
        raw_value |= 0xFF000000; 
    }

    res &= get_channel_config(config, &channel, channel_num);
    res &= get_factor_calibration(&channel, &factor);
    res &= get_zero_offset_calibration(&channel, &zero_offset);

    if (!res) {
        ESP_LOGE(TAG, "Failed to get channel configuration for device %d", ads1256_device_to_number(dev));
        return false;
    }

    int32_t diff = raw_value - zero_offset;
    *value = (float)diff / factor;

    return true;
}

bool ads1256_read_id(ads1256_device_t device, uint8_t* id)
{
    
    if(!ads1256_read_register(device, STATUS_REGISTER, id))
    {
        return false;
    }

    *id = *id >> 4; 
    return true;
}

bool ads1256_change_channel_and_read(ads1256_device_t device, uint8_t channel, float* value)
{
    uint8_t raw_data[3] = {0, 0, 0};
    bool res = ads1256_change_channel(device, channel);
    res &= ads1256_sync(device);
    res &= ads1256_wake_up(device);
    res &= ads1256_get_raw_data(device, raw_data);
    res &= ads1256_raw_data_to_value(device, raw_data, value, channel);

    return res;
}

void ads1256_get_config_info(ads1256_device_t device)
{
    ads1256_config_t* config;
    if (!valid_and_set_dev_config(device, &config)) {
        return;
    }

    ESP_LOGI(TAG, "Device: %d", ads1256_device_to_number(device));
    ESP_LOGI(TAG, "Active Channel: %d", config->active_channel);
    ESP_LOGI(TAG, "Samples per Second: %d", ads1256_sps_hex_to_value(config->sps));

    for (int i = 0; i < 4; i++) {
        ads1256_channel_t* channel = &config->channels[i];
        ESP_LOGI(TAG, "Channel %d: Hex Value: %02X Zero Offset: %d, Factor: %.2f", 
                 i, channel->channel_hex, channel->zero_offset, channel->factor);
        ESP_LOGI(TAG, "OFC_REG: %02X %02X %02X", channel->OFC_REG[0], channel->OFC_REG[1], channel->OFC_REG[2]);
        ESP_LOGI(TAG, "FSC_REG: %02X %02X %02X", channel->FSC_REG[0], channel->FSC_REG[1], channel->FSC_REG[2]);
    }
}

void ads1256_update_data_struct(ads1256_device_t device, const ads1256_data_t* samples, size_t num_samples)
{
    if (samples == NULL || num_samples == 0) {
        ESP_LOGE(TAG, "Invalid samples or count");
        return;
    }

    ads1256_data_t averaged;
    for (int ch = 0; ch < 4; ch++) {
        double sum = 0.0;
        for (size_t i = 0; i < num_samples; i++) {
            sum += (double)samples[i].weight[ch];
        }
        averaged.weight[ch] = (float)(sum / (double)num_samples);
    }

    if(device == ADS1256_DEVICE_1) {
        xSemaphoreTake(data_dev1_mutex, portMAX_DELAY);
        memcpy(ads1256_data_dev1.weight, averaged.weight, sizeof(averaged.weight));
        xSemaphoreGive(data_dev1_mutex);
    } else if(device == ADS1256_DEVICE_2) {
        xSemaphoreTake(data_dev2_mutex, portMAX_DELAY);
        memcpy(ads1256_data_dev2.weight, averaged.weight, sizeof(averaged.weight));
        xSemaphoreGive(data_dev2_mutex);
    } else {
        ESP_LOGE(TAG, "Invalid device number: %d", ads1256_device_to_number(device));
    }
}

bool ads1256_get_data_struct_copy(ads1256_device_t device, ads1256_data_t* data)
{
    if(data == NULL) {
        ESP_LOGE(TAG, "Invalid data pointer");
        return false;
    }

    if (device == ADS1256_DEVICE_1) {
        if (data_dev1_mutex == NULL) {
            ESP_LOGW(TAG, "ADS data mutex not ready");
            return false;
        }
        xSemaphoreTake(data_dev1_mutex, portMAX_DELAY);
        memcpy(data, &ads1256_data_dev1, sizeof(ads1256_data_t));
        xSemaphoreGive(data_dev1_mutex);
    } else if (device == ADS1256_DEVICE_2) {
        if (data_dev2_mutex == NULL) {
            ESP_LOGW(TAG, "ADS data mutex not ready");
            return false;
        }
        xSemaphoreTake(data_dev2_mutex, portMAX_DELAY);
        memcpy(data, &ads1256_data_dev2, sizeof(ads1256_data_t));
        xSemaphoreGive(data_dev2_mutex);
    } else {
        ESP_LOGE(TAG, "Invalid device number: %d", ads1256_device_to_number(device));
        return false;
    }

    return true;
}

void ads1256_print_data(ads1256_device_t device)
{
    ads1256_data_t data;

    if (!ads1256_get_data_struct_copy(device, &data)) {
        ESP_LOGE(TAG, "Failed to get data for device %d", ads1256_device_to_number(device));
        return;
    }

    ESP_LOGI(TAG, "Device: %d", ads1256_device_to_number(device));
    ESP_LOGI(TAG, "Weight Channel 0: %.2f", data.weight[0]);
    ESP_LOGI(TAG, "Weight Channel 1: %.2f", data.weight[1]);
    ESP_LOGI(TAG, "Weight Channel 2: %.2f", data.weight[2]);
    ESP_LOGI(TAG, "Weight Channel 3: %.2f", data.weight[3]);
}

bool ads1256_start_continuous_read(ads1256_device_t device)
{
    uint8_t tx_data = RDATAC_COMMAND;
    return ads1256_single_transmit(device, &tx_data, sizeof(tx_data), NULL, 0);
}

bool ads1256_stop_continuous_read(ads1256_device_t device)
{
    uint8_t tx_data = SDATAC_COMMAND;
    return ads1256_single_transmit(device, &tx_data, sizeof(tx_data), NULL, 0);
}


bool ads1256_tare(ads1256_device_t device)
{
    ads1256_config_t* config;
    if (!valid_and_set_dev_config(device, &config)) {
        ESP_LOGE(TAG, "Invalid device configuration for device %d", ads1256_device_to_number(device));
        return false;
    }

    ads1256_channel_t channel;
    if (!get_channel_config(config, &channel, config->active_channel)) {
        ESP_LOGE(TAG, "Failed to get channel configuration for device %d", ads1256_device_to_number(device));
        return false;
    }

    ads1256_data_t data;
    if (!ads1256_get_data_struct_copy(device, &data)) {
        ESP_LOGE(TAG, "Failed to get data for device %d", ads1256_device_to_number(device));
        return false;
    }
    int32_t new_zero_offset = (int32_t)(data.weight[config->active_channel] * channel.factor) + channel.zero_offset;
    

    return ads1256_set_zero_offset(device, new_zero_offset, config->active_channel);
}

bool ads1256_tare_channel(ads1256_device_t device, uint8_t channel)
{
    if (device != ADS1256_DEVICE_1) {
        ESP_LOGE(TAG, "tare supports only DEV1");
        return false;
    }

    if (channel > 3) {
        ESP_LOGE(TAG, "Invalid channel number: %d", channel);
        return false;
    }

    ads1256_config_t* config;
    if (!valid_and_set_dev_config(device, &config)) {
        ESP_LOGE(TAG, "Invalid device config for device %d", ads1256_device_to_number(device));
        return false;
    }

    ads1256_data_t data;
    if (!ads1256_get_data_struct_copy(device, &data)) {
        ESP_LOGE(TAG, "Failed to read data for tare");
        return false;
    }

    float factor = config->channels[channel].factor;
    int32_t zero = config->channels[channel].zero_offset;
    int32_t new_zero = (int32_t)(data.weight[channel] * factor) + zero;
    config->channels[channel].zero_offset = new_zero;

    data_config_t cfg;
    if (flash_get_runtime_config(&cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read nvs config");
        return false;
    }

    switch (channel) {
        case 0: cfg.weight_cfg.zero_offset_1 = new_zero; break;
        case 1: cfg.weight_cfg.zero_offset_2 = new_zero; break;
        case 2: cfg.weight_cfg.zero_offset_3 = new_zero; break;
        case 3: cfg.weight_cfg.zero_offset_4 = new_zero; break;
        default: return false;
    }

    if (flash_edit_config(cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to update runtime config");
        return false;
    }

    ESP_LOGI(TAG, "Tare channel %d complete, new zero_offset=%d. Remember to use `flash_save_config` to save your changes\n", channel, (int)new_zero);
    return true;
}

bool ads1256_tare_all(ads1256_device_t device){
    if(device != ADS1256_DEVICE_1){
        ESP_LOGE(TAG, "tare supports only DEV1");
        return false;
    }

    ads1256_config_t* config;
    if(!valid_and_set_dev_config(device,&config)){
        ESP_LOGE(TAG,"Invalid device config for device %d",ads1256_device_to_number(device));
        return false;
    }

    ads1256_data_t data;
    if(!ads1256_get_data_struct_copy(device, &data)){
        ESP_LOGE(TAG,"Failed to read data for tare");
        return false;
    }

    for (int ch=0;ch<4;ch++){
        float factor = config->channels[ch].factor;
        int32_t zero = config->channels[ch].zero_offset;
        int32_t new_zero = (int32_t)(data.weight[ch]*factor) +zero;
        config->channels[ch].zero_offset = new_zero;
    }

    data_config_t cfg;
    if (flash_get_runtime_config(&cfg) !=ESP_OK){
        ESP_LOGE(TAG,"Failed to read nvs config");
        return false;
    }

    cfg.weight_cfg.zero_offset_1 = config->channels[0].zero_offset;
    cfg.weight_cfg.zero_offset_2 = config->channels[1].zero_offset;
    cfg.weight_cfg.zero_offset_3 = config->channels[2].zero_offset;
    cfg.weight_cfg.zero_offset_4 = config->channels[3].zero_offset;
    
    if(flash_edit_config(cfg)!=ESP_OK){
        ESP_LOGE(TAG,"Failed to update runtime config");
        return false;
    }

    ESP_LOGI(TAG, "Tare complete. Remember to use `flash_save_config` to save your changes\n");
    return true;

}


bool ads1256_calibrate_channel(ads1256_device_t device, uint8_t channel, float weight){
    if (device!=ADS1256_DEVICE_1){
        ESP_LOGE(TAG,"calibrate supports only for DEV1");
        return false;
    }

    if (channel >3 || weight<=0.0f){
        ESP_LOGE(TAG,"Invalid channel or weight");
        return false;
    }

    ads1256_config_t* config;
    if(!valid_and_set_dev_config(device,&config)){
        ESP_LOGE(TAG,"Invalid device config for device %d",ads1256_device_to_number(device));
        return false;
    }

    ads1256_data_t data;
    if(!ads1256_get_data_struct_copy(device,&data)){
        ESP_LOGE(TAG,"Failed to read data for calubration");
        return false;
    }

    //raw_diff=raw-zero_offset=weight_measured*factor
    float current_factor = config->channels[channel].factor;
    int32_t raw_diff=(int32_t)(data.weight[channel]*current_factor);

    if (raw_diff==0){
        ESP_LOGE(TAG,"Raw diff 0, can't calibrate");
        return false;
    }

    //factor=(raw-zero offset) / known weight
    float new_factor = (float)raw_diff / weight;
    config->channels[channel].factor = new_factor;

    data_config_t cfg;
    if(flash_get_runtime_config(&cfg)!=ESP_OK){
        ESP_LOGE(TAG,"Failed to read nvs config");
        return false;
    }

    switch(channel){
        case 0: cfg.weight_cfg.factor_1 = new_factor; break;
        case 1: cfg.weight_cfg.factor_2 = new_factor; break;
        case 2: cfg.weight_cfg.factor_3 = new_factor; break;
        case 3: cfg.weight_cfg.factor_4 = new_factor; break;
        default: return false;
    }

    if(flash_edit_config(cfg) !=ESP_OK){
        ESP_LOGE(TAG,"Failed to update runtime config");
        return false;
    }

    ESP_LOGI(TAG, "Calibration complted: channel %d, factor %.6f. Remember to use `flash_save_config` to save your changes\n", channel, new_factor);
    return true;
}
