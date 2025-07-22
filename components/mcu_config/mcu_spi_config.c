///===-----------------------------------------------------------------------------------------===//
///
/// Copyright (c) PWr in Space. All rights reserved.
/// Created: 15.05.2024 by Michał Kos
///
///===-----------------------------------------------------------------------------------------===//

#include "mcu_spi_config.h"

#include "mcu_gpio_config.h"

#include "esp_log.h"
#include "esp_rom_sys.h"
#define TAG "MCU_SPI"

static mcu_spi_config_t spi_config = MCU_SPI_DEFAULT_CONFIG();
SemaphoreHandle_t mutex_spi;

esp_err_t mcu_spi_init(void) {
    esp_err_t ret = ESP_OK;
    if (spi_config.spi_init_flag) {
      return ESP_OK;
    }

    ret = spi_bus_initialize(spi_config.host_id, &spi_config.bus_config, SDSPI_DEFAULT_DMA);
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(ret);
    mutex_spi = xSemaphoreCreateMutex();
    spi_config.spi_init_flag = true;
    return ret;
}

esp_err_t mcu_spi_deinit(void) {
    esp_err_t ret = ESP_OK;
    if (!spi_config.spi_init_flag) {
      return ESP_OK;
    }
    ret = spi_bus_remove_device(spi_config.spi_ads1256_handle);
    ESP_ERROR_CHECK(ret);
    ret = spi_bus_free(spi_config.host_id);
    ESP_ERROR_CHECK(ret);
    spi_config.spi_init_flag = false;
    return ret;
}


bool _ads1256_add_device(void) {
  esp_err_t ret;

  spi_device_interface_config_t dev = {.clock_speed_hz = 2500000,
                                       .mode = 1,
                                       .spics_io_num = -1,
                                       .queue_size = 1,
                                       .flags = 0,
                                       .pre_cb = NULL};
  ret = spi_bus_add_device(spi_config.host_id, &dev, &spi_config.spi_ads1256_handle);
  ESP_ERROR_CHECK(ret);

  return ret == ESP_OK;
}             


bool _ads1256_spi_transmit(ads1256_spi_transmit_t* ads_transmit, uint8_t* rx_data, size_t rx_len) {
  spi_transaction_t t = {.flags = 0,
                          .length = 8 * sizeof(uint8_t) * ads_transmit->tx_len,
                          .tx_buffer = ads_transmit->tx_data,
                          .rxlength = 8 * sizeof(uint8_t) * rx_len,
                          .rx_buffer = rx_data};
  xSemaphoreTake(mutex_spi, portMAX_DELAY);
  gpio_set_level(ads_transmit->cs_pin, 0);
  esp_err_t res = spi_device_transmit(spi_config.spi_ads1256_handle, &t);
  gpio_set_level(ads_transmit->cs_pin, 1);
  xSemaphoreGive(mutex_spi);
  if (res != ESP_OK) {
      ESP_LOGE(TAG, "SPI transmission failed: %s", esp_err_to_name(res));
      return false;
  }

  return true;
}

bool _ads1256_spi_transmit_queued(const uint8_t* tx_data, size_t tx_len, uint8_t* rx_data, size_t rx_len)
{
    spi_transaction_t trans = {
        .flags = 0,
        .length = 8 * tx_len,
        .tx_buffer = tx_data,
        .rxlength = 8 * rx_len,
        .rx_buffer = rx_data
    };

    xSemaphoreTake(mutex_spi, portMAX_DELAY);
    gpio_set_level(15, 0);
    esp_err_t ret = spi_device_queue_trans(spi_config.spi_ads1256_handle, &trans, portMAX_DELAY);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_device_queue_trans failed: %s", esp_err_to_name(ret));
        xSemaphoreGive(mutex_spi);
        return false;
    }

    spi_transaction_t *ret_trans;
    ret = spi_device_get_trans_result(spi_config.spi_ads1256_handle, &ret_trans, portMAX_DELAY);
    gpio_set_level(15, 1);
    xSemaphoreGive(mutex_spi);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_device_get_trans_result failed: %s", esp_err_to_name(ret));
        return false;
    }

    return true;
}