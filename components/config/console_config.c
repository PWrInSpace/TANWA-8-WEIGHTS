///===-----------------------------------------------------------------------------------------===//
///
/// Copyright (c) PWr in Space. All rights reserved.
/// Created: 27.01.2024 by Michał Kos
///
///===-----------------------------------------------------------------------------------------===//
///
/// \file
/// This file contains implementation of the system console configuration, including initialization
/// and available commands for debugging/testing purposes.
///===-----------------------------------------------------------------------------------------===//

#include "esp_log.h"
#include "esp_system.h"

#include "console.h"
#include "console_config.h"
#include "timers_config.h"
#include "app_task.h"
#include "sd_task.h"
#define TAG "CONSOLE_CONFIG"


// example function to reset the device
int reset_device(int argc, char **argv) {
    ESP_LOGI(TAG, "Resetting device...");
    esp_restart();
    return 0;
}

int start_timer_test(int argc, char **argv) {
    // This function is a placeholder for starting a timer test
    ESP_LOGI(TAG, "Starting timer test from cmd");
    if (!start_test_timer()) {
        ESP_LOGE(TAG, "Failed to start test timer");
        return -1;
    }
    ESP_LOGI(TAG, "Test timer started successfully");
    return 0;
}

int readc_task(int argc, char **argv) {
    // This function is a placeholder for starting the readc task
    ESP_LOGI(TAG, "Starting readc task from cmd");
    start_readc_task();
    return 0;
}

int read_sd_file(int argc, char **argv) {
    // This function is a placeholder for reading a file from the SD card
    print_file("/sdcard/dupa.txt");
    return 0;
}

int empty_sd_file(int argc, char **argv) {
    if (empty_file("/sdcard/dupa.txt")) {
        ESP_LOGI(TAG, "File emptied successfully");
    } else {
        ESP_LOGE(TAG, "Failed to empty file");
    }
    return 0;
}
 // Place for the console configuration

 static esp_console_cmd_t cmd [] = {
 // example command:
 // cmd     help description   hint  function      args
 {"reset", "Reset the device", NULL, reset_device, NULL},
 {"timer_test", "Start a test timer", NULL, start_timer_test, NULL},
 {"readc_task_on_15sek", "Start the readc task", NULL, readc_task, NULL},
{"read_sd_file", "Read a file from the SD card", NULL, read_sd_file, NULL},
{"empty_sd_file", "Empty a file on the SD card", NULL, empty_sd_file, NULL},
 };

esp_err_t console_config_init() {
    esp_err_t ret;
    ret = console_init();
    ret = console_register_commands(cmd, sizeof(cmd) / sizeof(cmd[0]));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "%s", esp_err_to_name(ret));
        return ret;
    }
    return ret;
}