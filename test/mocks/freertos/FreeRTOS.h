#ifndef MOCK_FREERTOS_H
#define MOCK_FREERTOS_H

#include <stdint.h>
#include <stdbool.h>

typedef uint32_t TickType_t;
typedef int BaseType_t;

#define pdFALSE 0
#define pdTRUE  1
#define pdPASS  1
#define pdFAIL  0

#define portMAX_DELAY ((TickType_t)0xFFFFFFFF)
#define pdMS_TO_TICKS(ms) (ms)

typedef enum {
    eNoAction = 0,
    eSetBits,
    eIncrement,
    eSetValueWithOverwrite,
    eSetValueWithoutOverwrite
} eNotifyAction;

#define portYIELD_FROM_ISR(x) ((void)(x))

#endif
