#ifndef MOCK_FREERTOS_SEMPHR_H
#define MOCK_FREERTOS_SEMPHR_H

#include "FreeRTOS.h"

typedef void* SemaphoreHandle_t;

static inline SemaphoreHandle_t xSemaphoreCreateMutex(void) {
    static int dummy = 1;
    return (SemaphoreHandle_t)&dummy;
}

static inline SemaphoreHandle_t xSemaphoreCreateBinary(void) {
    static int dummy = 1;
    return (SemaphoreHandle_t)&dummy;
}

static inline BaseType_t xSemaphoreTake(SemaphoreHandle_t sem, TickType_t delay) {
    (void)sem; (void)delay;
    return pdTRUE;
}

static inline BaseType_t xSemaphoreGive(SemaphoreHandle_t sem) {
    (void)sem;
    return pdTRUE;
}

static inline void vSemaphoreDelete(SemaphoreHandle_t sem) {
    (void)sem;
}

#endif
