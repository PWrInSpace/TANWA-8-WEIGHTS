#ifndef ads1256_task_h
#define ads1256_task_h

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "ads1256.h"
#include "mcu_spi_config.h"

/* ustawic BUFFER_READC_SAMPLES tak aby byl wypelniany w max 3 sek*/
#define BUFFER_READC_SAMPLES 2988//1 sample = 6 bytes
#define HAMOWNIA_CHANNEL 1

/*
Kompilator twierdzi ze chce padding do readc_frame_t i mowi ze struktira ma 8 bajtow a nie 7
nie dodaje __attribute__((packed)) aby wymusic 7 bajtow bo to moze spowolnic dzialanie
wiec wypelniam to dodatkowym bajtem aby nie bylo rozbierznosci w dzialaniu na roznych
kompilatorach i architekturach
*/
typedef struct readc_frame_t {
    uint32_t time;
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
void ads1256_start_channel_task(ads1256_device_t device);

void ads1256_suspend_task(ads1256_device_t device);
void ads1256_resume_task(ads1256_device_t device);
void ads1256_delete_task(ads1256_device_t device);

#endif