# ADS1256 Unit Tests

Host-based unit tests for the `ads1256` hardware driver using the **Unity** testing framework and mock SPI/FreeRTOS layers.

## Prerequisites

- `cmake` (>= 3.16)
- `gcc` or `clang`

No ESP-IDF environment or physical ESP32 board is required to run these tests.

## How to Build and Run Tests

Run the following commands from the project root or the `test` directory:

### Option 1: Direct Execution

```bash
cd test
cmake -B build
cmake --build build
./build/test_ads1256
```

### Option 2: Using CTest

```bash
cd test
cmake -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

---

## Expected Test Results

### 1. Success Case (All Tests Passing)

When all tests pass, the output ends with `OK` and `0 Failures`:

```text
...
test/test_ads1256.c:276:test_set_sps_accepts_valid_rates:PASS
test/test_ads1256.c:277:test_set_sps_rejects_invalid_rate:PASS
test/test_ads1256.c:279:test_set_calibration_registers_null_check:PASS
test/test_ads1256.c:280:test_set_calibration_registers_transmits_8_bytes:PASS

-----------------------
19 Tests 0 Failures 0 Ignored 
OK
```

*(Note: `[ERROR]` lines during execution are expected; they verify that error-handling paths and loggers in the driver function properly when tested with invalid inputs.)*

### 2. Failure Case (Test Assertion Failed)

If a test fails (e.g. an unexpected return value or broken logic), Unity prints the file, line number, expected vs actual values, and ends with `FAIL`:

```text
test/test_ads1256.c:45:test_sps_hex_to_value_all_rates:FAIL: Expected 1000.0 Was 500.0

-----------------------
19 Tests 1 Failures 0 Ignored 
FAIL
```

When running via `ctest`, CTest will output:

```text
100% tests passed, 0 tests failed out of 1
# Or on failure:
0% tests passed, 1 test failed out of 1
```
