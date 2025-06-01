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
#ifndef PWRINSPACE_CONSOLE_CONFIG_H_
#define PWRINSPACE_CONSOLE_CONFIG_H_

#include "esp_err.h"

/**
 * @brief Initialize cli
 *
 * @return esp_err_t initialization status
 */
esp_err_t console_config_init();

#endif /* PWRINSPACE_CONSOLE_CONFIG_H_ */