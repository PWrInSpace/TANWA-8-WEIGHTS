#include "mock_spi.h"
#include "mcu_spi_config.h"
#include "freertos/task.h"
#include <string.h>

static bool s_should_fail = false;
static uint8_t s_rx_buffer[64];
static size_t s_rx_len = 0;

static uint8_t s_last_tx[MOCK_SPI_MAX_TX_LEN];
static size_t s_last_tx_len = 0;
static int s_last_cs_pin = -1;
static int s_call_count = 0;
static uint32_t s_total_delay_ms = 0;

void mock_spi_reset(void) {
    s_should_fail = false;
    s_rx_len = 0;
    s_last_tx_len = 0;
    s_last_cs_pin = -1;
    s_call_count = 0;
    s_total_delay_ms = 0;
    memset(s_rx_buffer, 0, sizeof(s_rx_buffer));
    memset(s_last_tx, 0, sizeof(s_last_tx));
}

void mock_spi_set_fail(bool fail) {
    s_should_fail = fail;
}

void mock_spi_set_rx_data(const uint8_t* rx, size_t len) {
    if (len > sizeof(s_rx_buffer)) len = sizeof(s_rx_buffer);
    memcpy(s_rx_buffer, rx, len);
    s_rx_len = len;
}

int mock_spi_get_call_count(void) {
    return s_call_count;
}

const uint8_t* mock_spi_get_last_tx(size_t* out_len) {
    if (out_len) *out_len = s_last_tx_len;
    return s_last_tx;
}

int mock_spi_get_last_cs_pin(void) {
    return s_last_cs_pin;
}

uint32_t mock_get_total_delay_ms(void) {
    return s_total_delay_ms;
}

void vTaskDelay(TickType_t ticks) {
    s_total_delay_ms += ticks;
}

bool _ads1256_spi_transmit(ads1256_spi_transmit_t* transmit, uint8_t* rx_data, size_t rx_len) {
    s_call_count++;
    if (s_should_fail) {
        return false;
    }

    if (transmit) {
        s_last_cs_pin = transmit->cs_pin;
        if (transmit->tx_data && transmit->tx_len > 0) {
            size_t copy_len = transmit->tx_len > MOCK_SPI_MAX_TX_LEN ? MOCK_SPI_MAX_TX_LEN : transmit->tx_len;
            memcpy(s_last_tx, transmit->tx_data, copy_len);
            s_last_tx_len = copy_len;
        }
    }

    if (rx_data && rx_len > 0 && s_rx_len > 0) {
        size_t copy_len = rx_len > s_rx_len ? s_rx_len : rx_len;
        memcpy(rx_data, s_rx_buffer, copy_len);
    }

    return true;
}
