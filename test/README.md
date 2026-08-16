# ADS1256 Unit Tests

Host-based unit tests for the `ads1256` hardware driver and `ads1256_wrapper` module using the **Unity** testing framework and mock SPI/FreeRTOS/GPIO layers.

## Prerequisites

- `cmake` (>= 3.16)
- `gcc` or `clang`

No ESP-IDF environment or physical ESP32 board is required to run these tests.

## How to Build and Run Tests

Run the following commands from the `test` directory:

### Option 1: Direct Execution

```bash
cd test
cmake -B build
cmake --build build
./build/test_ads1256
./build/test_ads1256_wrapper
```

### Option 2: Using CTest

```bash
cd test
cmake -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

---

## Test Coverage Overview

Below is the complete list of unit tests implemented in the test suite (31 tests in total).

### 1. Low-Level Driver Tests (`test_ads1256` — 21 Tests)

| № | Test Name | Tested Function | Description |
|---|---|---|---|
| 1 | `test_sps_hex_to_value_all_rates` | `ads1256_sps_hex_to_value` | Tests conversion for all 13 supported sampling rates (2.5 to 30000 SPS) |
| 2 | `test_sps_hex_to_value_invalid_returns_negative` | `ads1256_sps_hex_to_value` | Returns `-1.0f` for invalid rate codes (`0x00`, `0xFF`) |
| 3 | `test_create_and_get_pin_config` | `ads1256_create`, `ads1256_get_pin_config` | Correct initialization of driver structure and GPIO pin mapping |
| 4 | `test_single_transmit_param_validation` | `ads1256_single_transmit` | Parameter validation for NULL pointers and zero lengths |
| 5 | `test_single_transmit_routes_cs_pin` | `ads1256_single_transmit` | Verifies routing of Chip Select (CS) pin during SPI transmission |
| 6 | `test_single_transmit_spi_failure` | `ads1256_single_transmit` | Handles SPI bus transmission failures |
| 7 | `test_send_command_reset_with_delay` | `ads1256_send_command` | Sends `RESET` command with 100 ms execution delay |
| 8 | `test_send_command_selfcal_with_delay` | `ads1256_send_command` | Sends `SELFCAL` command with 600 ms execution delay |
| 9 | `test_send_command_wakeup_no_delay` | `ads1256_send_command` | Sends `WAKEUP` command without delay |
| 10 | `test_send_command_delay_override` | `ads1256_send_command_delay` | Explicit custom delay setting (200 ms) |
| 11 | `test_send_command_no_delay_on_spi_fail` | `ads1256_send_command` | Cancels execution delay if SPI transmit fails |
| 12 | `test_set_value_formats_wreg_command` | `ads1256_set_value` | Formats `WREG` command byte sequence when writing to registers |
| 13 | `test_read_register_formats_rreg_command` | `ads1256_read_register` | Formats `RREG` command and reads register value |
| 14 | `test_get_raw_data_sends_rdata_and_receives_3_bytes` | `ads1256_get_raw_data` | Sends `RDATA` command and receives 3-byte raw conversion frame |
| 15 | `test_read_id_shifts_status_register` | `ads1256_read_id` | Extracts chip ID bits from STATUS register |
| 16 | `test_read_cal_registers_reads_6_registers_in_loop` | `ads1256_read_cal_registers` | Sequentially reads 6 calibration registers (`OFC` / `FSC`) in loop |
| 17 | `test_read_cal_registers_handles_error` | `ads1256_read_cal_registers` | Handles SPI errors while reading calibration registers |
| 18 | `test_set_sps_accepts_valid_rates` | `ads1256_set_sps` | Writes valid sampling rate register code |
| 19 | `test_set_sps_rejects_invalid_rate` | `ads1256_set_sps` | Rejects invalid rate without attempting SPI transmission |
| 20 | `test_set_calibration_registers_null_check` | `ads1256_set_calibration_registers` | NULL check for `OFC` and `FSC` arrays |
| 21 | `test_set_calibration_registers_transmits_8_bytes` | `ads1256_set_calibration_registers` | Transmits 8-byte payload (`WREG` header + 6 calibration bytes) |

### 2. High-Level Wrapper Tests (`test_ads1256_wrapper` — 10 Tests)

| № | Test Name | Tested Function | Description |
|---|---|---|---|
| 1 | `test_wrapper_init_and_get_dev` | `ads1256_init`, `ads1256_wrapper_get_dev` | Wrapper initialization and access to underlying `ads1256_t` instance |
| 2 | `test_wrapper_load_and_get_calibration` | `ads1256_load_calibration`, `ads1256_get_calibration` | Loads and retrieves calibration arrays (zero offset & factor) for 4 channels |
| 3 | `test_wrapper_raw_data_to_value_positive` | `ads1256_raw_data_to_value` | Calculates weight from 24-bit positive raw sample: `(raw - zero) / factor` |
| 4 | `test_wrapper_raw_data_to_value_negative_sign_extension` | `ads1256_raw_data_to_value` | Restores sign extension for negative 24-bit samples (`0xFFFFFF` $\to$ `-1.0f`) |
| 5 | `test_wrapper_raw_data_to_value_null_check` | `ads1256_raw_data_to_value` | Validates NULL pointers |
| 6 | `test_wrapper_set_zero_offset` | `ads1256_set_zero_offset` | Updates zero offset for specific channel and checks channel boundary |
| 7 | `test_wrapper_update_data_struct_median_filter` | `ads1256_update_data_struct`, `ads1256_get_data_struct_copy` | Median filter algorithm over N samples and data validity flag handling |
| 8 | `test_wrapper_tare_all` | `ads1256_tare_all` | Recalculates zero offsets for all 4 channels based on current measurements |
| 9 | `test_wrapper_calibrate_channel` | `ads1256_calibrate_channel` | Calibrates channel scaling factor using a known reference weight |
| 10 | `test_wrapper_change_channel` | `ads1256_change_channel` | Changes active channel and updates MUX register |

---

## Expected Test Results

### 1. Success Case (All 31 Tests Passing)

```text
-----------------------
21 Tests 0 Failures 0 Ignored 
OK (test_ads1256)

-----------------------
10 Tests 0 Failures 0 Ignored 
OK (test_ads1256_wrapper)
```

*(Note: `[ERROR]` and `[WARN]` lines during execution are expected; they verify that error-handling paths log properly when tested with invalid inputs.)*
