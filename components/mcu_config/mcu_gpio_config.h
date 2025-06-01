///===-----------------------------------------------------------------------------------------===//
///
/// Copyright (c) PWr in Space. All rights reserved.
/// Created: 22.01.2024 by Michał Kos
///
///===-----------------------------------------------------------------------------------------===//
///
/// \file
/// This file contains the configuration of the GPIO pins of the MCU.
///===-----------------------------------------------------------------------------------------===//

#ifndef PWRINSPACE_MCU_GPIO_CONFIG_H_
#define PWRINSPACE_MCU_GPIO_CONFIG_H_

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "rom/gpio.h"
#include "soc/gpio_struct.h"

//ADD CONFIGURED GPIO PINS HERE
typedef enum {
    LED_GPIO = CONFIG_GPIO_LED,
} mcu_gpio_cfg_t;

// ADD GPIO PINS INDICES HERE
typedef enum {
    LED_GPIO_INDEX = 0,
    MAX_GPIO_INDEX
} mcu_gpio_index_cfg_t;

typedef struct {
    uint8_t pins[MAX_GPIO_INDEX];
    uint8_t num_pins;
    gpio_config_t configs[MAX_GPIO_INDEX];
} mcu_gpio_config_t;

esp_err_t mcu_gpio_init(void);

bool _mcu_gpio_set_level(uint8_t gpio, uint8_t level);

bool _mcu_gpio_get_level(uint8_t gpio, uint8_t* level);

#endif /* PWRINSPACE_MCU_GPIO_CONFIG_H_ */