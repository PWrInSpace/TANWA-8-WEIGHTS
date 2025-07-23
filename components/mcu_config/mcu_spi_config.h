///===-----------------------------------------------------------------------------------------===//
///
/// Copyright (c) PWr in Space. All rights reserved.
/// Created: 15.05.2024 by Michał Kos
///
///===-----------------------------------------------------------------------------------------===//
///
/// \file
/// This file contains the configuration of the SPI of the MCU.
///===-----------------------------------------------------------------------------------------===//

#ifndef PWRINSPACE_MCU_SPI_CONFIG_H_
#define PWRINSPACE_MCU_SPI_CONFIG_H_

#include <stdbool.h>

#include "driver/gpio.h"
#include "driver/spi_common.h"
#include "driver/sdspi_host.h"
#include "esp_log.h"
#include "esp_rom_gpio.h"
#include "freertos/task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "rom/gpio.h"
#include "sdkconfig.h"
#include "soc/gpio_struct.h"

#define MCU_SPI_DEFAULT_CONFIG()                   \
  {                                                \
    .host_id = SDSPI_DEFAULT_HOST,                          \
    .bus_config = {.miso_io_num = CONFIG_SPI_MISO, \
                   .mosi_io_num = CONFIG_SPI_MOSI, \
                   .sclk_io_num = CONFIG_SPI_SCK,  \
                   .quadwp_io_num = -1,            \
                   .quadhd_io_num = -1,            \
                   .max_transfer_sz = 4000},       \
    .dev_config = {.clock_speed_hz = 400000,       \
                   .mode = 0,                      \
                   .spics_io_num = -1,             \
                   .queue_size = 1,                \
                   .flags = 0,                     \
                   .pre_cb = NULL},                \
    .spi_init_flag = false,                        \
  }

extern SemaphoreHandle_t mutex_spi;

typedef struct {
  spi_host_device_t host_id;
  spi_device_handle_t spi_ads1256_handle;
  spi_bus_config_t bus_config;
  spi_device_interface_config_t dev_config;
  bool spi_init_flag;
} mcu_spi_config_t;

/**
 * \brief Initiates the SPI bus
 * \return ESP_OK on success, ESP_FAIL otherwise
 * \note This function will only initiate SPI peripheral,
 *      it will not configure the GPIO for CS pins.
 */
esp_err_t mcu_spi_init(void);

/**
 * \brief Deinitializes the SPI bus
 * \return ESP_OK on success, ESP_FAIL otherwise
*/
esp_err_t mcu_spi_deinit(void);



bool _ads1256_add_device(void);  

typedef struct ads1256_spi_transmit_t
{
  const uint8_t* tx_data;  
  size_t tx_len;            
  uint8_t cs_pin; 
  bool rx_enabled;
} ads1256_spi_transmit_t;

/**
 * \brief SPI transmit function for ADS1256
 * \param[in] ads1256_spi_transmit_t structure
 * \param[out] rx_data output buffer
 * \param[in] rx_len length of output buffer
 */
bool _ads1256_spi_transmit(ads1256_spi_transmit_t* ads_transmit, uint8_t* rx_data, size_t rx_len);

bool _ads1256_spi_transmit_queued(const uint8_t* tx_data, size_t tx_len, uint8_t* rx_data, size_t rx_len);
#endif /* PWRINSPACE_MCU_SPI_CONFIG_H_ */
