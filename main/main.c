#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"

#include "board_config.h"
#include "setup_task.h"
#include "mcu_gpio_config.h"
#include "sd_task.h"
#include "app_task.h"
#define TAG "APP"
#include "mcu_twai_config.h"
#include "can_config.h"

extern board_config_t config;

void app_main(void) {
    

    ESP_LOGI(TAG, "%s TANWA WAGI board starting", config.board_name);

    if(setup_task_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize setup task");
        return;
    }

    //Checklista od stworcy
    //1. Upewnij sie ze sps jest ustawiony na 1000 (domyslnie)
    //   w pliku ads1256.c dla DEV1 (zmienna ads1256_config_dev1)
    //2. Upewnij sie ze w pliku ads1256.c jest ustawiona zmienna ads1256_channels_dev1[1] na:
    // {0x9D, 0xF6, 0xFF} {0x79, 0xBA, 0x49}; (kalibracja dla hamowni)
    //3. Upewnij sie ze w pliku ads1256_task.c jest ustawiona zmienna buffer_readc_A i buffer_readc_B 
    //na BUFFER_READC_SAMPLES = 5000
    //4. Upewnij sie ze HAMOWNIA jest skalibrowana na zasilaniu tanwy i ma zmierzone zero_offset i mnoznik (zapytaj Bartka jak nie wiesz)
    //5. Upewnij sie ze karta SD jest zamontowana!!
}
