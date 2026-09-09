#ifndef SD_TASK_H
#define SD_TASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#define MOUNT_POINT "/sdcard"


extern TaskHandle_t sd_task;
extern volatile bool new_filename_flag;
esp_err_t sd_task_init(void);

void save_ads1256_buffor_task(void *arg);
bool run_readc_sd_task();
bool print_file(const char* path);
bool empty_file(const char* path);
void test(void *arg);
void run_weight_sd_task();
void delete_weight_sd_task();

#endif