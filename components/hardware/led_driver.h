///===-----------------------------------------------------------------------------------------===//
///
/// Copyright (c) PWr in Space. All rights reserved.
/// Created: 27.01.2024 by Michał Kos
///
///===-----------------------------------------------------------------------------------------===//
///
/// \file
/// This file contains declaration of the simple LED driver that can be used to control both 
/// positive and negative driven LEDs. 
///===-----------------------------------------------------------------------------------------===//

#ifndef PWRINSPACE_LED_DRIVER_H_
#define PWRINSPACE_LED_DRIVER_H_

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

typedef enum {
    LED_STATE_OFF = 0,
    LED_STATE_ON,
} led_state_t;

typedef enum {
    LED_DRIVE_POSITIVE = 0,
    LED_DRIVE_OPEN_DRAIN,
} led_drive_t;

typedef enum {
    LED_OK = 0,
    LED_FAIL,
    LED_GPIO_ERR,
    LED_INVALID_ARG,
    LED_NULL_ARG,
} led_status_t;

typedef bool (*led_GPIO_set_level)(uint8_t _gpio_num, uint8_t _level);
typedef void (*led_delay)(uint32_t _ms);

typedef struct {
    led_GPIO_set_level _gpio_set_level;
    led_delay _delay;
    uint8_t gpio_num;
    led_drive_t drive;
    led_state_t state;
} led_struct_t;

led_status_t led_set_state(led_struct_t* led, led_state_t state);

led_status_t led_toggle(led_struct_t* led);

led_status_t led_delay_blink(led_struct_t* led, uint32_t period_ms);

#endif /* PWRINSPACE_LED_DRIVER_H_ */