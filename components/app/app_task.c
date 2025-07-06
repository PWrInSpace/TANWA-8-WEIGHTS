#include "app_task.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "can_api.h"
#include "timers_config.h"
#include "ads1256_task.h"
#include "sd_task.h"

#define APP_TASK_STACK_SIZE CONFIG_APP_TASK_STACK_SIZE
#define APP_TASK_PRIORITY CONFIG_APP_TASK_PRIORITY
#define APP_TASK_CORE_ID CONFIG_APP_TASK_CORE_ID

static TaskHandle_t app_task_handle = NULL;

esp_err_t app_task_init(void) {
    
    if(xTaskCreatePinnedToCore(app_task, "app_task", APP_TASK_STACK_SIZE, NULL, APP_TASK_PRIORITY, &app_task_handle, APP_TASK_CORE_ID) == pdPASS) {
        ESP_LOGI("APP_TASK", "App task created successfully");
    } else {
        ESP_LOGE("APP_TASK", "Failed to create app task");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t app_task_deinit(void) {
    if (app_task_handle != NULL) {
        vTaskDelete(app_task_handle);
        app_task_handle = NULL;
    }
    
    return ESP_OK;
}

    
void start_readc_task(void) {
    ESP_LOGI("APP_TASK", "Starting readc task");

    ads1256_start_readc(ADS1256_DEVICE_1);
    run_readc_sd_task();

    ESP_LOGI("APP_TASK", "Readc task started");

    start_stopping_readc_task(15);
}

void app_task(void *arg) {

    // YOUR IMAGINATION IS THE ONLY LIMITATION
    while(1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}