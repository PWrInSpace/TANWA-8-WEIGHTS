#ifndef PWRINSPACE_APP_TASK_H
#define PWRINSPACE_APP_TASK_H

#include "esp_err.h"
#include "ads1256.h"

void app_task(void *arg);
esp_err_t app_task_init(void);
esp_err_t app_task_deinit(void);
void start_readc_task(ads1256_device_t dev ,uint8_t time, char *path);
#endif //PWRINSPACE_APP_TASK_H