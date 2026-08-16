#ifndef MOCK_FREERTOS_TASK_H
#define MOCK_FREERTOS_TASK_H

#include "FreeRTOS.h"

typedef void* TaskHandle_t;
void vTaskDelay(TickType_t ticks);

static inline BaseType_t xTaskNotifyFromISR(TaskHandle_t task, uint32_t value, eNotifyAction action, BaseType_t* higherPriorityTaskWoken) {
    (void)task; (void)value; (void)action;
    if (higherPriorityTaskWoken) *higherPriorityTaskWoken = pdFALSE;
    return pdPASS;
}

#endif
