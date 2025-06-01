#ifndef PWRINSPACE_SETUP_TASK_H
#define PWRINSPACE_SETUP_TASK_H

#include "esp_err.h"

void setup_task(void *arg);
esp_err_t setup_task_init(void);
esp_err_t setup_task_deinit(void);

#endif //PWRINSPACE_CAN_TASK_H