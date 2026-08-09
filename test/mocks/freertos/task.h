#ifndef MOCK_FREERTOS_TASK_H
#define MOCK_FREERTOS_TASK_H

#include "FreeRTOS.h"

typedef void* TaskHandle_t;
void vTaskDelay(TickType_t ticks);

#endif
