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
#include "ads1256_task.h"
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
    if(argc != 3)
    {
        ESP_LOGE(TAG, "Usage: command [dev_num] [time]");
        return -1;
    }
    int device = atoi(argv[1]);
    uint8_t time = atoi(argv[2]);

    ads1256_device_t dev;

    if(device == 1)
    {
        dev = ADS1256_DEVICE_1;
    }
    else if(device == 2)
    {
        dev = ADS1256_DEVICE_2;
    }
    else
    {
        ESP_LOGE(TAG, "Wrong dev_num. 1 - DEV1, 2-DEV2");
        return -1;
    }

    if(time < 1)
    {
        ESP_LOGE(TAG, "Time value must be greater than 0");
        return -1;
    }
    ESP_LOGI(TAG, "Starting readc task from cmd");

    start_readc_task(dev, time);
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

int change_mux_channel(int argc, char **argv)
{
    if(argc != 2)
    {
        ESP_LOGE(TAG, "One arg needed");
        return -1;
    }

    int channel = atoi(argv[1]);

    if(channel > 8 || channel < 1)
    {
        ESP_LOGE(TAG, "Channels range is 1-8");
        return -1;
    }

    ads1256_device_t dev;
    uint8_t channel_hex;

    switch(channel)
    {
        case 1: channel_hex = MUX_REGISTER_FIRST_CHANNEL; break;
        case 2: channel_hex = MUX_REGISTER_SECOND_CHANNEL; break;
        case 3: channel_hex = MUX_REGISTER_THIRD_CHANNEL; break;
        case 4: channel_hex = MUX_REGISTER_FOURTH_CHANNEL; break;
        default: ESP_LOGE(TAG, "Unknow channel value"); return -1;
    }

    if(channel <5)
    {
        dev = ADS1256_DEVICE_1;
    }
    else
    {
        dev = ADS1256_DEVICE_2;
    }

    if(!ads1256_change_channel(dev, channel_hex))
    {
        ESP_LOGE(TAG, "Channel change error");
        return -1;
    }

    ESP_LOGI(TAG, "Channel changed!");
    return 0;

}

int ads1256_get_sampes(int argc, char **argv)
{
    if(argc != 3)
    {
        ESP_LOGE(TAG, "Usage: command [dev_num] [nr_of_samples]");
        return -1;
    }


    int device = atoi(argv[1]);
    int samples = atoi(argv[2]);
    ads1256_device_t dev;

    if(device == 1)
    {
        dev = ADS1256_DEVICE_1;
    }
    else if(device == 2)
    {
        dev = ADS1256_DEVICE_2;
    }
    else
    {
        ESP_LOGE(TAG, "Wrong dev_num. 1 - DEV1, 2-DEV2");
        return -1;
    }

    if(samples < 1)
    {
        ESP_LOGE(TAG, "Number of samples have to be greatcher than 0");
        return -1;
    }

    uint8_t data[3];
    int32_t value;


    for(int i =0; i<samples; i++)
    {
        if(!ads1256_get_raw_data(dev, data))
        {
            return -1;
        }
        value = (data[0] << 16) | (data[1] << 8) | data[2];

        if (value & 0x800000) {
            value |= 0xFF000000;
        }

        ESP_LOGI(TAG, "Raw signed value: %d", value);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    return 0;
}

int read_cal_registers(int argc, char **argv)
{
    if(argc != 2)
    {
        ESP_LOGE(TAG, "Usage: command [dev_num]");
        return -1;
    }

    int device = atoi(argv[1]);
    ads1256_device_t dev;

    if(device == 1)
    {
        dev = ADS1256_DEVICE_1;
    }
    else if(device == 2)
    {
        dev = ADS1256_DEVICE_2;
    }
    else
    {
        ESP_LOGE(TAG, "Wrong dev_num. 1 - DEV1, 2-DEV2");
        return -1;
    }

    if(!ads1256_read_cal_registers(dev))
    {
        ESP_LOGE(TAG, "Failed to read calibration registers");
        return -1;
    }

    ESP_LOGI(TAG, "Calibration registers read successfully for device %d", device);
    return 0;
}

int calibrate_device(int argc, char **argv)
{
    if(argc != 2)
    {
        ESP_LOGE(TAG, "Usage: command [dev_num]");
        return -1;
    }
    int device = atoi(argv[1]);
    ads1256_device_t dev;
    if(device == 1)
    {
        dev = ADS1256_DEVICE_1;
    }
    else if(device == 2)
    {
        dev = ADS1256_DEVICE_2;
    }
    else
    {
        ESP_LOGE(TAG, "Wrong dev_num. 1 - DEV1, 2-DEV2");
        return -1;
    }
    if(!ads1256_self_cal(dev))
    {
        ESP_LOGE(TAG, "Failed to perform self-calibration on device %d", device);
        return -1;
    }
    ESP_LOGI(TAG, "Self-calibration completed successfully for device %d", device);

    if(!ads1256_read_cal_registers(dev))
    {
        ESP_LOGE(TAG, "Failed to read calibration registers");
        return -1;
    }
    ESP_LOGI(TAG, "Calibration registers read successfully for device %d", device);
    return 0;
}

int ads1256_reset_cli(int argc, char **argv)
{
    if(argc != 2)
    {
        ESP_LOGE(TAG, "Usage: command [dev_num]");
        return -1;
    }
    int device = atoi(argv[1]);
    ads1256_device_t dev;
    if(device == 1)
    {
        dev = ADS1256_DEVICE_1;
    }
    else if(device == 2)
    {
        dev = ADS1256_DEVICE_2;
    }
    else
    {
        ESP_LOGE(TAG, "Wrong dev_num. 1 - DEV1, 2-DEV2");
        return -1;
    }
    if(!ads1256_reset(dev))
    {
        ESP_LOGE(TAG, "Failed to reset device %d", device);
        return -1;
    }
    ESP_LOGI(TAG, "Device %d reset successfully", device);
    return 0;
}

 // Place for the console configuration

 static esp_console_cmd_t cmd [] = {
 // example command:
 // cmd     help description   hint  function      args
 {"reset", "Reset the device", NULL, reset_device, NULL},
 {"timer_test", "Start a test timer", NULL, start_timer_test, NULL},
 {"readc_task", "Start the readc task", NULL, readc_task, NULL},
{"read_sd_file", "Read a file from the SD card", NULL, read_sd_file, NULL},
{"empty_sd_file", "Empty a file on the SD card", NULL, empty_sd_file, NULL},
{"ads_samples", "Returns read data for n sec (int dev, int samples)", NULL,ads1256_get_sampes, NULL },
{"ads_change_mux", "Mux change command (int mux nr) 1-4 dev1 muxs 5-8 dev2 muxs", NULL, change_mux_channel, NULL},
{"ads_read_cal", "Read calibration registers (int dev_num)", NULL, read_cal_registers, NULL},
{"ads_calibrate", "Calibrate device (int dev_num)", NULL, calibrate_device, NULL},
{"ads_reset", "Reset device (int dev_num)", NULL, ads1256_reset_cli,NULL},
{"ads_cal_reg", "Read calibration registers (int dev_num)", NULL, read_cal_registers, NULL},
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