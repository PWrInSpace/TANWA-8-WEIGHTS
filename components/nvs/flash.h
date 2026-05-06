#ifndef PWRINSPACE_FLASH_H
#define PWRINSPACE_FLASH_H

#include "nvs.h"
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs_data_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define NVS_NAMESPACE "app_storage"
#define BLOB_KEY "calibr_config"

// Make sure that NVS_NAMESPACE and BLOB_KEY don't exceed NVS_KEY_NAME_MAX_SIZE
static_assert(sizeof(NVS_NAMESPACE) <= NVS_KEY_NAME_MAX_SIZE, "NVS_NAMESPACE exceeds NVS_KEY_NAME_MAX_SIZE");
static_assert(sizeof(BLOB_KEY) <= NVS_KEY_NAME_MAX_SIZE, "BLOB_KEY exceeds NVS_KEY_NAME_MAX_SIZE");

// Generate struct automatically using macro
typedef struct {
    #define DATA(name, type, default_val) type name;
    #define DATA_ARRAY(name, type, size, default_val) type name[size];
    #define SECTION_BEGIN(name) struct {
    #define SECTION_END(name) } name;

    CONFIG_FIELDS

    #undef DATA
    #undef DATA_ARRAY
    #undef SECTION_BEGIN
    #undef SECTION_END
} data_config_t;

/**
 * @brief Initialize NVS and configuration storage.
 * @return `ESP_OK` if successful, error code if failed
 */
esp_err_t flash_init(void);

/**
 * @brief Save runtime config to flash (NVS commit).
 * @return `ESP_OK` if successful, error code if failed
 */
esp_err_t flash_commit(void);

/**
 * @brief Erase entire NVS storage.
 * @return `ESP_OK` if successful, error code if failed
 */
esp_err_t flash_erase_all(void);

/**
 * @brief Erase NVS partition with saved config.
 * @return `ESP_OK` if successful, error code if failed
 */
esp_err_t flash_erase_config(void);

/**
 * @brief Restore default configuration values to runtime config.
 * @return `ESP_OK` if successful, error code if failed
 */
esp_err_t flash_restore_defaults(void);

/**
 * @brief Read configuration from flash.
 * @param out_config [out] pointer to configuration structure
 * @return `ESP_OK` if successful, error code if failed (structure zeroed on failure)
 */
esp_err_t flash_read(data_config_t *out_config);

/**
 * @brief Get current runtime config.
 * @param out_config [out] pointer to configuration structure
 * @return `ESP_OK` if successful, error code if failed
 */
esp_err_t flash_get_runtime_config(data_config_t *out_config);

/**
 * @brief Overwrite the runtime config.
 * @param config [in] configuration structure
 * @return `ESP_OK` if successful, error code if failed
 */
esp_err_t flash_edit_config(data_config_t config);

// |--- Functions for CLI ---|

/**
 * @brief Edit a single configuration field in runtime config.
 * @param field [in] field name
 * @param value [in] new value
 * @return `ESP_OK` if successful, error code if failed
 */
esp_err_t flash_edit_field(const char *field, const char *value);

/**
 * @brief Get the list of configuration field names.
 * @param count [out] pointer to value holding number of fields
 * @return pointer to an array of field name strings
 */
const char **flash_get_field_names(size_t *count);

/**
 * @brief Prints the provided configuration structure to stdout.
 * @param config The configuration structure to print (passed by value)
 */
void flash_print_config(const data_config_t config);

#endif // PWRINSPACE_FLASH_H