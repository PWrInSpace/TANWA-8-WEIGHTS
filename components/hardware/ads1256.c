#include "ads1256.h"
#include "mcu_spi_config.h"
#include "mcu_gpio_config.h"
#include "esp_log.h"
#include "driver/gpio.h"

#define TAG "ads1256"

TaskHandle_t DRDY1_task = NULL;
TaskHandle_t DRDY2_task = NULL;
QueueHandle_t ads1256_queue_1;

/*
ads_cal_reg map
{OFC0, OFC1, OFC2, FSC0, FSC1, FSC2, zero_offset}
*/
const uint8_t ads_cal_reg[8][6] = { //TODO
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // DEVICE_1 first channel
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // DEVICE_1 second channel
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // DEVICE_1 third channel
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // DEVICE_1 fourth channel
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // DEVICE_2 first channel
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // DEVICE_2 second channel
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // DEVICE_2 third channel
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}  // DEVICE_2 fourth channel
};

const int32_t ads_cal_zero_offset[8] = 
{
    0, // DEVICE_1 first channel
    0, // DEVICE_1 second channel
    0, // DEVICE_1 third channel
    0, // DEVICE_1 fourth channel
    0, // DEVICE_2 first channel
    0, // DEVICE_2 second channel
    0, // DEVICE_2 third channel
    0  // DEVICE_2 fourth channel
};

const double ads_cal_factor[8] = 
{
    1.0f, // DEVICE_1 first channel
    1.0f, // DEVICE_1 second channel
    1.0f, // DEVICE_1 third channel
    1.0f, // DEVICE_1 fourth channel
    1.0f, // DEVICE_2 first channel
    1.0f, // DEVICE_2 second channel
    1.0f, // DEVICE_2 third channel
    1.0f  // DEVICE_2 fourth channel
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
    if (channel != MUX_REGISTER_FIRST_CHANNEL && channel != MUX_REGISTER_SECOND_CHANNEL && channel != MUX_REGISTER_THIRD_CHANNEL && channel != MUX_REGISTER_FOURTH_CHANNEL) {
        ESP_LOGE(TAG, "Invalid channel: %d", channel);
        return false;
    }

    bool result = ads1256_set_value(MUX_REGISTER, channel, device);

    //TODO: add here update of cal registers 
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
    if(ads1256_queue_1 == NULL)
    {
        ads1256_queue_1 = xQueueCreate(QUEUE_LENGTH, sizeof(ads1256_raw_data_sample_t));
    }


    if(!ads1256_read_cal_registers(device))
    {
        ESP_LOGE("ADS1256", "Failed to read calibration registers");
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
    if(ads1256_set_value(MUX_REGISTER, MUX_REGISTER_THIRD_CHANNEL, device))
    {
        ESP_LOGI("ADS1256", "ADS1256 MUX register set successfully");
    }
    else
    {
        ESP_LOGE("ADS1256", "Failed to set ADS1256 MUX register");
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

    if(ads1256_set_value(DATA_RATE_REGISTER, DATA_RATE_REGISTER_30000SPS, device))
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
    if(!ads1256_read_cal_registers(device))
    {
        ESP_LOGE("ADS1256", "Failed to read calibration registers");
        return false;
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


// void ads1256_read_data_continuously(void*  pvParameters)  //!FOR TESTING PURPOSES!
// {
//     ads1256_device_t* device = (ads1256_device_t*)pvParameters;
//     ads1256_raw_data_t data;
//     uint8_t dummy_data[3] = {0x00, 0x00, 0x00}; 
//     int32_t value = 0;
//     //-6230 - 2000 -> -6230/2000 -> 3.115 to 1g

//     /* counting average of measurments */
//     int64_t sum = 0;
//     uint16_t counter = 0;
//     /* counting average of measurments */
//     double grams = 0.0f;
//     //petla for do debuga
//     // while (1)

//     for(int i = 0; i < 30000*10 ; i++) 
//     {
//         ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
//         gpio_set_level(*device, 0);
//         if(!_ads1256_spi_transmit(dummy_data, sizeof(dummy_data), data.channel_1, sizeof(data.channel_1)))
//         {
//             ESP_LOGE("ADS1256", "Failed to read data from ADS1256");
//         }
//         gpio_set_level(*device, 1);

//         value = (data.channel_1[0] << 16) | (data.channel_1[1] << 8) | data.channel_1[2];
//         if (value & 0x800000) {
//             value |= 0xFF000000;
//         }
//         value -= zero_offset; // Adjusting the value with zero offset
//         grams = (double)value / -3.115; 
//         // ESP_LOGI("ADS1256", "Raw sign value: %d on %s", value, ads1256_device_to_string(device));
//             // -4470
//         value_buffer[(buffer_index++)%BUFFER_SIZE] = grams;
//         ESP_LOGI("ADS1256", "Weight %.4f grams on %s", grams, ads1256_device_to_string(device));
        

//         /* counting average of measurments */
//         counter ++;
//         sum += value;
//         // ESP_LOGI("ADS1256", "Average value after %d reads: %lld on %s", counter, sum / counter, ads1256_device_to_string(device));
//         /* counting average of measurments */


//     }


//     for (size_t i = 0; i < buffer_index; i++) {
//         ESP_LOGI("ADS1256", "Buffered value[%d]: %.4f", i, value_buffer[i]);
//         vTaskDelay(pdMS_TO_TICKS(10)); // Spowolnienie wypisywania
//     }
//     free(device);
//     vTaskDelete(NULL);
// }



void ads1256_data_from_channels(void*  pvParameters)
{
    vTaskDelay(pdMS_TO_TICKS(1000)); 

    ads1256_device_t* device = (ads1256_device_t*)pvParameters;
    ads1256_raw_data_t data;

    while (1)
    {

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        ads1256_change_channel(*device, MUX_REGISTER_FIRST_CHANNEL);
        ads1256_sync(*device);
        ads1256_wake_up(*device);
        ads1256_get_raw_data(*device, data.channel_1);

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        ads1256_change_channel(*device, MUX_REGISTER_SECOND_CHANNEL);
        ads1256_sync(*device);
        ads1256_wake_up(*device);
        ads1256_get_raw_data(*device, data.channel_2);

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); 
        ads1256_change_channel(*device, MUX_REGISTER_THIRD_CHANNEL);
        ads1256_sync(*device);
        ads1256_wake_up(*device);
        ads1256_get_raw_data(*device, data.channel_3);

    }

    free(device);
    vTaskDelete(NULL);
    
}


void ads1256_start_channel_task(ads1256_device_t device)
{
    ads1256_device_t* device_ptr = malloc(sizeof(ads1256_device_t));
    if (device_ptr == NULL) {
        ESP_LOGE("ADS1256", "Failed to allocate memory for device");
        return;
    }
    *device_ptr = device;
    xTaskCreate(ads1256_data_from_channels, "ads1256_task", 4096, (void*)device_ptr, 10, &DRDY1_task);
}