#include "unity.h"
#include "ads1256.h"
#include "mock_spi.h"
#include <string.h>

static ads1256_t* s_ads = NULL;
static ads1256_pin_config_t s_pins = {
    .cs_gpio = 15,
    .drdy_gpio = 18,
    .reset_gpio = 17,
    .pwdn_gpio = 16
};

void setUp(void) {
    mock_spi_reset();
    s_ads = ads1256_create(&s_pins);
}

void tearDown(void) {
    if (s_ads) {
        ads1256_destroy(s_ads);
        s_ads = NULL;
    }
}

/* =========================================================================
 * 1. Tests for ads1256_sps_hex_to_value
 * ========================================================================= */

void test_sps_hex_to_value_all_rates(void) {
    TEST_ASSERT_EQUAL_FLOAT(30000.0f, ads1256_sps_hex_to_value(SPS_30000));
    TEST_ASSERT_EQUAL_FLOAT(15000.0f, ads1256_sps_hex_to_value(SPS_15000));
    TEST_ASSERT_EQUAL_FLOAT(7500.0f,  ads1256_sps_hex_to_value(SPS_7500));
    TEST_ASSERT_EQUAL_FLOAT(3750.0f,  ads1256_sps_hex_to_value(SPS_3750));
    TEST_ASSERT_EQUAL_FLOAT(2000.0f,  ads1256_sps_hex_to_value(SPS_2000));
    TEST_ASSERT_EQUAL_FLOAT(1000.0f,  ads1256_sps_hex_to_value(SPS_1000));
    TEST_ASSERT_EQUAL_FLOAT(500.0f,   ads1256_sps_hex_to_value(SPS_500));
    TEST_ASSERT_EQUAL_FLOAT(100.0f,   ads1256_sps_hex_to_value(SPS_100));
    TEST_ASSERT_EQUAL_FLOAT(50.0f,    ads1256_sps_hex_to_value(SPS_50));
    TEST_ASSERT_EQUAL_FLOAT(25.0f,    ads1256_sps_hex_to_value(SPS_25));
    TEST_ASSERT_EQUAL_FLOAT(10.0f,    ads1256_sps_hex_to_value(SPS_10));
    TEST_ASSERT_EQUAL_FLOAT(5.0f,     ads1256_sps_hex_to_value(SPS_5));
    TEST_ASSERT_EQUAL_FLOAT(2.5f,     ads1256_sps_hex_to_value(SPS_2P5));
}

void test_sps_hex_to_value_invalid_returns_negative(void) {
    TEST_ASSERT_EQUAL_FLOAT(-1.0f, ads1256_sps_hex_to_value(0xFF));
    TEST_ASSERT_EQUAL_FLOAT(-1.0f, ads1256_sps_hex_to_value(0x00));
}

/* =========================================================================
 * 2. Tests for ads1256_create / destroy / pin_config
 * ========================================================================= */

void test_create_and_get_pin_config(void) {
    TEST_ASSERT_NOT_NULL(s_ads);
    const ads1256_pin_config_t* cfg = ads1256_get_pin_config(s_ads);
    TEST_ASSERT_NOT_NULL(cfg);
    TEST_ASSERT_EQUAL_INT(15, cfg->cs_gpio);
    TEST_ASSERT_EQUAL_INT(18, cfg->drdy_gpio);
    TEST_ASSERT_EQUAL_INT(17, cfg->reset_gpio);
    TEST_ASSERT_EQUAL_INT(16, cfg->pwdn_gpio);
}

/* =========================================================================
 * 3. Tests for ads1256_single_transmit
 * ========================================================================= */

void test_single_transmit_param_validation(void) {
    uint8_t dummy = 0xAA;
    // NULL tx_data or length 0
    TEST_ASSERT_FALSE(ads1256_single_transmit(s_ads, NULL, 1, NULL, 0));
    TEST_ASSERT_FALSE(ads1256_single_transmit(s_ads, &dummy, 0, NULL, 0));
    // NULL rx_data with rx_length > 0
    TEST_ASSERT_FALSE(ads1256_single_transmit(s_ads, &dummy, 1, NULL, 3));
}

void test_single_transmit_routes_cs_pin(void) {
    uint8_t tx = 0x12;
    TEST_ASSERT_TRUE(ads1256_single_transmit(s_ads, &tx, 1, NULL, 0));
    TEST_ASSERT_EQUAL_INT(15, mock_spi_get_last_cs_pin());
    TEST_ASSERT_EQUAL_INT(1, mock_spi_get_call_count());
}

void test_single_transmit_spi_failure(void) {
    uint8_t tx = 0x12;
    mock_spi_set_fail(true);
    TEST_ASSERT_FALSE(ads1256_single_transmit(s_ads, &tx, 1, NULL, 0));
}

/* =========================================================================
 * 4. Tests for ads1256_send_command (Universal Command Function)
 * ========================================================================= */

void test_send_command_reset_with_delay(void) {
    TEST_ASSERT_TRUE(ads1256_send_command(s_ads, RESET_COMMAND));

    size_t len;
    const uint8_t* tx = mock_spi_get_last_tx(&len);
    TEST_ASSERT_EQUAL_INT(1, len);
    TEST_ASSERT_EQUAL_HEX8(RESET_COMMAND, tx[0]);
    TEST_ASSERT_EQUAL_UINT32(100, mock_get_total_delay_ms());
}

void test_send_command_selfcal_with_delay(void) {
    TEST_ASSERT_TRUE(ads1256_send_command(s_ads, SELFCAL_COMMAND));

    size_t len;
    const uint8_t* tx = mock_spi_get_last_tx(&len);
    TEST_ASSERT_EQUAL_INT(1, len);
    TEST_ASSERT_EQUAL_HEX8(SELFCAL_COMMAND, tx[0]);
    TEST_ASSERT_EQUAL_UINT32(600, mock_get_total_delay_ms());
}

void test_send_command_wakeup_no_delay(void) {
    TEST_ASSERT_TRUE(ads1256_send_command(s_ads, WAKEUP_COMMAND));

    size_t len;
    const uint8_t* tx = mock_spi_get_last_tx(&len);
    TEST_ASSERT_EQUAL_INT(1, len);
    TEST_ASSERT_EQUAL_HEX8(WAKEUP_COMMAND, tx[0]);
    TEST_ASSERT_EQUAL_UINT32(0, mock_get_total_delay_ms());
}

void test_send_command_delay_override(void) {
    TEST_ASSERT_TRUE(ads1256_send_command_delay(s_ads, RESET_COMMAND, 200));

    size_t len;
    const uint8_t* tx = mock_spi_get_last_tx(&len);
    TEST_ASSERT_EQUAL_INT(1, len);
    TEST_ASSERT_EQUAL_HEX8(RESET_COMMAND, tx[0]);
    TEST_ASSERT_EQUAL_UINT32(200, mock_get_total_delay_ms());
}

void test_send_command_no_delay_on_spi_fail(void) {
    mock_spi_set_fail(true);
    TEST_ASSERT_FALSE(ads1256_send_command(s_ads, RESET_COMMAND));
    TEST_ASSERT_EQUAL_UINT32(0, mock_get_total_delay_ms());
}

/* =========================================================================
 * 5. Tests for ads1256_set_value & ads1256_read_register
 * ========================================================================= */

void test_set_value_formats_wreg_command(void) {
    TEST_ASSERT_TRUE(ads1256_set_value(s_ads, ADCON_REGISTER, ADCON_REGISTER_SETUP));

    size_t len;
    const uint8_t* tx = mock_spi_get_last_tx(&len);
    TEST_ASSERT_EQUAL_INT(3, len);
    TEST_ASSERT_EQUAL_HEX8(WREG_COMMAND | ADCON_REGISTER, tx[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00, tx[1]); // 1 byte write (n-1)
    TEST_ASSERT_EQUAL_HEX8(ADCON_REGISTER_SETUP, tx[2]);
}

void test_read_register_formats_rreg_command(void) {
    uint8_t rx_data[1] = {0xAB};
    mock_spi_set_rx_data(rx_data, 1);

    uint8_t val = 0;
    TEST_ASSERT_TRUE(ads1256_read_register(s_ads, STATUS_REGISTER, &val));

    size_t len;
    const uint8_t* tx = mock_spi_get_last_tx(&len);
    TEST_ASSERT_EQUAL_INT(2, len);
    TEST_ASSERT_EQUAL_HEX8(RREG_COMMAND | STATUS_REGISTER, tx[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00, tx[1]);
    TEST_ASSERT_EQUAL_HEX8(0xAB, val);
}

/* =========================================================================
 * 6. Tests for ads1256_get_raw_data & ads1256_read_id
 * ========================================================================= */

void test_get_raw_data_sends_rdata_and_receives_3_bytes(void) {
    uint8_t mock_raw[3] = {0x12, 0x34, 0x56};
    mock_spi_set_rx_data(mock_raw, 3);

    uint8_t raw[3] = {0};
    TEST_ASSERT_TRUE(ads1256_get_raw_data(s_ads, raw));

    size_t len;
    const uint8_t* tx = mock_spi_get_last_tx(&len);
    TEST_ASSERT_EQUAL_INT(1, len);
    TEST_ASSERT_EQUAL_HEX8(RDATA_COMMAND, tx[0]);

    TEST_ASSERT_EQUAL_HEX8(0x12, raw[0]);
    TEST_ASSERT_EQUAL_HEX8(0x34, raw[1]);
    TEST_ASSERT_EQUAL_HEX8(0x56, raw[2]);
}

void test_read_id_shifts_status_register(void) {
    uint8_t status_val = 0x3F; // Top 4 bits: 0x3 (ID = 3)
    mock_spi_set_rx_data(&status_val, 1);

    uint8_t id = 0;
    TEST_ASSERT_TRUE(ads1256_read_id(s_ads, &id));
    TEST_ASSERT_EQUAL_UINT8(3, id);
}

/* =========================================================================
 * 7. Tests for ads1256_read_cal_registers (Array Loop Refactor)
 * ========================================================================= */

void test_read_cal_registers_reads_6_registers_in_loop(void) {
    TEST_ASSERT_TRUE(ads1256_read_cal_registers(s_ads));
    TEST_ASSERT_EQUAL_INT(6, mock_spi_get_call_count());
}

void test_read_cal_registers_handles_error(void) {
    mock_spi_set_fail(true);
    TEST_ASSERT_FALSE(ads1256_read_cal_registers(s_ads));
}

/* =========================================================================
 * 8. Tests for ads1256_set_sps (Validation via ads1256_sps_hex_to_value)
 * ========================================================================= */

void test_set_sps_accepts_valid_rates(void) {
    TEST_ASSERT_TRUE(ads1256_set_sps(s_ads, DATA_RATE_REGISTER_1000SPS));

    size_t len;
    const uint8_t* tx = mock_spi_get_last_tx(&len);
    TEST_ASSERT_EQUAL_INT(3, len);
    TEST_ASSERT_EQUAL_HEX8(WREG_COMMAND | DATA_RATE_REGISTER, tx[0]);
    TEST_ASSERT_EQUAL_HEX8(DATA_RATE_REGISTER_1000SPS, tx[2]);
}

void test_set_sps_rejects_invalid_rate(void) {
    TEST_ASSERT_FALSE(ads1256_set_sps(s_ads, 0xEE)); // Invalid rate
    TEST_ASSERT_EQUAL_INT(0, mock_spi_get_call_count()); // No SPI transmit attempted
}

/* =========================================================================
 * 9. Tests for ads1256_set_calibration_registers
 * ========================================================================= */

void test_set_calibration_registers_null_check(void) {
    uint8_t dummy[3] = {0};
    TEST_ASSERT_FALSE(ads1256_set_calibration_registers(s_ads, NULL, dummy));
    TEST_ASSERT_FALSE(ads1256_set_calibration_registers(s_ads, dummy, NULL));
}

void test_set_calibration_registers_transmits_8_bytes(void) {
    uint8_t ofc[3] = {0x11, 0x22, 0x33};
    uint8_t fsc[3] = {0x44, 0x55, 0x66};

    TEST_ASSERT_TRUE(ads1256_set_calibration_registers(s_ads, ofc, fsc));

    size_t len;
    const uint8_t* tx = mock_spi_get_last_tx(&len);
    TEST_ASSERT_EQUAL_INT(8, len);
    TEST_ASSERT_EQUAL_HEX8(WREG_COMMAND | OFC0_REGISTER, tx[0]);
    TEST_ASSERT_EQUAL_HEX8(0x05, tx[1]); // 6 bytes write (0x05)
    TEST_ASSERT_EQUAL_HEX8(0x11, tx[2]);
    TEST_ASSERT_EQUAL_HEX8(0x22, tx[3]);
    TEST_ASSERT_EQUAL_HEX8(0x33, tx[4]);
    TEST_ASSERT_EQUAL_HEX8(0x44, tx[5]);
    TEST_ASSERT_EQUAL_HEX8(0x55, tx[6]);
    TEST_ASSERT_EQUAL_HEX8(0x66, tx[7]);
}

/* =========================================================================
 * Main Runner
 * ========================================================================= */

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_sps_hex_to_value_all_rates);
    RUN_TEST(test_sps_hex_to_value_invalid_returns_negative);

    RUN_TEST(test_create_and_get_pin_config);

    RUN_TEST(test_single_transmit_param_validation);
    RUN_TEST(test_single_transmit_routes_cs_pin);
    RUN_TEST(test_single_transmit_spi_failure);

    RUN_TEST(test_send_command_reset_with_delay);
    RUN_TEST(test_send_command_selfcal_with_delay);
    RUN_TEST(test_send_command_wakeup_no_delay);
    RUN_TEST(test_send_command_delay_override);
    RUN_TEST(test_send_command_no_delay_on_spi_fail);

    RUN_TEST(test_set_value_formats_wreg_command);
    RUN_TEST(test_read_register_formats_rreg_command);

    RUN_TEST(test_get_raw_data_sends_rdata_and_receives_3_bytes);
    RUN_TEST(test_read_id_shifts_status_register);

    RUN_TEST(test_read_cal_registers_reads_6_registers_in_loop);
    RUN_TEST(test_read_cal_registers_handles_error);

    RUN_TEST(test_set_sps_accepts_valid_rates);
    RUN_TEST(test_set_sps_rejects_invalid_rate);

    RUN_TEST(test_set_calibration_registers_null_check);
    RUN_TEST(test_set_calibration_registers_transmits_8_bytes);

    return UNITY_END();
}
