///===-----------------------------------------------------------------------------------------===//
///
/// Copyright (c) PWr in Space. All rights reserved.
/// Created: 27.01.2024 by Michał Kos
///
///===-----------------------------------------------------------------------------------------===//
///
/// \file
/// This file contains declaration of the system console configuration, including initialization
/// and available commands for debugging/testing purposes.
///===-----------------------------------------------------------------------------------------===//

#ifndef PWRINSPACE_BOARD_CONFIG_H
#define PWRINSPACE_BOARD_CONFIG_H

#include "led_driver.h"
#include "esp_err.h"

typedef struct {
    char board_name[32];
    led_struct_t status_led;
} board_config_t;

extern board_config_t config;

esp_err_t board_config_init(void);

#endif /* PWRINSPACE_BOARD_CONFIG_H */


// RTR -> TEN SAM ID U MASTER I SLAVE BO RTR PRZEGRYWA WALKE O MIEJSCE!