// Copyright 2022 PWrInSpace, Kuba
#include <string.h>
#include <esp_log.h>
#include "system_timer.h"
#include "esp_log.h"

#define TAG "TIMER"

#define MS_TO_MICRO(x) x * 1000

static struct {
    sys_timer_t* timers;
    size_t number_of_timers;
}gb;


bool sys_timer_init(sys_timer_t * timers, size_t number_of_timers) {
    if (timers == NULL || number_of_timers == 0) {
        return false;
    }

    if (number_of_timers > MAX_NUMBER_OF_TIMERS) {
        return false;
    }

    // work around, timer handle can not be null
    gb.timers = (sys_timer_t*)malloc(sizeof(sys_timer_t) * number_of_timers);
    memcpy(gb.timers, timers, sizeof(sys_timer_t) * number_of_timers);
    gb.number_of_timers = number_of_timers;

    esp_timer_init();

    esp_timer_create_args_t args;
    ESP_LOGI(TAG, "Timers number %d", number_of_timers);
    for (size_t i = 0; i < gb.number_of_timers; ++i) {
        args.callback = gb.timers[i].timer_callback_fnc;
        args.arg = gb.timers[i].timer_arg;
        args.dispatch_method = 0;
        esp_err_t ret = esp_timer_create(&args, &gb.timers[i].timer_handle);
        if (ret == ESP_ERR_NO_MEM) {
            ESP_LOGE(TAG, "memory error");
            return false;
        } else if (ret == ESP_ERR_INVALID_ARG) {
            ESP_LOGE(TAG, "Inavalid arrg %d", i);
            return false;
        }
    }

    return true;
}

static size_t get_timer_index_by_id(sys_timer_id_t id) {
    for (size_t i = 0; i < gb.number_of_timers; ++i) {
        if (gb.timers[i].timer_id == id) {
            return i;
        }
    }

    return TIMER_INVALID_INDEX;
}


bool sys_timer_start(sys_timer_id_t id, uint32_t miliseconds, sys_timer_type_t type) {
    if (miliseconds == 0) {
        return false;
    }

    size_t index = get_timer_index_by_id(id);
    if (index == TIMER_INVALID_INDEX) {
        return false;
    }

    if (gb.timers[index].timer_callback_fnc == NULL) {
        ESP_LOGE(TAG, "Callback is NULL");
        return false;
    }

    if (esp_timer_stop(gb.timers[index].timer_handle) != ESP_OK) {
        // ESP_LOGW(TAG, "TIMER WAS RUNNING");
    }

    if (type == TIMER_TYPE_ONE_SHOT) {
        esp_timer_start_once(gb.timers[index].timer_handle, MS_TO_MICRO(miliseconds));
    } else {
        esp_timer_start_periodic(gb.timers[index].timer_handle, MS_TO_MICRO(miliseconds));
    }

    return true;
}

bool sys_timer_stop(sys_timer_id_t id) {
    size_t index = get_timer_index_by_id(id);
    if (index == TIMER_INVALID_INDEX) {
        return false;
    }

    if (esp_timer_stop(gb.timers[index].timer_handle) != ESP_OK) {
        ESP_LOGW(TAG, "TIMER WAS NOT RUNNING");
    }
    return true;
}

bool sys_timer_delete(sys_timer_id_t id) {
    size_t index = get_timer_index_by_id(id);
    if (index == TIMER_INVALID_INDEX) {
        return false;
    }

    if (esp_timer_stop(gb.timers[index].timer_handle) != ESP_OK) {
        ESP_LOGW(TAG, "TIMER stop error");
        return false;
    }

    if (esp_timer_delete(gb.timers[index].timer_handle) != ESP_OK) {
        ESP_LOGE(TAG, "Timer delete error");
        return false;
    }

    return true;
}

bool sys_timer_restart(sys_timer_id_t id, uint64_t timeout) {
    size_t index = get_timer_index_by_id(id);
    if (index == TIMER_INVALID_INDEX) {
        return false;
    }


    if (esp_timer_restart(gb.timers[index].timer_handle, MS_TO_MICRO(timeout)) != ESP_OK) {
        ESP_LOGW(TAG, "Timer restart error");
        return false;
    }

    return true;
}

bool sys_timer_get_expiry_time(sys_timer_id_t id, uint64_t *expiry) {
    size_t index = get_timer_index_by_id(id);
    if (index == TIMER_INVALID_INDEX) {
        ESP_LOGE(TAG, "Invalid timer id");
        return false;
    }

    if (esp_timer_is_active(gb.timers[index].timer_handle) == false) {
        return false;
    }

    if (esp_timer_get_expiry_time(gb.timers[index].timer_handle, expiry) != ESP_OK) {
        return false;
    }

    return true;
}
