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
#include <string.h>

#define TAG "CONSOLE_CONFIG"

/* HELP FUNCs*/

char* add_sd_prefix(const char* path) {
    if (strncmp(path, "/sdcard/", 7) != 0) {
        char* file_path = malloc(strlen(MOUNT_POINT) + strlen(path) + 2); // +2 for '/' and '\0'
        if (!file_path) {
            ESP_LOGE(TAG, "Failed to allocate memory for file_path");
            return NULL;
        }
        snprintf(file_path, strlen(MOUNT_POINT) + strlen(path) + 2, "%s/%s", MOUNT_POINT, path);
        return file_path;
    } else {
        return strdup(path);
    }
}

/* HELP FUNCs*/

int reset_device(int argc, char **argv) {
    ESP_LOGI(TAG, "Resetting device...");
    esp_restart();
    return 0;
}
int read_mux_samples(int argc, char **argv) {
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
    ESP_LOGI(TAG, "Starting read mux samples task from cmd (forever xd) on device %d", device);
    ads1256_start_channel_task(dev);

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
        // char *path = add_sd_prefix(argv[3]);
    ESP_LOGI(TAG, "Starting readc task from cmd");

    start_readc_task(dev, time);
    return 0;
}

int read_sd_file(int argc, char **argv) {
    if(argc != 2) {
        ESP_LOGE(TAG, "Usage: command [file_path]");
        return -1;
    }
    const char *file_path = add_sd_prefix(argv[1]);
    if (!print_file(file_path)) {
        ESP_LOGE(TAG, "Failed to read file: %s", file_path);
        free((void *)file_path);
        return -1;
    }
    free((void *)file_path);
    return 0;
}

int empty_sd_file(int argc, char **argv) {
    if(argc != 2) {
        ESP_LOGE(TAG, "Usage: command [file_path]");
        return -1;
    }

    const char *file_path = add_sd_prefix(argv[1]);
    if (empty_file(file_path)) {
        ESP_LOGI(TAG, "File emptied successfully at: %s", file_path);
    } else {
        ESP_LOGE(TAG, "Failed to empty file");
    }
    return 0;
}

int change_mux_channel(int argc, char **argv)
{
    if(argc != 3)
    {
        ESP_LOGE(TAG, "Usage: command [dev_num] [channel] ");
        return -1;
    }
    int channel = atoi(argv[2]);
    int device_num = atoi(argv[1]);

    if(channel < 0 || channel > 3)
    {
        ESP_LOGE(TAG, "Channel value must be between 0 and 3");
        return -1;
    }

    ads1256_device_t dev;


    if(device_num == 1)
    {
        dev = ADS1256_DEVICE_1;
    }
    else if(device_num == 2)
    {
        dev = ADS1256_DEVICE_2;
    }
    else
    {
        ESP_LOGE(TAG, "Wrong dev_num. 1 - DEV1, 2-DEV2");
        return -1;
    }

    if(!ads1256_change_channel(dev, channel))
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
        // if(!ads1256_get_raw_data(dev, data))
        // {
        //     return -1;
        // }
        // value = (data[0] << 16) | (data[1] << 8) | data[2];

        // if (value & 0x800000) {
        //     value |= 0xFF000000;
        // }

        // ESP_LOGI(TAG, "Raw signed value: %d", value);
        float weight = 0.0f;
        uint8_t raw_data[3];
        ads1256_get_raw_data(ADS1256_DEVICE_1, raw_data);
        int32_t raw_value = (raw_data[0] << 16) | (raw_data[1] << 8) | raw_data[2];
        if (raw_value & 0x800000) {
            raw_value |= 0xFF000000; 
        }
        ESP_LOGI(TAG, "Raw signed value: %d", raw_value);
        // ads1256_raw_data_to_weight(&raw_data, ADS1256_DEVICE_1, &weight, 1);
        // ESP_LOGI("CAN_COMMANDS", "Weight from device %d [N], channel %d: %f", ads_device, channel_num, weight);
        // ESP_LOGI("CLI","waga w [n]: %f", weight );
        // uint8_t resp[4];
        // memcpy(resp, &weight, sizeof(weight));
        // esp_err_t err = can_send_message(0x3F20, resp, sizeof(resp));

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    return 0;
}

int dev_info(int argc, char **argv)
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

    ads1256_get_config_info(dev);
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

int ads1256_set_sps_cmd(int argc, char **argv)
{
    if(argc != 3)
    {
        ESP_LOGE(TAG, "Usage: command [dev_num] [sps_value]");
        return -1;
    }
    int device = atoi(argv[1]);
    int sps_value = atoi(argv[2]);
    ads1256_device_t dev;
    uint8_t sps_register_value;

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

    switch(sps_value)
    {
        case 205: sps_register_value = DATA_RATE_REGISTER_2P5SPS; break;
        case 5: sps_register_value = DATA_RATE_REGISTER_5SPS; break;
        case 10: sps_register_value = DATA_RATE_REGISTER_10SPS; break;
        case 25: sps_register_value = DATA_RATE_REGISTER_25SPS; break;
        case 50: sps_register_value = DATA_RATE_REGISTER_50SPS; break;
        case 100: sps_register_value = DATA_RATE_REGISTER_100SPS; break;
        case 500: sps_register_value = DATA_RATE_REGISTER_500SPS; break;
        case 1000: sps_register_value = DATA_RATE_REGISTER_1000SPS; break;
        case 2000: sps_register_value = DATA_RATE_REGISTER_2000SPS; break;
        case 3750: sps_register_value = DATA_RATE_REGISTER_3750SPS; break;
        case 7500: sps_register_value = DATA_RATE_REGISTER_7500SPS; break;
        case 15000: sps_register_value = DATA_RATE_REGISTER_15000SPS; break;
        case 30000: sps_register_value = DATA_RATE_REGISTER_30000SPS; break;
        default: 
            ESP_LOGE(TAG, "Invalid SPS value. Valid values are: 2.5 (205), 5, 10, 25, 50, 100, 500, 1000, 2000, 3750, 7500, 15000, 30000");
            return -1;
    }

    if(!ads1256_set_sps(dev, sps_register_value))
    {
        ESP_LOGE(TAG, "Failed to set data rate on device %d", device);
        return -1;
    }

    return 1;
}

int print_data(int argc, char **argv) {
    if(argc != 2) {
        ESP_LOGE(TAG, "Usage: command [dev_num]");
        return -1;
    }

    int device = atoi(argv[1]);
    ads1256_device_t dev;

    if(device == 1) {
        dev = ADS1256_DEVICE_1;
    } else if(device == 2) {
        dev = ADS1256_DEVICE_2;
    } else {
        ESP_LOGE(TAG, "Wrong dev_num. 1 - DEV1, 2-DEV2");
        return -1;
    }

    ads1256_print_data(dev);
    return 0;
}

int suspend_task(int argc, char **argv) {
    if(argc != 2) {
        ESP_LOGE(TAG, "Usage: command [dev_num]");
        return -1;
    }

    int device = atoi(argv[1]);
    ads1256_device_t dev;

    if(device == 1) {
        dev = ADS1256_DEVICE_1;
    } else if(device == 2) {
        dev = ADS1256_DEVICE_2;
    } else {
        ESP_LOGE(TAG, "Wrong dev_num. 1 - DEV1, 2-DEV2");
        return -1;
    }

    ads1256_suspend_task(dev);
    return 0;
}

int resume_task(int argc, char **argv) {
    if(argc != 2) {
        ESP_LOGE(TAG, "Usage: command [dev_num]");
        return -1;
    }

    int device = atoi(argv[1]);
    ads1256_device_t dev;

    if(device == 1) {
        dev = ADS1256_DEVICE_1;
    } else if(device == 2) {
        dev = ADS1256_DEVICE_2;
    } else {
        ESP_LOGE(TAG, "Wrong dev_num. 1 - DEV1, 2-DEV2");
        return -1;
    }

    ads1256_resume_task(dev);
    return 0;
}

int dlete_task(int argc, char **argv) {
    if(argc != 2) {
        ESP_LOGE(TAG, "Usage: command [dev_num]");
        return -1;
    }

    int device = atoi(argv[1]);
    ads1256_device_t dev;

    if(device == 1) {
        dev = ADS1256_DEVICE_1;
    } else if(device == 2) {
        dev = ADS1256_DEVICE_2;
    } else {
        ESP_LOGE(TAG, "Wrong dev_num. 1 - DEV1, 2-DEV2");
        return -1;
    }

    ads1256_delete_task(dev);
    return 0;
}

int help_cmd(int argc, char **argv);



 // Place for the console configuration

 static esp_console_cmd_t cmd [] = {
 // example command:
 // cmd     help description   hint  function      args
 {"reset", "Reset the device", NULL, reset_device, NULL, NULL, NULL},
 {"ads_readc", "Run ads readc func for a [n] seconds. Usage: ads_readc [dev_num] [time_s] [file_path]", NULL, readc_task, NULL, NULL, NULL},
{"sd_read_file", "Print file on std out from sd. Usage: sd_read_file [file_path]", NULL, read_sd_file, NULL, NULL, NULL},
{"sd_clear_file", "Empty a file on the SD card. Usage: sd_clear_file [file_path]", NULL, empty_sd_file, NULL, NULL, NULL},
{"ads_samples", "Returns measurements for n sec (1Hz). Usage: ads_samples [dev_num] [time]", NULL,ads1256_get_sampes, NULL, NULL, NULL},
{"ads_change_mux", "Change ads channel. Usage: ads_change_mux [dev_num] [0-3]", NULL, change_mux_channel, NULL, NULL, NULL},
{"ads_read_cal", "Read calibration registers. Usage: ads_read_cal", NULL, read_cal_registers, NULL, NULL, NULL},
{"ads_calibrate", "Calibrate device on current channel. Usage: ads_calibrate [dev_num]", NULL, calibrate_device, NULL, NULL, NULL},
{"ads_reset", "Reset ads device. Usage: ads_reset [dev_num]", NULL, ads1256_reset_cli,NULL, NULL, NULL},
{"ads_set_sps", "Set data rate for ads device. Usage: ads_set_sps [dev_num] [sps_value]", NULL, ads1256_set_sps_cmd, NULL, NULL, NULL},
{"read_mux_samples", "Read samples from the ADS1256 MUX. Usage: read_mux_samples [dev_num] [nr_of_samples]", NULL, read_mux_samples, NULL, NULL, NULL},
{"dev_info", "Display device configuration information. Usage: dev_info [dev_num]", NULL, dev_info, NULL, NULL, NULL},
{"ads_print_data", "Print data from ADS1256 device. Usage: ads_print_data [dev_num]", NULL, print_data, NULL, NULL, NULL},
{"help", "Display this help message", NULL, help_cmd, NULL, NULL, NULL},
{"ads_suspend_task", "Suspend ADS1256 task. Usage: ads_suspend_task [dev_num]", NULL, suspend_task, NULL, NULL, NULL},
{"ads_resume_task", "Resume ADS1256 task. Usage: ads_resume_task [dev_num]", NULL, resume_task, NULL, NULL, NULL},
{"ads_delete_task", "Delete ADS1256 task. Usage: ads_delete_task [dev_num]", NULL, dlete_task, NULL, NULL, NULL},




};

int help_cmd(int argc, char **argv) {
    ESP_LOGI(TAG, "Available commands:");
    for (int i = 0; i < sizeof(cmd) / sizeof(cmd[0]); i++) {
        ESP_LOGI(TAG, "%-16s - %s", cmd[i].command, cmd[i].help);
    }
    return 0;
}


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