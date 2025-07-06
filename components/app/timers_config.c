#include "timers_config.h"
#include "esp_log.h"
#include "ads1256_task.h"
#include "sd_task.h"

#define TAG "TIMERS"

void test_timer_callback(void* arg) {
    ESP_LOGI(TAG, "Test timer callback executed");
}

void stop_readc_task(void *arg) {
    ESP_LOGI(TAG, "Stopping readc task");
    readc_stop_flag = true;
}

bool timers_init(void) {
    sys_timer_t timers[] = {
        {
            .timer_id = 0,
            .timer_callback_fnc = test_timer_callback,
            .timer_arg = NULL
        },
        {
            .timer_id = 1,
            .timer_callback_fnc = stop_readc_task,
            .timer_arg = NULL
        },
    };

    return sys_timer_init(timers, sizeof(timers) / sizeof(timers[0]));
}

bool start_test_timer(void) {
    return sys_timer_start(0, 5, TIMER_TYPE_PERIODIC);
}
bool start_stopping_readc_task(uint16_t seconds) {
    ESP_LOGI(TAG, "Starting readc task stop timer for %d seconds", seconds);
    if (seconds == 0) {
        ESP_LOGE(TAG, "Cannot start readc task stop timer with 0 seconds");
        return false;
    }

    uint16_t milliseconds = seconds * 1000;

    return sys_timer_start(1, milliseconds, TIMER_TYPE_ONE_SHOT);
}