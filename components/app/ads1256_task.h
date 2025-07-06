#ifndef ads1256_task_h
#define ads1256_task_h

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "ads1256.h"
#include "mcu_spi_config.h"

#define BUFFER_READC_SAMPLES 10000 //1 sample = 6 bytes

/*
Kompilator twierdzi ze chce padding i mowi ze struktira ma 6 bajtow a nie 5
nie dodaje __attribute__((packed)) aby wymusic 5 bajtow bo to moze spowolnic dzialanie
wiec wypelniam to dodatkowym bajtem aby nie bylo rozbierznosci w dzialaniu na roznych
kompilatorach i architekturach
*/

typedef struct readc_frame_t {
    uint16_t time;
    uint8_t data[3];
    uint8_t unused; 
} readc_frame_t;

extern volatile bool readc_stop_flag;
extern readc_frame_t* buffer_readc_A;
extern readc_frame_t* buffer_readc_B;
extern SemaphoreHandle_t readc_A_mutex;
extern SemaphoreHandle_t readc_B_mutex;
extern SemaphoreHandle_t buffer_A_ready;
extern SemaphoreHandle_t buffer_B_ready;


bool ads1256_task_init(void);
void ads1256_start_readc(ads1256_device_t device);


#endif