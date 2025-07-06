#ifndef TIMERS_CONFIG_H
#define TIMERS_CONFIG_H

#include "system_timer.h"
#include "stdbool.h"

bool timers_init(void);
bool start_test_timer(void);
bool start_stopping_readc_task(uint16_t seconds);

#endif