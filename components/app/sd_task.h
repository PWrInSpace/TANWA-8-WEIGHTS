#ifndef SD_TASK_H
#define SD_TASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#define MOUNT_POINT "/sdcard"


esp_err_t sd_task_init(void);

void save_ads1256_buffor_task(void *arg);
void run_readc_sd_task(const char* path);
bool print_file(const char* path);
bool empty_file(const char* path);
void test(void *arg);

#endif