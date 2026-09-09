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

    
bool start_readc_task(ads1256_wrapper_t* w, uint8_t time)
{
    if (!ads1256_start_readc(w)) {
        ESP_LOGE("APP_TASK", "Failed to start readc task");
        return false;
    }

    if (!start_stopping_readc_task(time)) {
        ESP_LOGE("APP_TASK", "Failed to start readc stop timer");
        readc_stop_flag = true;
        TaskHandle_t task = ads1256_wrapper_get_drdy_task(w);
        if (task != NULL) xTaskNotifyGive(task);
        return false;
    }

    ESP_LOGI("APP_TASK", "Readc task started");
    return true;
}

void app_task(void *arg) {

    // YOUR IMAGINATION IS THE ONLY LIMITATION
    while(1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}