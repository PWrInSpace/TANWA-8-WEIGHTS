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
#include <stdlib.h>
#include "flash.h"

#define TAG "CONSOLE_CONFIG"

static int          g_cmd_count = 0;
static console_cmd_ex_t *g_cmd_list = NULL;

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

int tare_cmd(int argc, char **argv) {
    if (argc != 1) {
        ESP_LOGE(TAG, "Usage: tare");
        return 0;
    }

    if (!ads1256_tare_all(ADS1256_DEVICE_1)) {
        ESP_LOGE(TAG, "Tare failed");
        return 0;
    }

    ESP_LOGI(TAG, "Tare complete");
    return 0;
}

int calibrate_cmd(int argc, char **argv){
    if(argc != 3) {
        ESP_LOGE(TAG, "Usage: calibrate <channel> <weight>");
        return 0;
    }

    int channel = atoi(argv[1]);
    float weight = (float)atof(argv[2]);

    if (channel < 0 || channel > 3){
        ESP_LOGE(TAG, "Channel must be in range 0...3");
        return 0;
    }

    if (weight<=0.0f){
        ESP_LOGE(TAG,"Weight must be > 0 (use tare for zero weight)");
        return 0;
    }

    if (!ads1256_calibrate_channel(ADS1256_DEVICE_1,(uint8_t)channel, weight)){
        ESP_LOGE(TAG, "Calibration failed");
        return 0;
    }

    ESP_LOGI(TAG, "Calibration compelte");
    return 0;

}

static void print_config(const data_config_t *cfg, const char *label) {
    printf("%s\n", label);
    flash_print_config(*cfg);
    printf("\n");
}

int read_flash_cmd(int argc, char **argv) {
    (void)argc;
    (void)argv;

    data_config_t data;
    if (flash_read(&data) != ESP_OK) {
        printf("Couldn't retrieve data from flash memory\n");
        return 0;
    }

    print_config(&data, "Memory contents:");
    return 0;
}

int display_config_cmd(int argc, char **argv) {
    (void)argc;
    (void)argv;

    data_config_t data;
    if (flash_get_runtime_config(&data) != ESP_OK) {
        printf("Couldn't retrieve runtime config\n");
        return 0;
    }

    print_config(&data, "Runtime config:");
    return 0;
}

int save_flash_cmd(int argc, char **argv) {
    esp_err_t ret;
    ret = flash_commit();

    if (ret != ESP_OK) {
        printf("Couldn't save data to flash memory\nErr: %s\n", esp_err_to_name(ret));
        return 0;
    }

    printf("Successfully saved data to flash memory\n");
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

int read_id(int argc, char **argv)
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

    uint8_t id;
    for(int i =0; i<50; i++)
    {
        if(!ads1256_read_id(dev, &id))
        {
            ESP_LOGE(TAG, "Failed to read ID from device %d", device);
            return -1;
        }
        esp_rom_delay_us(10);
    }

    ESP_LOGI(TAG, "Device %d ID: 0x%02X", device, id);
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

static esp_err_t setup_commands(int *cmd_count, console_cmd_ex_t **cmd_list) {
    static console_cmd_ex_t cmd[] = {
        {
            .cmd = {
                .command = "reset",
                .help = "Reset the device",
                .hint = NULL,
                .func = reset_device,
                .argtable = NULL,
                .func_w_context = NULL,
                .context = NULL,
            },
            .arg_completion = NULL,
        },
        {
            .cmd = {
                .command = "ads_readc",
                .help = "Run ads readc func for a [n] seconds. Usage: ads_readc [dev_num] [time_s] [file_path]",
                .hint = NULL,
                .func = readc_task,
                .argtable = NULL,
                .func_w_context = NULL,
                .context = NULL,
            },
            .arg_completion = NULL,
        },
        {
            .cmd = {
                .command = "sd_read_file",
                .help = "Print file on std out from sd. Usage: sd_read_file [file_path]",
                .hint = NULL,
                .func = read_sd_file,
                .argtable = NULL,
                .func_w_context = NULL,
                .context = NULL,
            },
            .arg_completion = NULL,
        },
        {
            .cmd = {
                .command = "sd_clear_file",
                .help = "Empty a file on the SD card. Usage: sd_clear_file [file_path]",
                .hint = NULL,
                .func = empty_sd_file,
                .argtable = NULL,
                .func_w_context = NULL,
                .context = NULL,
            },
            .arg_completion = NULL,
        },
        {
            .cmd = {
                .command = "ads_samples",
                .help = "Returns measurements for n sec (1Hz). Usage: ads_samples [dev_num] [time]",
                .hint = NULL,
                .func = ads1256_get_sampes,
                .argtable = NULL,
                .func_w_context = NULL,
                .context = NULL,
            },
            .arg_completion = NULL,
        },
        {
            .cmd = {
                .command = "ads_change_mux",
                .help = "Change ads channel. Usage: ads_change_mux [dev_num] [0-3]",
                .hint = NULL,
                .func = change_mux_channel,
                .argtable = NULL,
                .func_w_context = NULL,
                .context = NULL,
            },
            .arg_completion = NULL,
        },
        {
            .cmd = {
                .command = "ads_read_cal",
                .help = "Read calibration registers. Usage: ads_read_cal",
                .hint = NULL,
                .func = read_cal_registers,
                .argtable = NULL,
                .func_w_context = NULL,
                .context = NULL,
            },
            .arg_completion = NULL,
        },
        {
            .cmd = {
                .command = "ads_calibrate",
                .help = "Calibrate device on current channel. Usage: ads_calibrate [dev_num]",
                .hint = NULL,
                .func = calibrate_device,
                .argtable = NULL,
                .func_w_context = NULL,
                .context = NULL,
            },
            .arg_completion = NULL,
        },
        {
            .cmd = {
                .command = "ads_reset",
                .help = "Reset ads device. Usage: ads_reset [dev_num]",
                .hint = NULL,
                .func = ads1256_reset_cli,
                .argtable = NULL,
                .func_w_context = NULL,
                .context = NULL,
            },
            .arg_completion = NULL,
        },
        {
            .cmd = {
                .command = "ads_set_sps",
                .help = "Set data rate for ads device. Usage: ads_set_sps [dev_num] [sps_value]",
                .hint = NULL,
                .func = ads1256_set_sps_cmd,
                .argtable = NULL,
                .func_w_context = NULL,
                .context = NULL,
            },
            .arg_completion = NULL,
        },
        {
            .cmd = {
                .command = "ads_read_mux_samples",
                .help = "Read samples from the ADS1256 MUX. Usage: ads_read_mux_samples [dev_num] [nr_of_samples]",
                .hint = NULL,
                .func = read_mux_samples,
                .argtable = NULL,
                .func_w_context = NULL,
                .context = NULL,
            },
            .arg_completion = NULL,
        },
        {
            .cmd = {
                .command = "ads_dev_info",
                .help = "Display device configuration information. Usage: ads_dev_info [dev_num]",
                .hint = NULL,
                .func = dev_info,
                .argtable = NULL,
                .func_w_context = NULL,
                .context = NULL,
            },
            .arg_completion = NULL,
        },
        {
            .cmd = {
                .command = "ads_print_data",
                .help = "Print data from ADS1256 device. Usage: ads_print_data [dev_num]",
                .hint = NULL,
                .func = print_data,
                .argtable = NULL,
                .func_w_context = NULL,
                .context = NULL,
            },
            .arg_completion = NULL,
        },
        {
            .cmd = {
                .command = "help",
                .help = "Display this help message",
                .hint = NULL,
                .func = help_cmd,
                .argtable = NULL,
                .func_w_context = NULL,
                .context = NULL,
            },
            .arg_completion = NULL,
        },
        {
            .cmd = {
                .command = "ads_suspend_task",
                .help = "Suspend ADS1256 task. Usage: ads_suspend_task [dev_num]",
                .hint = NULL,
                .func = suspend_task,
                .argtable = NULL,
                .func_w_context = NULL,
                .context = NULL,
            },
            .arg_completion = NULL,
        },
        {
            .cmd = {
                .command = "ads_resume_task",
                .help = "Resume ADS1256 task. Usage: ads_resume_task [dev_num]",
                .hint = NULL,
                .func = resume_task,
                .argtable = NULL,
                .func_w_context = NULL,
                .context = NULL,
            },
            .arg_completion = NULL,
        },
        {
            .cmd = {
                .command = "ads_delete_task",
                .help = "Delete ADS1256 task. Usage: ads_delete_task [dev_num]",
                .hint = NULL,
                .func = dlete_task,
                .argtable = NULL,
                .func_w_context = NULL,
                .context = NULL,
            },
            .arg_completion = NULL,
        },
        {
            .cmd = {
                .command = "ads_read_id",
                .help = "Read ID from ADS1256 device. Usage: ads_read_id [dev_num]",
                .hint = NULL,
                .func = read_id,
                .argtable = NULL,
                .func_w_context = NULL,
                .context = NULL,
            },
            .arg_completion = NULL,
        },
        {
            .cmd = {
                .command = "ads_tare",
                .help = "Zero all sensors. Usage: ads_tare",
                .hint = NULL,
                .func = tare_cmd,
                .argtable = NULL,
                .func_w_context = NULL,
                .context = NULL,
            },
            .arg_completion = NULL,
        },
        {
            .cmd = {
                .command = "ads_calibrate_channel",
                .help = "Calibrate one channel. Usage: ads_calibrate_channel <channel> <weight>",
                .hint = NULL,
                .func = calibrate_cmd,
                .argtable = NULL,
                .func_w_context = NULL,
                .context = NULL,
            },
            .arg_completion = NULL,
        },
        {
            .cmd = {
                .command = "read_flash",
                .help = "Reads and displays saved data in flash memory.",
                .hint = NULL,
                .func = read_flash_cmd,
                .argtable = NULL,
                .func_w_context = NULL,
                .context = NULL,
            },
            .arg_completion = NULL,
        },
        {
            .cmd = {
                .command = "display_config",
                .help = "Displays current runtime config (RAM).",
                .hint = NULL,
                .func = display_config_cmd,
                .argtable = NULL,
                .func_w_context = NULL,
                .context = NULL,
            },
            .arg_completion = NULL,
        },
        {
            .cmd = {
                .command = "save_flash",
                .help = "Saves current runtime config to flash memory.",
                .hint = NULL,
                .func = save_flash_cmd,
                .argtable = NULL,
                .func_w_context = NULL,
                .context = NULL,
            },
            .arg_completion = NULL,
        },
    };

    *cmd_count = sizeof(cmd) / sizeof(cmd[0]);
    *cmd_list = cmd;
    return ESP_OK;
}

static bool starts_with(const char *text, const char *prefix) {
    return text != NULL && prefix != NULL && strncmp(text, prefix, strlen(prefix)) == 0;
}

static bool is_ads_command(const console_cmd_ex_t *cmd) {
    return cmd != NULL && starts_with(cmd->cmd.command, "ads_");
}

static bool is_sd_command(const console_cmd_ex_t *cmd) {
    return cmd != NULL && starts_with(cmd->cmd.command, "sd_");
}

static bool is_system_command(const console_cmd_ex_t *cmd) {
    return cmd != NULL && !is_ads_command(cmd) && !is_sd_command(cmd);
}

static void print_command_section(const char *title,
                                  bool (*predicate)(const console_cmd_ex_t *),
                                  const console_cmd_ex_t *cmds,
                                  size_t cmd_count) {
    bool printed_any = false;

    printf("========== %s ==========\n", title);

    for (size_t i = 0; i < cmd_count; i++) {
        if (predicate(&cmds[i])) {
            printf("%-20s - %s\n", cmds[i].cmd.command, cmds[i].cmd.help);
            printed_any = true;
        }
    }

    if (!printed_any) {
        printf("(no commands)\n");
    }

    printf("\n");
}


int help_cmd(int argc, char **argv) {
    (void)argv;

    if (g_cmd_list == NULL) {
        ESP_LOGE(TAG, "Commands not initialized");
        return 0;
    }

    if (argc == 1) {
        ESP_LOGI(TAG, "Available commands:");
        print_command_section("System", is_system_command, g_cmd_list, g_cmd_count);
        print_command_section("ADS", is_ads_command, g_cmd_list, g_cmd_count);
        print_command_section("SD", is_sd_command, g_cmd_list, g_cmd_count);
        return 0;
    }

    if (argc == 2) {
        for (size_t i = 0; i < (size_t)g_cmd_count; i++) {
            if (strcmp(g_cmd_list[i].cmd.command, argv[1]) == 0) {
                printf("%s\n", g_cmd_list[i].cmd.command);
                printf("  %s\n", g_cmd_list[i].cmd.help);
                return 0;
            }
        }

        ESP_LOGE(TAG, "Unknown command: %s", argv[1]);
        return 0;
    }

    ESP_LOGE(TAG, "Usage: help [command]");
    return 0;
}

esp_err_t console_config_init() {
    esp_err_t ret = console_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "%s", esp_err_to_name(ret));
        return ret;
    }

    ret = setup_commands(&g_cmd_count, &g_cmd_list);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to setup commands");
        return ret;
    }

    ret = console_register_commands(g_cmd_list, g_cmd_count);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "%s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}