// Copyright 2022 PWrInSpace, Kuba
#ifndef TIMER_H
#define TIMER_H

#include <stdbool.h>
#include "esp_timer.h"

#define MAX_NUMBER_OF_TIMERS 15
#define TIMER_INVALID_INDEX 999

typedef enum {
    TIMER_TYPE_ONE_SHOT = 0,
    TIMER_TYPE_PERIODIC,
}sys_timer_type_t;

typedef uint8_t sys_timer_id_t;

typedef void (*sys_timer_callback)(void *arg);

typedef struct {
    sys_timer_id_t timer_id;
    sys_timer_callback timer_callback_fnc;
    void *timer_arg;
    esp_timer_handle_t timer_handle;
} sys_timer_t;

/**
 * @brief Initialzie and create timers
 *
 * @param timers pointer to timers, !must be global variable!
 * @param number_of_timers number of timers
 */
bool sys_timer_init(sys_timer_t * timers, size_t number_of_timers);
bool sys_timer_start(sys_timer_id_t id, uint32_t miliseconds, sys_timer_type_t type);
bool sys_timer_stop(sys_timer_id_t id);
bool sys_timer_delete(sys_timer_id_t id);
bool sys_timer_restart(sys_timer_id_t id, uint64_t timeout);
bool sys_timer_get_expiry_time(sys_timer_id_t id, uint64_t *expiry);

#endif
