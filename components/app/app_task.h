#ifndef PWRINSPACE_APP_TASK_H
#define PWRINSPACE_APP_TASK_H

#include "esp_err.h"
#include "ads1256_wrapper.h"

void app_task(void *arg);
esp_err_t app_task_init(void);
esp_err_t app_task_deinit(void);
bool start_readc_task(ads1256_wrapper_t* w, uint8_t time);
#endif //PWRINSPACE_APP_TASK_H