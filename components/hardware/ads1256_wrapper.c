#include "ads1256_wrapper.h"
#include "mcu_spi_config.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "flash.h"
#include <string.h>

#define TAG "ads1256"

struct ads1256_wrapper_t {
    ads1256_t* hal; //Hardware Abstraction Layer, think about the name :/
    ads1256_channel_t channels[4];
    uint8_t active_channel;
    ads1256_sps_e sps;
    ads1256_data_t data;
    bool data_valid;
    SemaphoreHandle_t data_mutex;
    TaskHandle_t drdy_task;
};

static ads1256_channel_t default_channels[4] = {
    {CHANNEL_0, 0, 1.0f, {0x33, 0xF6, 0xFF}, {0xD1, 0xBA, 0x49}},
    {CHANNEL_1, 300, -136.6f, {0x2C, 0xF6, 0xFF}, {0xCB, 0xBB, 0x49}},
    {CHANNEL_2, -5000, -3.01f, {0x2C, 0xF6, 0xFF}, {0xCB, 0xBB, 0x49}},
    {CHANNEL_3, -4570, -2.9833f, {0xCF, 0xFE, 0xFF}, {0x3B, 0xAF, 0x49}}
};

static bool get_channel_config(ads1256_wrapper_t* w, ads1256_channel_t **channel_config, uint8_t channel_num) {
    if (w == NULL || channel_config == NULL || channel_num > 3) {
        ESP_LOGE(TAG, "Invalid channel configuration or channel number");
        return false;
    }

    *channel_config = &w->channels[channel_num];
    return true;
}

static bool get_zero_offset_calibration(ads1256_channel_t* channel_config , int32_t* zero_offset)
{

    if(channel_config == NULL || zero_offset == NULL) {
        ESP_LOGE(TAG, "Invalid channel configuration or zero offset pointer");
        return false;
    }

    *zero_offset = channel_config->zero_offset;

    return true;

}


static bool set_zero_offset_calibration(ads1256_channel_t* channel_config, int32_t zero_offset)
{
    if(channel_config == NULL) {
        ESP_LOGE(TAG, "Invalid channel configuration pointer");
        return false;
    }

    channel_config->zero_offset = zero_offset;

    return true;
}

static bool get_factor_calibration(ads1256_channel_t* channel_config, float* factor)
{
    if(channel_config == NULL || factor == NULL) {
        ESP_LOGE(TAG, "Invalid channel configuration or factor pointer");
        return false;
    }

    *factor = channel_config->factor;

    return true;
}

static bool install_isr_service()
{
    esp_err_t res = gpio_install_isr_service(0);
    if (res != ESP_OK && res != ESP_ERR_INVALID_STATE) {
        ESP_LOGE("ISR", "GPIO ISR service install failed!");
        return false;
    }
    return true;
}

static void IRAM_ATTR gpio_isr_handler(void* arg) {
    ads1256_wrapper_t* w = (ads1256_wrapper_t*)arg;
    TaskHandle_t DRDY_task = w->drdy_task;
    if (DRDY_task == NULL) {
        return;
    }

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR(DRDY_task, 0, eNoAction, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static bool setup_isr(ads1256_wrapper_t* w) {
    esp_err_t res = gpio_isr_handler_add(ads1256_get_pin_config(w->hal)->drdy_gpio, gpio_isr_handler, w);
    if (res != ESP_OK) {
        ESP_LOGE("ISR", "Failed to attach ISR to %d GPIO", ads1256_get_pin_config(w->hal)->drdy_gpio);
        return false;
    }

    ESP_LOGI("ISR", "ISR attached to GPIO %d", ads1256_get_pin_config(w->hal)->drdy_gpio);
    return true;
}

static bool ads1256_pins_init(ads1256_pin_config_t* pin_config)
{
    /*init lokalny gpio output*/
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_num_t output_pins[] = {
        pin_config->cs_gpio,
        pin_config->reset_gpio,
        pin_config->pwdn_gpio
    };

    for (int i = 0; i < sizeof(output_pins)/sizeof(output_pins[0]); i++) {
        io_conf.pin_bit_mask = 1ULL << output_pins[i];
        gpio_config(&io_conf);
    }

    ESP_ERROR_CHECK(gpio_set_level(pin_config->cs_gpio, 1));
    ESP_ERROR_CHECK(gpio_set_level(pin_config->reset_gpio, 1));
    ESP_ERROR_CHECK(gpio_set_level(pin_config->pwdn_gpio, 1));

    /*init lokalny gpio input*/

    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_NEGEDGE;

    io_conf.pin_bit_mask = 1ULL << pin_config->drdy_gpio;
    gpio_config(&io_conf);

    if(!install_isr_service())
    {
        ESP_LOGE("ADS1256", "Failed to install ISR service");
        return false;
    }
    ESP_LOGI("ADS1256", "ADS1256 GPIO pins initialized successfully");
    return true;
}

ads1256_wrapper_t* ads1256_init(ads1256_pin_config_t* pin_config)
{
    ads1256_t* hal = NULL;
    ads1256_wrapper_t* w = NULL;
    bool pins_initialized = false;
    bool isr_registered = false;

    hal = ads1256_create(pin_config);
    if (hal == NULL) { return NULL; }

    w = calloc(1, sizeof(ads1256_wrapper_t));
    if (w == NULL) { ads1256_destroy(hal); return NULL; }

    w->hal = hal;
    memcpy(w->channels, default_channels, sizeof(default_channels));
    w->active_channel = 1;
    w->sps = SPS_1000;
    w->drdy_task = NULL;

    w->data_mutex = xSemaphoreCreateMutex();

    if (w->data_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create ADS data mutex");
        goto cleanup;
    }

    if (!ads1256_pins_init(pin_config)) {
        ESP_LOGE(TAG, "Failed to initialize ADS1256 pins");
        goto cleanup;
    }
    pins_initialized = true;

    if (!setup_isr(w)) {
        ESP_LOGE(TAG, "Failed to setup ISR");
        goto cleanup;
    }
    isr_registered = true;

    if (gpio_intr_enable(pin_config->drdy_gpio) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable ADS1256 DRDY interrupt");
        goto cleanup;
    }

    data_config_t cfg;
    esp_err_t config_result = flash_read(&cfg);

    if (config_result == ESP_ERR_NVS_NOT_INITIALIZED) {
        ESP_LOGE(TAG, "NVS is not initialized");
        goto cleanup;
    }

    if (config_result == ESP_OK) {
        w->channels[0].zero_offset = cfg.weight_cfg.zero_offset_1;
        w->channels[0].factor = cfg.weight_cfg.factor_1;
        w->channels[1].zero_offset = cfg.weight_cfg.zero_offset_2;
        w->channels[1].factor = cfg.weight_cfg.factor_2;
        w->channels[2].zero_offset = cfg.weight_cfg.zero_offset_3;
        w->channels[2].factor = cfg.weight_cfg.factor_3;
        w->channels[3].zero_offset = cfg.weight_cfg.zero_offset_4;
        w->channels[3].factor = cfg.weight_cfg.factor_4;

        ESP_LOGI(TAG, "Loaded weight calibration from NVS");
    } 
    else {
        ESP_LOGW(TAG,"Calibration not available: %s. Using defaults", esp_err_to_name(config_result));
    }

    if (!ads1256_reset(w->hal)) {
        ESP_LOGE("ADS1256", "Failed to reset ADS1256");
        goto cleanup;
    }
    ESP_LOGI("ADS1256", "ADS1256 reset successfully");

    if (!ads1256_set_value(w->hal, STATUS_REGISTER, STATUS_REGISTER_DEFAULT)) {
        ESP_LOGE("ADS1256", "Failed to set ADS1256 status register");
        goto cleanup;
    }
    ESP_LOGI("ADS1256", "ADS1256 status register set successfully");

    if (!ads1256_set_value(w->hal, ADCON_REGISTER, ADCON_REGISTER_SETUP)) {
        ESP_LOGE("ADS1256", "Failed to set ADS1256 ADCON register");
        goto cleanup;
    }
    ESP_LOGI("ADS1256", "ADS1256 ADCON register set successfully");

    if (!ads1256_change_channel(w, w->active_channel)) {
        ESP_LOGE("ADS1256", "Failed to change channel on dev");
        goto cleanup;
    }
    ESP_LOGI("ADS1256", "Channel changed to %d", w->channels[w->active_channel].channel_hex);

    if (!ads1256_set_sps(w->hal, w->sps)) {
        ESP_LOGE("ADS1256", "Failed to set SPS");
        goto cleanup;
    }
    ESP_LOGI("ADS1256", "SPS set to %d", w->sps);

    return w;

cleanup:
    if (w != NULL) {
        if (pins_initialized) {
            gpio_intr_disable(pin_config->drdy_gpio);
        }
        if (isr_registered) {
            gpio_isr_handler_remove(pin_config->drdy_gpio);
        }
        if (w->data_mutex) vSemaphoreDelete(w->data_mutex);
        free(w);
    }
    ads1256_destroy(hal);
    return NULL;
}

void ads1256_deinit(ads1256_wrapper_t* w)
{
    if (w == NULL) {
        return;
    }

    if (w->drdy_task != NULL) {
        ESP_LOGE(
            TAG,
            "Cannot deinit ADS while its task is running");
        return;
    }

    const ads1256_pin_config_t* pins =
        ads1256_get_pin_config(w->hal);

    gpio_intr_disable(pins->drdy_gpio);
    gpio_isr_handler_remove(pins->drdy_gpio);

    if (w->data_mutex != NULL) {
        vSemaphoreDelete(w->data_mutex);
    }

    ads1256_destroy(w->hal);
    free(w);
}

bool ads1256_change_channel(ads1256_wrapper_t* w, uint8_t channel)
{
    if( channel > 3) {
        ESP_LOGE(TAG, "Invalid channel number: %d. Must be between 0 and 3.", channel);
        return false;
    }

    if(w->active_channel == channel) {
        return true;
    }

    bool result = true;

    result &= ads1256_set_value(w->hal, MUX_REGISTER, w->channels[channel].channel_hex);

    if(result) { w->active_channel = channel;}

    result &= ads1256_set_calibration_registers(w->hal, w->channels[channel].OFC_REG, w->channels[channel].FSC_REG);

    return result;
}

bool ads1256_change_channel_and_read(ads1256_wrapper_t* w, uint8_t channel, float* value)
{
    uint8_t raw_data[3] = {0, 0, 0};
    bool res = ads1256_change_channel(w, channel);
    res &= ads1256_sync(w->hal);
    res &= ads1256_wake_up(w->hal);
    res &= ads1256_get_raw_data(w->hal, raw_data);
    res &= ads1256_raw_data_to_value(w, raw_data, value, channel);

    return res;
}

bool ads1256_raw_data_to_value(ads1256_wrapper_t* w, uint8_t* data, float* value, uint8_t channel_num)
{
    if (data == NULL || value == NULL) {
        ESP_LOGE(TAG, "Invalid data or value pointer");
        return false;
    }

    bool res = true;
    ads1256_channel_t *channel = NULL;
    int32_t zero_offset;
    float factor;

    int32_t raw_value = (data[0] << 16) | (data[1] << 8) | data[2];

    if (raw_value & 0x800000) {
        raw_value |= 0xFF000000; 
    }

    res &= get_channel_config(w, &channel, channel_num);
    res &= get_factor_calibration(channel, &factor);
    res &= get_zero_offset_calibration(channel, &zero_offset);

    if (!res) {
        ESP_LOGE(TAG, "Failed to get channel configuration");
        return false;
    }

    int32_t diff = raw_value - zero_offset;
    *value = (float)diff / factor;

    return true;
}

bool ads1256_set_zero_offset(ads1256_wrapper_t* w, int32_t zero_offset, uint8_t channel_num)
{
    if (channel_num > 3) {
        ESP_LOGE(TAG, "Invalid channel number: %d", channel_num);
        return false;
    }

    ads1256_channel_t *channel_config = NULL;
    if (!get_channel_config(w, &channel_config, channel_num)) {
        ESP_LOGE(TAG, "Failed to get channel configuration");
        return false;
    }

    return set_zero_offset_calibration(channel_config, zero_offset);
}

bool ads1256_tare(ads1256_wrapper_t* w)
{
    ads1256_channel_t *channel = NULL;
    if (!get_channel_config(w, &channel, w->active_channel)) {
        ESP_LOGE(TAG, "Failed to get channel configuration");
        return false;
    }

    ads1256_data_t data;
    if (!ads1256_get_data_struct_copy(w, &data)) {
        ESP_LOGE(TAG, "Failed to get data");
        return false;
    }
    int32_t new_zero_offset = (int32_t)(data.weight[w->active_channel] * channel->factor) + channel->zero_offset;
    

    return ads1256_set_zero_offset(w, new_zero_offset, w->active_channel);
}

bool ads1256_tare_all(ads1256_wrapper_t* w){

    ads1256_data_t data;
    if(!ads1256_get_data_struct_copy(w, &data)){
        ESP_LOGE(TAG,"Failed to read data for tare");
        return false;
    }

    for (int ch=0;ch<4;ch++){
        float factor = w->channels[ch].factor;
        int32_t zero = w->channels[ch].zero_offset;
        int32_t new_zero = (int32_t)(data.weight[ch]*factor) +zero;
        w->channels[ch].zero_offset = new_zero;
    }

    data_config_t cfg;
    if (flash_get_runtime_config(&cfg) !=ESP_OK){
        ESP_LOGE(TAG,"Failed to read nvs config");
        return false;
    }

    cfg.weight_cfg.zero_offset_1 = w->channels[0].zero_offset;
    cfg.weight_cfg.zero_offset_2 = w->channels[1].zero_offset;
    cfg.weight_cfg.zero_offset_3 = w->channels[2].zero_offset;
    cfg.weight_cfg.zero_offset_4 = w->channels[3].zero_offset;
    
    if(flash_edit_config(cfg)!=ESP_OK){
        ESP_LOGE(TAG,"Failed to update runtime config");
        return false;
    }

    ESP_LOGI(TAG, "Tare complete");
    ESP_LOGI(TAG, "Saved to RAM only. Use save_flash to persist.");
    return true;

}

bool ads1256_calibrate_channel(ads1256_wrapper_t* w, uint8_t channel, float weight){

    if (channel >3 || weight<=0.0f){
        ESP_LOGE(TAG,"Invalid channel or weight");
        return false;
    }

    ads1256_data_t data;
    if(!ads1256_get_data_struct_copy(w, &data)){
        ESP_LOGE(TAG,"Failed to read data for calubration");
        return false;
    }

    //raw_diff=raw-zero_offset=weight_measured*factor
    float current_factor = w->channels[channel].factor;
    int32_t raw_diff=(int32_t)(data.weight[channel]*current_factor);

    if (raw_diff==0){
        ESP_LOGE(TAG,"Raw diff 0, can't calibrate");
        return false;
    }

    //factor=(raw-zero offset) / known weight
    float new_factor = (float)raw_diff / weight;
    w->channels[channel].factor = new_factor;

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


    ESP_LOGI(TAG, "Calibration complted: channel %d, factor %.6f", channel, new_factor);
    ESP_LOGI(TAG, "Saved to RAM only. Use save_flash to persist.");
    return true;


}

void ads1256_update_data_struct(
    ads1256_wrapper_t* w,
    const ads1256_data_t* samples,
    size_t num_samples)
{
    if (w == NULL || w->data_mutex == NULL ||
        samples == NULL || num_samples == 0) {
        ESP_LOGE(TAG, "Invalid update data arguments");
        return;
    }

    ads1256_data_t averaged = {0};

    for (int channel = 0; channel < 4; channel++) {
        double sum = 0.0;

        for (size_t sample = 0; sample < num_samples; sample++) {
            sum += samples[sample].weight[channel];
        }

        averaged.weight[channel] = (float)(sum / num_samples);
    }

    xSemaphoreTake(w->data_mutex, portMAX_DELAY);
    w->data = averaged;
    w->data_valid = true;
    xSemaphoreGive(w->data_mutex);
}

bool ads1256_get_data_struct_copy(
    ads1256_wrapper_t* w,
    ads1256_data_t* data)
{
    if (w == NULL || data == NULL || w->data_mutex == NULL) {
        ESP_LOGE(TAG, "Invalid data copy arguments");
        return false;
    }

    xSemaphoreTake(w->data_mutex, portMAX_DELAY);

    if (!w->data_valid) {
        xSemaphoreGive(w->data_mutex);
        ESP_LOGW(TAG, "ADS data is not ready");
        return false;
    }

    *data = w->data;

    xSemaphoreGive(w->data_mutex);
    return true;
}

void ads1256_print_data(ads1256_wrapper_t* w)
{
    ads1256_data_t data;

    if (!ads1256_get_data_struct_copy(w, &data)) {
        ESP_LOGE(TAG, "Failed to get data");
        return;
    }

    ESP_LOGI(TAG, "Weight Channel 0: %.2f", data.weight[0]);
    ESP_LOGI(TAG, "Weight Channel 1: %.2f", data.weight[1]);
    ESP_LOGI(TAG, "Weight Channel 2: %.2f", data.weight[2]);
    ESP_LOGI(TAG, "Weight Channel 3: %.2f", data.weight[3]);
}

void ads1256_get_config_info(ads1256_wrapper_t* w)
{
    ESP_LOGI(TAG, "Active Channel: %d", w->active_channel);
    ESP_LOGI(TAG, "Samples per Second: %d", ads1256_sps_hex_to_value(w->sps));

    for (int i = 0; i < 4; i++) {
        ads1256_channel_t* channel = &w->channels[i];
        ESP_LOGI(TAG, "Channel %d: Hex Value: %02X Zero Offset: %d, Factor: %.2f", 
                 i, channel->channel_hex, channel->zero_offset, channel->factor);
        ESP_LOGI(TAG, "OFC_REG: %02X %02X %02X", channel->OFC_REG[0], channel->OFC_REG[1], channel->OFC_REG[2]);
        ESP_LOGI(TAG, "FSC_REG: %02X %02X %02X", channel->FSC_REG[0], channel->FSC_REG[1], channel->FSC_REG[2]);
    }
}

ads1256_t* ads1256_wrapper_get_hal(ads1256_wrapper_t* w)
{
    return w->hal;
}

void ads1256_wrapper_set_drdy_task(ads1256_wrapper_t* w, TaskHandle_t task)
{
    w->drdy_task = task;
}

TaskHandle_t ads1256_wrapper_get_drdy_task(ads1256_wrapper_t* w)
{
    return w->drdy_task;
}