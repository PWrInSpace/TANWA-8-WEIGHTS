#ifndef PWRINSPACE_CAN_COMMANDS_H
#define PWRINSPACE_CAN_COMMANDS_H

#include "esp_err.h"

typedef enum {
    // COM -> WEIGHT_BOARD
    CAN_GET_STATUS	                =   0x3FF0,
    CAN_GET_BOARD_DATA	            =   0x2FE0,
    CAN_START_MEASURE	            =   0x0FD0,
    CAN_ADS_TARE	                =   0x3FC0,
    CAN_SET_ADS_CH	                =   0x3FA0,
    CAN_SET_ADS_OFFSET	            =   0x3F90,
    CAN_GET_ADS_CH_ALL_WEIGHT       =	0x2F90,
    CAN_GET_ADS_CH_WEIGHT	        =   0x2F80,
    CAN_GET_WEIGHTS 	            =   0x2F40,

    // WEIGHT_BOARD -> COM
    CAN_SEND_STATUS	                =   0x3F40,
    CAN_SEND_BOARD_DATA	            =   0x3F11,
    CAN_SEND_ADS1_ALL_CH_WEIGHT1    =	0x3F12,
    CAN_SEND_ADS1_ALL_CH_WEIGHT2    =	0x3F13,
    CAN_SEND_ADS2_ALL_CH_WEIGHT1    =	0x3F14,
    CAN_SEND_ADS2_ALL_CH_WEIGHT2    =	0x3F15,
    CAN_SEND_ADS_CH_WEIGHT	        =   0x3F20,
    CAN_SEND_WEIGHTS	            =   0x3F30


} can_message_id_t;

/*
* WEIGHT_BOARD -> COM
* 
* Data length = 3 bytes
* data [0] = TEMP0 | data [1] = TEMP1 | data [2] = I_CURR
*/
esp_err_t can_get_status(uint8_t *data, uint8_t length);

/*
* WEIGHT_BOARD -> COM
* ---
* --- TODO: active channel ostatnie pomiary z kazdego, offsety, SPS i tyle ale to potem
* ---
*/
esp_err_t can_get_board_data(uint8_t *data, uint8_t length);

/*
* COM -> WEIGHT_BOARD 
* 
* Data length = 3 bytes
* data [0] = dev_num (1 or 2) | data [1..2] = Time_s
*
*/
esp_err_t can_start_measure(uint8_t *data, uint8_t length);

/*
* COM -> WEIGHT_BOARD 
* 
* Data length = 1 byte
* data [0] = dev_num (1 or 2)
*
*/
esp_err_t can_ads_tare(uint8_t *data, uint8_t length);


/*
* COM -> WEIGHT_BOARD
*
* Data length = 2 bytes
* data [0] = dev_num (1 or 2) | data [1] = channel_num (0-3)
*/
esp_err_t can_set_ads_ch(uint8_t *data, uint8_t length);

/*
* COM -> WEIGHT_BOARD
*
* Data length = 6 bytes
* data [0] = dev_num (1 or 2) | data[1] = channel_num (0-3) | data [2..5] = offset (int32_t)
*/
esp_err_t can_set_ads_offset(uint8_t *data, uint8_t length);

/*
* COM -> WEIGHT_BOARD
* Data length = 1 byte
* data [0] = dev_num (1 or 2)
*
* 1'st WEIGHT_BOARD -> COM
* Data length = 8 bytes
* data [0..3] = weigh channel_0 (float) | data [4..7] = weigh channel_1 (float)
*
* 2'nd WEIGHT_BOARD -> COM
* Data length = 8 bytes
* data [0..3] = weigh channel_2 (float) | data [4..7] = weigh channel_3 (float)
*/
esp_err_t can_get_ads_ch_all_weight(uint8_t *data, uint8_t length);

/*
* COM -> WEIGHT_BOARD
* Data length = 2 bytes
* data [0] = dev_num (1 or 2) | data [1] = channel_num (0-3)
*
* WEIGHT_BOARD -> COM
* Data length = 4 bytes 
* data [0..3] = weight (float)
*/
esp_err_t can_get_ads_ch_weight(uint8_t *data, uint8_t length);

/*
* WEIGHT_BOARD -> COM
* Data length = 8 bytes
* data [0..3] = rocket weight (float) | data [4..7] = N2O weight (float)
*/
esp_err_t can_get_weights(uint8_t *data, uint8_t length);

#endif //PWRINSPACE_CAN_COMMANDS_H