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
#include "board_config.h"
#include <string.h>
#include <stdlib.h>
#include "flash.h"

#define TAG "CONSOLE_CONFIG"

static int          g_cmd_count = 0;
static console_cmd_ex_t *g_cmd_list = NULL;
static TaskHandle_t pw_task_handle = NULL;
static volatile bool pw_stop_flag = false;

/* HELP FUNCs*/

char* add_sd_prefix(const char* path) {
    if (strncmp(path, "/sdcard/", 7) != 0) {
        char* file_path = malloc(strlen(MOUNT_POINT) + strlen(path) + 2);
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

int reset_device(int argc, char **argv) {
    ESP_LOGI(TAG, "Resetting device...");
    esp_restart();
    return 0;
}

int tare_cmd(int argc, char **argv) {
    int device = 1;
    int channel = -1;

    if (argc == 2) {
        int arg_val = atoi(argv[1]);
        if (arg_val >= 0 && arg_val <= 3) {
            device = 1;
            channel = arg_val;
        } else {
            device = arg_val;
        }
    } else if (argc >= 3) {
        device = atoi(argv[1]);
        channel = atoi(argv[2]);
    }

    ads1256_wrapper_t* w = board_get_ads1256(device);
    if (w == NULL) {
        ESP_LOGE(TAG, "Device %d not found", device);
        return 0;
    }

    if (channel == -1) {
        if (!ads1256_tare_all(w)) {
            ESP_LOGE(TAG, "Tare all failed");
            return 0;
        }
        ESP_LOGI(TAG, "Tare all channels complete for device %d", device);
    } else {
        if (channel < 0 || channel > 3) {
            ESP_LOGE(TAG, "Channel must be in range 0...3");
            return 0;
        }

        if (!ads1256_tare_channel(w, (uint8_t)channel)) {
            ESP_LOGE(TAG, "Tare channel %d failed", channel);
            return 0;
        }
        ESP_LOGI(TAG, "Tare channel %d complete for device %d", channel, device);
    }

    {
        data_config_t cfg;
        if (flash_get_runtime_config(&cfg) == ESP_OK) {
            ads1256_calibration_t cal[4];
            ads1256_get_calibration(w, cal);
            if (device == 1) {
                cfg.weight_cfg.zero_offset_1 = cal[0].zero_offset;
                cfg.weight_cfg.zero_offset_2 = cal[1].zero_offset;
                cfg.weight_cfg.zero_offset_3 = cal[2].zero_offset;
                cfg.weight_cfg.zero_offset_4 = cal[3].zero_offset;
            }
            flash_edit_config(cfg);
        }
    }

    ESP_LOGI(TAG, "Tare OK. Use flash_save_config to persist.");
    return 0;
}

int edit_flash_cmd(int argc, char **argv) {
    if (argc != 3) {
        ESP_LOGE(TAG, "Usage: flash_edit_config <key> <value>");
        return 0;
    }

    if (flash_edit_field(argv[1], argv[2]) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to edit field %s", argv[1]);
        return 0;
    }

    ESP_LOGI(TAG, "Field %s updated in RAM config", argv[1]);
    return 0;
}

int restore_defaults_cmd(int argc, char **argv) {
    (void)argc;
    (void)argv;

    if (flash_restore_defaults() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to restore default config");
        return 0;
    }

    ESP_LOGI(TAG, "Default config restored to RAM");
    return 0;
}

int erase_flash_cmd(int argc, char **argv) {
    if (argc != 2 || strcmp(argv[1], "Y") != 0) {
        ESP_LOGE(TAG, "Usage: flash_erase Y (confirmation required)");
        return 0;
    }

    if (flash_erase_config() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to erase flash config");
        return 0;
    }

    ESP_LOGI(TAG, "Flash config erased");
    return 0;
}

int calibrate_cmd(int argc, char **argv){
    if(argc != 4) {
        ESP_LOGE(TAG, "Usage: calibrate [dev_num] <channel> <weight>");
        return 0;
    }

    int device = atoi(argv[1]);
    int channel = atoi(argv[2]);
    float weight = (float)atof(argv[3]);

    ads1256_wrapper_t* w = board_get_ads1256(device);
    if (w == NULL) {
        ESP_LOGE(TAG, "Device %d not found", device);
        return 0;
    }
    if (channel < 0 || channel > 3){
        ESP_LOGE(TAG, "Channel must be in range 0...3");
        return 0;
    }
    if (weight<=0.0f){
        ESP_LOGE(TAG,"Weight must be > 0 (use tare for zero weight)");
        return 0;
    }

    if (!ads1256_calibrate_channel(w,(uint8_t)channel, weight)){
        ESP_LOGE(TAG, "Calibration failed");
        return 0;
    }

    {
        data_config_t cfg;
        if (flash_get_runtime_config(&cfg) == ESP_OK) {
            ads1256_calibration_t cal[4];
            ads1256_get_calibration(w, cal);
            cfg.weight_cfg.factor_1 = cal[0].factor;
            cfg.weight_cfg.factor_2 = cal[1].factor;
            cfg.weight_cfg.factor_3 = cal[2].factor;
            cfg.weight_cfg.factor_4 = cal[3].factor;
            flash_edit_config(cfg);
        }
    }

    ESP_LOGI(TAG, "Calibration complete. Use save_flash to persist.");
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

    ads1256_wrapper_t* w = board_get_ads1256(device);
    if (w == NULL)
    {
        ESP_LOGE(TAG, "Device %d not found", device);
        return -1;
    }
    if(time < 1)
    {
        ESP_LOGE(TAG, "Time value must be greater than 0");
        return -1;
    }
    ESP_LOGI(TAG, "Starting read mux samples task from cmd (forever xd) on device %d", device);
    if (!ads1256_start_channel_task(w)) {
        ESP_LOGE(TAG, "Failed to start channel task");
        return -1;
    }

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

    ads1256_wrapper_t* w = board_get_ads1256(device);
    if (w == NULL)
    {
        ESP_LOGE(TAG, "Device %d not found", device);
        return -1;
    }

    if(time < 1)
    {
        ESP_LOGE(TAG, "Time value must be greater than 0");
        return -1;
    }
    ESP_LOGI(TAG, "Starting readc task from cmd");

    if (!start_readc_task(w, time)) {
        ESP_LOGE(TAG, "Failed to start readc task");
        return 0;
    }

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
    int device_num = atoi(argv[1]);
    int channel = atoi(argv[2]);

    ads1256_wrapper_t* w = board_get_ads1256(device_num);
    if (w == NULL)
    {
        ESP_LOGE(TAG, "Device %d not found", device_num);
        return -1;
    }
    if(channel < 0 || channel > 3)
    {
        ESP_LOGE(TAG, "Channel value must be between 0 and 3");
        return -1;
    }

    if (!ads1256_stop_task_and_wait(w, pdMS_TO_TICKS(2000))) {
        ESP_LOGE(TAG, "Failed to stop task");
        return -1;
    }

    if(!ads1256_change_channel(w, channel))
    {
        ESP_LOGE(TAG, "Channel change error");
        ads1256_start_channel_task(w);
        return -1;
    }

    ads1256_start_channel_task(w);

    ESP_LOGI(TAG, "Channel changed!");
    return 0;
}

int ads1256_get_samples(int argc, char **argv)
{
    if(argc != 3)
    {
        ESP_LOGE(TAG, "Usage: command [dev_num] [nr_of_samples]");
        return -1;
    }

    int device = atoi(argv[1]);
    int samples = atoi(argv[2]);

    ads1256_wrapper_t* w = board_get_ads1256(device);
    if (w == NULL)
    {
        ESP_LOGE(TAG, "Device %d not found", device);
        return -1;
    }
    if(samples < 1)
    {
        ESP_LOGE(TAG, "Number of samples have to be greater than 0");
        return -1;
    }

    if (!ads1256_stop_task_and_wait(w, pdMS_TO_TICKS(2000))) {
        ESP_LOGE(TAG, "Failed to stop task");
        return -1;
    }

    for(int i =0; i<samples; i++)
    {
        uint8_t raw_data[3];
        ads1256_get_raw_data(ads1256_wrapper_get_dev(w), raw_data);
        int32_t raw_value = (raw_data[0] << 16) | (raw_data[1] << 8) | raw_data[2];
        if (raw_value & 0x800000) {
            raw_value |= 0xFF000000;
        }
        ESP_LOGI(TAG, "Raw signed value: %d", raw_value);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ads1256_start_channel_task(w);
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

    ads1256_wrapper_t* w = board_get_ads1256(device);
    if (w == NULL)
    {
        ESP_LOGE(TAG, "Device %d not found", device);
        return -1;
    }

    if (!ads1256_stop_task_and_wait(w, pdMS_TO_TICKS(2000))) {
        ESP_LOGE(TAG, "Failed to stop task");
        return -1;
    }

    uint8_t id;
    for(int i =0; i<50; i++)
    {
        if(!ads1256_read_id(ads1256_wrapper_get_dev(w), &id))
        {
            ESP_LOGE(TAG, "Failed to read ID from device %d", device);
            ads1256_start_channel_task(w);
            return -1;
        }
        esp_rom_delay_us(10);
    }

    ESP_LOGI(TAG, "Device %d ID: 0x%02X", device, id);
    ads1256_start_channel_task(w);
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

    ads1256_wrapper_t* w = board_get_ads1256(device);
    if (w == NULL)
    {
        ESP_LOGE(TAG, "Device %d not found", device);
        return -1;
    }

    ads1256_get_config_info(w);
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

    ads1256_wrapper_t* w = board_get_ads1256(device);
    if (w == NULL)
    {
        ESP_LOGE(TAG, "Device %d not found", device);
        return -1;
    }

    if (!ads1256_stop_task_and_wait(w, pdMS_TO_TICKS(2000))) {
        ESP_LOGE(TAG, "Failed to stop task");
        return -1;
    }

    if(!ads1256_read_cal_registers(ads1256_wrapper_get_dev(w)))
    {
        ESP_LOGE(TAG, "Failed to read calibration registers");
        ads1256_start_channel_task(w);
        return -1;
    }

    ESP_LOGI(TAG, "Calibration registers read successfully for device %d", device);
    ads1256_start_channel_task(w);
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

    ads1256_wrapper_t* w = board_get_ads1256(device);
    if (w == NULL)
    {
        ESP_LOGE(TAG, "Device %d not found", device);
        return -1;
    }

    if (!ads1256_stop_task_and_wait(w, pdMS_TO_TICKS(2000))) {
        ESP_LOGE(TAG, "Failed to stop task");
        return -1;
    }

    if(!ads1256_send_command(ads1256_wrapper_get_dev(w), SELFCAL_COMMAND))
    {
        ESP_LOGE(TAG, "Failed to perform self-calibration on device %d", device);
        ads1256_start_channel_task(w);
        return -1;
    }
    ESP_LOGI(TAG, "Self-calibration completed successfully for device %d", device);

    if(!ads1256_read_cal_registers(ads1256_wrapper_get_dev(w)))
    {
        ESP_LOGE(TAG, "Failed to read calibration registers");
        ads1256_start_channel_task(w);
        return -1;
    }
    ESP_LOGI(TAG, "Calibration registers read successfully for device %d", device);
    ads1256_start_channel_task(w);
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

    ads1256_wrapper_t* w = board_get_ads1256(device);
    if (w == NULL)
    {
        ESP_LOGE(TAG, "Device %d not found", device);
        return -1;
    }

    if (!ads1256_stop_task_and_wait(w, pdMS_TO_TICKS(2000))) {
        ESP_LOGE(TAG, "Failed to stop task");
        return -1;
    }

    if(!ads1256_send_command(ads1256_wrapper_get_dev(w), RESET_COMMAND))
    {
        ESP_LOGE(TAG, "Failed to reset device %d", device);
        ads1256_start_channel_task(w);
        return -1;
    }
    ESP_LOGI(TAG, "Device %d reset successfully", device);
    ads1256_start_channel_task(w);
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
    double sps_raw = strtod(argv[2], NULL);
    int sps_x10 = (int)(sps_raw * 10 + 0.5);
    uint8_t sps_register_value;

    ads1256_wrapper_t* w = board_get_ads1256(device);
    if (w == NULL)
    {
        ESP_LOGE(TAG, "Device %d not found", device);
        return -1;
    }

    switch(sps_x10)
    {
        case 25:     sps_register_value = DATA_RATE_REGISTER_2P5SPS; break;    // 2.5 SPS
        case 50:     sps_register_value = DATA_RATE_REGISTER_5SPS; break;      // 5 SPS
        case 100:    sps_register_value = DATA_RATE_REGISTER_10SPS; break;     // 10 SPS
        case 250:    sps_register_value = DATA_RATE_REGISTER_25SPS; break;     // 25 SPS
        case 500:    sps_register_value = DATA_RATE_REGISTER_50SPS; break;     // 50 SPS
        case 1000:   sps_register_value = DATA_RATE_REGISTER_100SPS; break;    // 100 SPS
        case 5000:   sps_register_value = DATA_RATE_REGISTER_500SPS; break;    // 500 SPS
        case 10000:  sps_register_value = DATA_RATE_REGISTER_1000SPS; break;   // 1000 SPS
        case 20000:  sps_register_value = DATA_RATE_REGISTER_2000SPS; break;   // 2000 SPS
        case 37500:  sps_register_value = DATA_RATE_REGISTER_3750SPS; break;   // 3750 SPS
        case 75000:  sps_register_value = DATA_RATE_REGISTER_7500SPS; break;   // 7500 SPS
        case 150000: sps_register_value = DATA_RATE_REGISTER_15000SPS; break;  // 15000 SPS
        case 300000: sps_register_value = DATA_RATE_REGISTER_30000SPS; break;  // 30000 SPS
        default:
            ESP_LOGE(TAG, "Invalid SPS value. Valid values are: 2.5, 5, 10, 25, 50, 100, 500, 1000, 2000, 3750, 7500, 15000, 30000");
            return -1;
    }

    if (!ads1256_stop_task_and_wait(w, pdMS_TO_TICKS(2000))) {
        ESP_LOGE(TAG, "Failed to stop task");
        return -1;
    }

    if(!ads1256_wrapper_set_sps(w, (ads1256_sps_e)sps_register_value))
    {
        ESP_LOGE(TAG, "Failed to set data rate on device %d", device);
        ads1256_start_channel_task(w);
        return -1;
    }

    ads1256_start_channel_task(w);
    return 0;
}

int print_data(int argc, char **argv) {
    if(argc != 2) {
        ESP_LOGE(TAG, "Usage: command [dev_num]");
        return -1;
    }

    int device = atoi(argv[1]);

    ads1256_wrapper_t* w = board_get_ads1256(device);
    if (w == NULL) {
        ESP_LOGE(TAG, "Device %d not found", device);
        return -1;
    }

    ads1256_print_data(w);
    return 0;
}

int pw_cmd(int argc, char **argv) {
    (void)argc;
    (void)argv;

    char* cmd[] = {"pw", "1"};
    return print_data(2, cmd);
}

static void pw_periodic_task(void *arg) {
    ads1256_wrapper_t* w = (ads1256_wrapper_t*)arg;
    pw_stop_flag = false;

    while (!pw_stop_flag) {
        ads1256_print_data(w);
        printf("---------------------------------------------------\n");
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    pw_task_handle = NULL;
    ESP_LOGI(TAG, "Periodic weight printing stopped");
    vTaskDelete(NULL);
}

int start_pw_cmd(int argc, char **argv) {
    (void)argc;
    (void)argv;

    if (pw_task_handle != NULL) {
        ESP_LOGW(TAG, "pw task is already running, use stop_pw first");
        return 0;
    }

    ads1256_wrapper_t* w = board_get_ads1256(1);
    if (w == NULL) {
        ESP_LOGE(TAG, "Device 1 not found");
        return 0;
    }

    if (xTaskCreate(pw_periodic_task, "pw_task", 4096, w, 5, &pw_task_handle) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create pw task");
        return 0;
    }

    ESP_LOGI(TAG, "Periodic weight printing started (every 0.5s)");
    return 0;
}

int stop_pw_cmd(int argc, char **argv) {
    (void)argc;
    (void)argv;

    if (pw_task_handle == NULL) {
        ESP_LOGW(TAG, "pw task is not running");
        return 0;
    }

    pw_stop_flag = true;
    return 0;
}

int stop_task(int argc, char **argv) {
    if(argc != 2) {
        ESP_LOGE(TAG, "Usage: command [dev_num]");
        return -1;
    }

    int device = atoi(argv[1]);

    ads1256_wrapper_t* w = board_get_ads1256(device);
    if (w == NULL) {
        ESP_LOGE(TAG, "Device %d not found", device);
        return -1;
    }

    if (!ads1256_stop_task_and_wait(w, pdMS_TO_TICKS(2000))) {
        ESP_LOGE(TAG, "Failed to stop task");
        return -1;
    }
    return 0;
}

int start_task(int argc, char **argv) {
    if(argc != 2) {
        ESP_LOGE(TAG, "Usage: command [dev_num]");
        return -1;
    }

    int device = atoi(argv[1]);

    ads1256_wrapper_t* w = board_get_ads1256(device);
    if (w == NULL) {
        ESP_LOGE(TAG, "Device %d not found", device);
        return -1;
    }

    if (!ads1256_start_channel_task(w)) {
        ESP_LOGE(TAG, "Failed to start task");
        return -1;
    }
    return 0;
}

int delete_task(int argc, char **argv) {
    if(argc != 2) {
        ESP_LOGE(TAG, "Usage: command [dev_num]");
        return -1;
    }

    int device = atoi(argv[1]);

    ads1256_wrapper_t* w = board_get_ads1256(device);
    if (w == NULL) {
        ESP_LOGE(TAG, "Device %d not found", device);
        return -1;
    }

    ads1256_delete_task(w);
    return 0;
}

int help_cmd(int argc, char **argv);

static esp_err_t setup_commands(int *cmd_count, console_cmd_ex_t **cmd_list) {
    static console_cmd_ex_t cmd[] = {
        { {"reset",                  "Reset the device",                                                                  NULL, reset_device,          NULL, NULL, NULL}, NULL },
        { {"ads_readc",              "Run ads readc func for a [n] seconds. Usage: ads_readc [dev_num] [time_s] [file_path]", NULL, readc_task,            NULL, NULL, NULL}, NULL },
        { {"sd_read_file",           "Print file on std out from sd. Usage: sd_read_file [file_path]",                   NULL, read_sd_file,          NULL, NULL, NULL}, NULL },
        { {"sd_clear_file",          "Empty a file on the SD card. Usage: sd_clear_file [file_path]",                    NULL, empty_sd_file,         NULL, NULL, NULL}, NULL },
        { {"ads_samples",            "Returns measurements for n sec (1Hz). Usage: ads_samples [dev_num] [time]",        NULL, ads1256_get_samples,    NULL, NULL, NULL}, NULL },
        { {"ads_change_mux",         "Change ads channel. Usage: ads_change_mux [dev_num] [0-3]",                        NULL, change_mux_channel,    NULL, NULL, NULL}, NULL },
        { {"ads_read_cal",           "Read calibration registers. Usage: ads_read_cal",                                  NULL, read_cal_registers,    NULL, NULL, NULL}, NULL },
        { {"ads_calibrate",          "Calibrate device on current channel. Usage: ads_calibrate [dev_num]",              NULL, calibrate_device,      NULL, NULL, NULL}, NULL },
        { {"ads_reset",              "Reset ads device. Usage: ads_reset [dev_num]",                                     NULL, ads1256_reset_cli,     NULL, NULL, NULL}, NULL },
        { {"ads_set_sps",            "Set data rate for ads device. Usage: ads_set_sps [dev_num] [sps_value]",           NULL, ads1256_set_sps_cmd,   NULL, NULL, NULL}, NULL },
        { {"ads_read_mux_samples",   "Read samples from the ADS1256 MUX. Usage: ads_read_mux_samples [dev_num] [nr_of_samples]", NULL, read_mux_samples,      NULL, NULL, NULL}, NULL },
        { {"ads_dev_info",           "Display device configuration information. Usage: ads_dev_info [dev_num]",          NULL, dev_info,              NULL, NULL, NULL}, NULL },
        { {"ads_print_data",         "Print data from ADS1256 device. Usage: ads_print_data [dev_num]",                  NULL, print_data,            NULL, NULL, NULL}, NULL },
        { {"help",                   "Display this help message",                                                        NULL, help_cmd,              NULL, NULL, NULL}, NULL },
        { {"ads_stop_task",          "Stop ADS1256 task. Usage: ads_stop_task [dev_num]",                                NULL, stop_task,             NULL, NULL, NULL}, NULL },
        { {"ads_start_task",         "Start ADS1256 task. Usage: ads_start_task [dev_num]",                              NULL, start_task,            NULL, NULL, NULL}, NULL },
        { {"ads_delete_task",        "Delete ADS1256 task. Usage: ads_delete_task [dev_num]",                            NULL, delete_task,           NULL, NULL, NULL}, NULL },
        { {"ads_read_id",            "Read ID from ADS1256 device. Usage: ads_read_id [dev_num]",                        NULL, read_id,               NULL, NULL, NULL}, NULL },
        { {"ads_tare",               "Zero sensors. Usage: ads_tare [dev_num] [channel 0-3]",                           NULL, tare_cmd,              NULL, NULL, NULL}, NULL },
        { {"ads_calibrate_channel",  "Calibrate one channel. Usage: ads_calibrate_channel [dev_num] <channel> <weight>", NULL, calibrate_cmd,         NULL, NULL, NULL}, NULL },
        { {"flash_read",             "Reads and displays saved data in flash memory.",                                   NULL, read_flash_cmd,        NULL, NULL, NULL}, NULL },
        { {"flash_display_config",   "Displays current runtime config (RAM).",                                           NULL, display_config_cmd,    NULL, NULL, NULL}, NULL },
        { {"flash_save_config",      "Saves current runtime config to flash memory.",                                    NULL, save_flash_cmd,        NULL, NULL, NULL}, NULL },
        { {"flash_edit_config",      "Edit a field in RAM config. Usage: flash_edit_config <key> <value>",              NULL, edit_flash_cmd,        NULL, NULL, NULL}, NULL },
        { {"flash_restore_config",   "Restore RAM config to defaults.",                                                  NULL, restore_defaults_cmd,  NULL, NULL, NULL}, NULL },
        { {"flash_erase",            "Erase NVS flash config. Usage: flash_erase Y",                                     NULL, erase_flash_cmd,       NULL, NULL, NULL}, NULL },
        { {"pw",                    "Print weights from device 1.",                                                      NULL, pw_cmd,                NULL, NULL, NULL}, NULL },
        { {"start_pw",              "Start periodic weight printing (every 0.5s).",                                      NULL, start_pw_cmd,          NULL, NULL, NULL}, NULL },
        { {"stop_pw",               "Stop periodic weight printing.",                                                    NULL, stop_pw_cmd,           NULL, NULL, NULL}, NULL },
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