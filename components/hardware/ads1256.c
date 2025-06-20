#include "ads1256.h"
#include "mcu_spi_config.h"
#include "mcu_gpio_config.h"
#include "esp_log.h"
#include "driver/gpio.h"

#define TAG "ads1256"

TaskHandle_t DRDY1_task = NULL;
TaskHandle_t DRDY2_task = NULL;
static ads1256_device_t device_copy;
#define BUFFER_SIZE (60000) // 1 MB / sizeof(int32_t) = 262144 próbek

static int32_t value_buffer[BUFFER_SIZE] = {2137};
static size_t buffer_index = 0;


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

// void isr_rdy1_loop(void*  pvParameters)
// {
//     while (1)
//     {
//         //TODO: Implementacja handling ADS1256 data ready interrupt na RDY_GPIO_1
//         ulTaskNotifyTake(pdTRUE, portMAX_DELAY); 
//     }
// }

// void isr_rdy2_loop(void*  pvParameters)
// {
//     while (1)
//     {
//         //TODO: Implementacja handling ADS1256 data ready interrupt na RDY_GPIO_2
//         ulTaskNotifyTake(pdTRUE, portMAX_DELAY); 
//     }
// }

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
    gpio_set_level(device,0);
    uint8_t tx[3] = {WREG_COMMAND | register_address, 0x00, value};
    res = res && ads1256_single_transmit(device, &tx[0], sizeof(tx));
    res = res && ads1256_single_transmit(device, &tx[1], sizeof(tx));
    res = res && ads1256_single_transmit(device, &tx[2], sizeof(tx));
    esp_rom_delay_us(2); 
    gpio_set_level(device,1);

    return res;
}

bool ads1256_self_cal(ads1256_device_t device)
{
    const uint8_t tx = SELFCAL_COMMAND;
    gpio_set_level(device,0);
    bool res = ads1256_single_transmit(device, &tx, sizeof(tx));
    esp_rom_delay_us(2);
    gpio_set_level(device,1);

    return res;
}


bool ads1256_reset(ads1256_device_t device)
{
    const uint8_t tx = RESET_COMMAND;
    gpio_set_level(device,0);
    bool result = ads1256_single_transmit(device, &tx, sizeof(tx));
    vTaskDelay(pdMS_TO_TICKS(1)); 
    gpio_set_level(device,1);
    return result;
}

bool ads1256_wake_up(ads1256_device_t device)
{
    const uint8_t tx = WAKEUP_COMMAND;
    gpio_set_level(device, 0); 
    bool result = ads1256_single_transmit(device, &tx, sizeof(tx));
    esp_rom_delay_us(2);
    gpio_set_level(device, 1);
    return result;
}

bool ads1256_sync(ads1256_device_t device)
{
    const uint8_t tx = SYNC_COMMAND;
    gpio_set_level(device, 0);
    bool result = ads1256_single_transmit(device, &tx, sizeof(tx));
    esp_rom_delay_us(4);
    gpio_set_level(device, 1);
    return result;
}

bool ads1256_change_channel(ads1256_device_t device, uint8_t channel)
{
    if (channel != MUX_REGISTER_FIRST_CHANNEL && channel != MUX_REGISTER_SECOND_CHANNEL && channel != MUX_REGISTER_THIRD_CHANNEL && channel != MUX_REGISTER_FOURTH_CHANNEL) {
        ESP_LOGE(TAG, "Invalid channel: %d", channel);
        return false;
    }

    bool result = ads1256_set_value(0x01, channel, device);
    return result;


}


 bool ads1256_pins_init(void)
 {
    /*init lokalny gpio output*/
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
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
    ESP_ERROR_CHECK(gpio_set_level(RESET_GPIO_2, 0)); 
    ESP_ERROR_CHECK(gpio_set_level(PWDN_GPIO_1, 1)); 
    ESP_ERROR_CHECK(gpio_set_level(PWDN_GPIO_2, 1)); 

    ESP_LOGI("ADS1256", "%d\n", gpio_get_level(CS_GPIO_2));

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

    if(ads1256_reset(device))
    {
        ESP_LOGI("ADS1256", "ADS1256 reset successfully");
    }
    else
    {
        ESP_LOGE("ADS1256", "Failed to reset ADS1256");
        return false;
    }
    if(ads1256_set_value(0x00, STATUS_REGISTER_DEFAULT, device))
    {
        ESP_LOGI("ADS1256", "ADS1256 status register set successfully");
    }
    else
    {
        ESP_LOGE("ADS1256", "Failed to set ADS1256 status register");
        return false;
    }
    if(ads1256_set_value(0x01, MUX_REGISTER_FOURTH_CHANNEL, device))
    {
        ESP_LOGI("ADS1256", "ADS1256 MUX register set successfully");
    }
    else
    {
        ESP_LOGE("ADS1256", "Failed to set ADS1256 MUX register");
        return false;
    }
    if(ads1256_set_value(0x02, ADCON_REGISTER, device))
    {
        ESP_LOGI("ADS1256", "ADS1256 ADCON register set successfully");
    }
    else
    {
        ESP_LOGE("ADS1256", "Failed to set ADS1256 ADCON register");
        return false;
    }

    if(ads1256_set_value(0x03, DATA_RATE_REGISTER_30000SPS, device))
    {
        ESP_LOGI("ADS1256", "ADS1256 data rate register set successfully");
    }
    else
    {
        ESP_LOGE("ADS1256", "Failed to set ADS1256 data rate register");
        return false;
    }
    if(ads1256_self_cal(device))
    {
        ESP_LOGI("ADS1256", "ADS1256 self-calibration completed successfully");
    }
    else
    {
        ESP_LOGE("ADS1256", "Failed to perform self-calibration on ADS1256");
        return false;
    }
    return true;
}

bool ads1256_get_raw_data(ads1256_device_t device, ads1256_raw_data_t* data)
{
    uint8_t tx_data = RDATA_COMMAND; 
    uint8_t dummy_data[3] = {0x00, 0x00, 0x00}; 
    gpio_set_level(device,0);
    
    if(ads1256_single_transmit(device, &tx_data, sizeof(tx_data)) == false)
    {
        ESP_LOGE("ADS1256", "Failed to send RDATA command to ADS1256");
        return false;
    }

    esp_rom_delay_us(7); 

    if(!_ads1256_spi_transmit(dummy_data, sizeof(dummy_data), data->channel_1, sizeof(data->channel_1)))
    {
        ESP_LOGE("ADS1256", "Failed to read data from ADS1256");
        return false;
    }

    gpio_set_level(device,1);
    
    int32_t value = (data->channel_1[0] << 16) | (data->channel_1[1] << 8) | data->channel_1[2];
    if (value & 0x800000) {
        value |= 0xFF000000; // sign-extend if negative
    }
        ESP_LOGI("ADS1256", "Raw sign value: %d", value);



    return true;
}

bool ads1256_read_id(ads1256_device_t device)
{
    //debug func
    gpio_set_level(device,0);

    uint8_t tx_data1[2] = {0x12, 0x00};
    if(!_ads1256_spi_transmit(tx_data1, sizeof(tx_data1), NULL, 0))
    {
        ESP_LOGE("ADS1256", "Failed to read ID from ADS1256");
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(1)); 

    uint8_t txdata2[1] = {0x00}; 
    uint8_t rx_data2[1] = {0};
    if(!_ads1256_spi_transmit(txdata2, sizeof(txdata2), rx_data2, sizeof(rx_data2)))
    {
        ESP_LOGE("ADS1256", "Failed to read ID from ADS1256");
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(1)); 
    gpio_set_level(device,1);
    ESP_LOGI("ADS1256", "GAIN: %d on %s", (rx_data2[0]), ads1256_device_to_string(device));
    return true;
}

void ads1256_read_data_continuously(void*  pvParameters)
{
    ads1256_device_t device = *((ads1256_device_t*) pvParameters);
    ads1256_raw_data_t data;
    uint8_t dummy_data[3] = {0x00, 0x00, 0x00}; 
    int32_t value = 0;

    /* counting average of measurments */
    // int64_t sum = 0;
    // uint16_t counter = 0;
    /* counting average of measurments */

    //petla for do debuga
    
    for(int i = 0; i < 60000 ; i++) 
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        // esp_rom_delay_us(500); 
        gpio_set_level(device, 0);
        if(!_ads1256_spi_transmit(dummy_data, sizeof(dummy_data), data.channel_1, sizeof(data.channel_1)))
        {
            ESP_LOGE("ADS1256", "Failed to read data from ADS1256");
        }
        gpio_set_level(device, 1);

        value = (data.channel_1[0] << 16) | (data.channel_1[1] << 8) | data.channel_1[2];
        if (value & 0x800000) {
            value |= 0xFF000000;
        }
        // ESP_LOGI("ADS1256", "Raw sign value: %d on %s", value, ads1256_device_to_string(device));

        if (buffer_index < BUFFER_SIZE) {
            value_buffer[buffer_index++] = value;
        } else {
            ESP_LOGW("ADS1256", "Buffer full! Ignoring new values.");
        }
        

        /* counting average of measurments */
        // counter ++;
        // sum += value;
        // ESP_LOGI("ADS1256", "Average value after %d reads: %lld on %s", counter, sum / counter, ads1256_device_to_string(device));
        /* counting average of measurments */


    }


    for (size_t i = 0; i < buffer_index; i++) {
        ESP_LOGI("ADS1256", "Buffered value[%d]: %d", i, value_buffer[i]);
        vTaskDelay(pdMS_TO_TICKS(10)); // Spowolnienie wypisywania
    }
    vTaskDelete(NULL);
}

void ads1256_start_readc(ads1256_device_t device)
{
    vTaskDelay(pdMS_TO_TICKS(1000)); 

    device_copy = device;
    uint8_t tx_data = RDATAC_COMMAND;

    gpio_set_level(device, 0); 
    if(ads1256_single_transmit(device, &tx_data, sizeof(tx_data)) == false)
    {
        ESP_LOGE("ADS1256", "Failed to start continuous read on ADS1256");
    }
    else
    {
        ESP_LOGI("ADS1256", "Continuous read started on %s", ads1256_device_to_string(device));
    }

    vTaskDelay(pdMS_TO_TICKS(1)); 

    gpio_set_level(device, 1); 


    xTaskCreate(ads1256_read_data_continuously, "ads1256_task_readc", 4096, (void*)&device_copy, 10, &DRDY1_task);


}


void ads1256_data_from_channels(void*  pvParameters)
{
    vTaskDelay(pdMS_TO_TICKS(1000)); 

    ads1256_device_t device = *((ads1256_device_t*) pvParameters);
    ads1256_raw_data_t data;

    while (1)
    {

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        ads1256_change_channel(device, MUX_REGISTER_FIRST_CHANNEL);
        ads1256_sync(device);
        ads1256_wake_up(device);
        ads1256_get_raw_data(device, &data);

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        ads1256_change_channel(device, MUX_REGISTER_SECOND_CHANNEL);
        ads1256_sync(device);
        ads1256_wake_up(device);
        ads1256_get_raw_data(device, &data);

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); 
        ads1256_change_channel(device, MUX_REGISTER_THIRD_CHANNEL);
        ads1256_sync(device);
        ads1256_wake_up(device);
        ads1256_get_raw_data(device, &data);

    }
    
}


void ads1256_start_channel_task(ads1256_device_t device)
{
    device_copy = device;
    xTaskCreate(ads1256_data_from_channels, "ads1256_task", 4096, (void*)&device_copy, 10, &DRDY1_task);
}