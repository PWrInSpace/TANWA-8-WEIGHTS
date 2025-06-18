#include "ads1256.h"
#include "mcu_spi_config.h"
#include "mcu_gpio_config.h"
#include "esp_log.h"
#include "driver/gpio.h"

#define TAG "ads1256"

TaskHandle_t DRDY1_task = NULL;
TaskHandle_t DRDY2_task = NULL;
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

void isr_rdy1_loop(void*  pvParameters)
{
    while (1)
    {
        //TODO: Implementacja handling ADS1256 data ready interrupt na RDY_GPIO_1
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); 
    }
}

void isr_rdy2_loop(void*  pvParameters)
{
    while (1)
    {
        //TODO: Implementacja handling ADS1256 data ready interrupt na RDY_GPIO_2
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); 
    }
}

bool ads1256_single_transmit(ads1256_device_t device, const uint8_t* tx_data, size_t tx_length)
{
    if (tx_data == NULL || tx_length == 0) {
        ESP_LOGE(TAG, "Invalid transmit data or length");
        return false;
    }

    gpio_set_level(device,0);
    if(!_ads1256_spi_transmit(tx_data, tx_length, NULL, 0)) {
        ESP_LOGE(TAG, "Failed to transmit data to ADS1256");
        return false;
    }
    esp_rom_delay_us(1); 
    gpio_set_level(device,1);

    return true;

}
bool ads1256_set_value(uint8_t register_address, uint8_t value, ads1256_device_t device)
{
    bool res = true;
    uint8_t tx[3] = {WREG_COMMAND | register_address, 0x00, value};
    res = res && ads1256_single_transmit(device, &tx[0], sizeof(tx));
    res = res && ads1256_single_transmit(device, &tx[1], sizeof(tx));
    res = res && ads1256_single_transmit(device, &tx[3], sizeof(tx));
    return res;
}

bool ads1256_self_cal(ads1256_device_t device)
{
    const uint8_t tx = SELFCAL_COMMAND;
    return ads1256_single_transmit(device, &tx, sizeof(tx));
}


bool ads1256_reset(ads1256_device_t device)
{
    const uint8_t tx = RESET_COMMAND;
    bool result = ads1256_single_transmit(device, &tx, sizeof(tx));
    vTaskDelay(pdMS_TO_TICKS(10)); 
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
        CS_GPIO_2
    };

    for (int i = 0; i < sizeof(output_pins)/sizeof(output_pins[0]); i++) {
        io_conf.pin_bit_mask = 1ULL << output_pins[i];
        gpio_config(&io_conf);
    }

    ESP_ERROR_CHECK(gpio_set_level(CS_GPIO_1, 1));  
    ESP_ERROR_CHECK(gpio_set_level(CS_GPIO_2, 1)); 

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

    

    /*Debug problemu z ustawianiem pin lvl na outpucie*/
    // if(gpio_get_level(CS_GPIO_1) == 1)
    // {
    //     ESP_LOGI("ADS1256", "GPIO %d is set to HIGH (correctly configured)", CS_GPIO_1);
    // }
    // else
    // {
    //     ESP_LOGE("ADS1256", "GPIO %d is not set to HIGH (check configuration)", CS_GPIO_1);
    //     // return false;
    // }

    // if(gpio_get_level(CS_GPIO_2) == 1)
    // {
    //     ESP_LOGI("ADS1256", "GPIO %d is set to HIGH (correctly configured)", CS_GPIO_2);
    // }
    // else
    // {
    //     ESP_LOGE("ADS1256", "GPIO %d is not set to HIGH (check configuration)", CS_GPIO_2);
    //     // return false;
    // }
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

    return true;
}

bool ads1256_init(ads1256_device_t device)
{    
    // xTaskCreate(isr_rdy1_loop, "ad7190_task", 4096, NULL, 10, &DRDY1_task); //TODO: to nie powinno byc w init
    // xTaskCreate(isr_rdy2_loop, "ad7190_task", 4096, NULL, 10, &DRDY2_task); //TODO: to nie powinno byc w init

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
    if(ads1256_set_value(0x01, MUX_REGISTER_FIRST_CHANNEL, device))
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

    if(ads1256_set_value(0x03, DATA_RATE_REGISTER_100SPS, device))
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
    
    data->channel_1[0] = rx_data2[0];
    data->channel_1[1] = rx_data2[1];
    data->channel_1[2] = rx_data2[2];
    int32_t value = (rx_data2[0] << 16) | (rx_data2[1] << 8) | rx_data2[2];
    if (value & 0x800000) {
        value |= 0xFF000000; // sign-extend if negative
    }
    ESP_LOGI("ADS1256", "Raw sign value: %d", value);



    return true;
}

bool ads1256_read_id(ads1256_device_t device)
{
    gpio_set_level(device,0);

    uint8_t tx_data1[2] = {0x12, 0x00};
    // uint8_t rx_data1[2] = {0, 0};
    if(!_ads1256_spi_transmit(tx_data1, sizeof(tx_data1), NULL, 0))
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
    gpio_set_level(device,1);
    ESP_LOGI("ADS1256", "ADS1256 GAIN: %d", rx_data2[0] & 0x07);
    return true;
}


bool ads1256_read_id2()
{
    
    // gpio_set_direction(7, GPIO_MODE_OUTPUT);
    gpio_set_level(7,0);
    uint8_t tx_data1[2] = {0x12, 0x00};
    uint8_t rx_data1[2] = {0, 0};
    if(!_ads1256_spi_transmit(tx_data1, sizeof(tx_data1), rx_data1, sizeof(rx_data1)))
    {
        ESP_LOGE("ADS1256", "Failed to read ID from ADS1256");
        return false;
    }
    esp_rom_delay_us(7);
    uint8_t txdata2[1] = {0x00}; // Dummy byte to read ID
    uint8_t rx_data2[1] = {0};
    // vTaskDelay(pdMS_TO_TICKS(10)); 
    if(!_ads1256_spi_transmit(txdata2, sizeof(txdata2), rx_data2, sizeof(rx_data2)))
    {
        ESP_LOGE("ADS1256", "Failed to read ID from ADS1256");
        return false;
    }
    esp_rom_delay_us(1);
    gpio_set_level(7,1);

    // *id = ;
    ESP_LOGI("ADS1256", "ADS1256 GAIN: %d",rx_data2[0]);
    return true;
}