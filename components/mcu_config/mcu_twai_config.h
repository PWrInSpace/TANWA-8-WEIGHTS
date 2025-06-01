#ifndef PWRINSPACE_MCU_TWAI_CONFIG_H_
#define PWRINSPACE_MCU_TWAI_CONFIG_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "driver/twai.h"
#include "esp_err.h"

/*!
 * \brief TWAI configuration structure
 * \param tx_gpio_num GPIO number for TX pin
 * \param rx_gpio_num GPIO number for RX pin
 * \param mode TWAI mode
 */
typedef struct {
  gpio_num_t tx_gpio_num;
  gpio_num_t rx_gpio_num;
  twai_mode_t mode;
  twai_general_config_t g_config;
  twai_timing_config_t t_config;
  twai_filter_config_t f_config;
} mcu_twai_config_t;

/*!
 * \brief TWAI initialization, initializes and starts TWAI driver
 * \return ESP_OK on success, ESP_FAIL otherwise
 */
esp_err_t mcu_twai_init();

/*!
 * \brief TWAI deinitialization, stops and deinitializes TWAI driver
 * \return ESP_OK on success, ESP_FAIL otherwise
 */
esp_err_t mcu_twai_deinit();

/*!
 * \brief TWAI check alerts, checks if there are any alerts on the bus
 * \return ESP_OK on success, ESP_FAIL otherwise
 */
void mcu_twai_check_alerts();

#endif // PWRINSPACE_MCU_TWAI_CONFIG_H_