#include "can_api.h"

#include <string.h>

#include "driver/twai.h"

#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define CAN_TASK_STACK_SIZE CONFIG_CAN_TASK_STACK_SIZE
#define CAN_TASK_PRIORITY CONFIG_CAN_TASK_PRIORITY
#define CAN_TASK_CORE_ID CONFIG_CAN_TASK_CORE_ID

#define TWAI_MAX_MESSAGE_LENGTH 8

#define TAG "CAN_API"

static struct {
    can_command_t *commands;
    size_t num_commands;
    TaskHandle_t task_handle;
} gb;

esp_err_t can_start(void) {
    esp_err_t err;

    // Start the TWAI driver
    err = twai_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start TWAI driver: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

esp_err_t can_stop(void) {
    esp_err_t err;

    // Stop the TWAI driver
    err = twai_stop();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop TWAI driver: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

esp_err_t can_send_message(uint32_t id, uint8_t *data, uint8_t length) {
    esp_err_t err;
    twai_message_t message;

    if(length > TWAI_MAX_MESSAGE_LENGTH) {
        ESP_LOGE(TAG, "Data length exceeds maximum allowed length");
        return ESP_ERR_INVALID_ARG;
    }

    // Prepare the message
    message.identifier = id;
    message.data_length_code = length;
    memcpy(message.data, data, length);

    // Send the message
    err = twai_transmit(&message, pdMS_TO_TICKS(10));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send CAN message: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

esp_err_t can_register_commands(can_command_t *commands, size_t num_commands) {
    if (commands == NULL || num_commands == 0) {
        ESP_LOGE(TAG, "Invalid arguments");
        return ESP_ERR_INVALID_ARG;
    }

    gb.commands = commands;
    gb.num_commands = num_commands;

    return ESP_OK;
}

esp_err_t can_task_init(void) {

    if (gb.commands == NULL || gb.num_commands == 0) {
        ESP_LOGE(TAG, "No CAN commands registered");
        return ESP_ERR_INVALID_STATE;
    }

    if(xTaskCreatePinnedToCore(can_task, "CAN Task", CAN_TASK_STACK_SIZE, NULL, CAN_TASK_PRIORITY, &gb.task_handle, CAN_TASK_CORE_ID) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create CAN task");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void can_task(void *arg) {
    esp_err_t err;
    twai_message_t message;

    while (1) {
        // Receive a message
        err = twai_receive(&message, pdMS_TO_TICKS(10));
        if (err == ESP_OK) {
            // Process the received message
            for (size_t i = 0; i < gb.num_commands; i++) {
                if (gb.commands[i].message_id == message.identifier) {
                    gb.commands[i].handler(message.data, message.data_length_code);
                    break;
                }
            }
        } else if (err != ESP_ERR_TIMEOUT) {
            ESP_LOGE(TAG, "Failed to receive CAN message: %s", esp_err_to_name(err));
        }
    }
}

esp_err_t can_deinit(void) {
    esp_err_t err;

    // Stop the TWAI driver
    err = can_stop();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop CAN driver: %s", esp_err_to_name(err));
        return err;
    }

    // Delete the CAN task
    if (gb.task_handle != NULL) {
        vTaskDelete(gb.task_handle);
        gb.task_handle = NULL;
    }

    // Clear the command list
    gb.commands = NULL;
    gb.num_commands = 0;

    return ESP_OK;
}