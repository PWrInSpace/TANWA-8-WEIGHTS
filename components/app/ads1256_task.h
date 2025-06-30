#ifndef ads1256_task_h
#define ads1256_task_h

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "ads1256.h"
#include "mcu_spi_config.h"

#define BUFFER_READC_SIZE 32000 //(32kB)

extern uint8_t* buffer_readc_A;
extern uint8_t* buffer_readc_B;
extern SemaphoreHandle_t readc_A_mutex;
extern SemaphoreHandle_t readc_B_mutex;

bool ads1256_task_init(void);
void ads1256_start_readc(ads1256_device_t device);


#endif