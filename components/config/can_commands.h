#ifndef PWRINSPACE_CAN_COMMANDS_H
#define PWRINSPACE_CAN_COMMANDS_H

#include "esp_err.h"

/** PLACE YOUR CAN CALLBACKS AND CAN MESSAGES HERE IN FORMAT*/
typedef enum {
    START_READC_TASK_ID = 0x0FD0,
    GET_ADS_SINGLE_CHANNEL_WEIGHT_ID = 0x2F80,
} can_message_id_t;

esp_err_t start_readc_command_handler(uint8_t *data, uint8_t length);
esp_err_t get_ads_single_channel_weight(uint8_t *data, uint8_t length);
#endif //PWRINSPACE_CAN_COMMANDS_H