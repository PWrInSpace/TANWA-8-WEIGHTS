#ifndef MOCK_SPI_H
#define MOCK_SPI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MOCK_SPI_MAX_TX_LEN 64

void mock_spi_reset(void);
void mock_spi_set_fail(bool fail);
void mock_spi_set_rx_data(const uint8_t* rx, size_t len);

int mock_spi_get_call_count(void);
const uint8_t* mock_spi_get_last_tx(size_t* out_len);
int mock_spi_get_last_cs_pin(void);
uint32_t mock_get_total_delay_ms(void);

#endif
