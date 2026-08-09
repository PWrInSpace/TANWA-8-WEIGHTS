#ifndef MOCK_MCU_SPI_CONFIG_H
#define MOCK_MCU_SPI_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    const uint8_t* tx_data;
    size_t tx_len;
    int cs_pin;
    bool rx_enabled;
} ads1256_spi_transmit_t;

bool _ads1256_spi_transmit(ads1256_spi_transmit_t* transmit, uint8_t* rx_data, size_t rx_len);

#endif
