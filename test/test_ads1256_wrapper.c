#include "unity.h"
#include "ads1256_wrapper.h"
#include "mock_spi.h"
#include <string.h>

static ads1256_wrapper_t* s_wrapper = NULL;
static ads1256_pin_config_t s_pins = {
    .cs_gpio = 15,
    .drdy_gpio = 18,
    .reset_gpio = 17,
    .pwdn_gpio = 16
};

void setUp(void) {
    mock_spi_reset();
    s_wrapper = ads1256_init(&s_pins);
}

void tearDown(void) {
    if (s_wrapper) {
        ads1256_deinit(s_wrapper);
        s_wrapper = NULL;
    }
}

/* =========================================================================
 * 1. Test init and deinit
 * ========================================================================= */

void test_wrapper_init_and_get_dev(void) {
    TEST_ASSERT_NOT_NULL(s_wrapper);
    TEST_ASSERT_NOT_NULL(ads1256_wrapper_get_dev(s_wrapper));
}

/* =========================================================================
 * 2. Test calibration load and get
 * ========================================================================= */

void test_wrapper_load_and_get_calibration(void) {
    TEST_ASSERT_FALSE(ads1256_load_calibration(NULL, NULL));
    TEST_ASSERT_FALSE(ads1256_get_calibration(NULL, NULL));

    ads1256_calibration_t cal_in[4] = {
        {100, 2.5f},
        {-500, -1.0f},
        {0, 1.0f},
        {1234, 5.67f}
    };

    TEST_ASSERT_TRUE(ads1256_load_calibration(s_wrapper, cal_in));

    ads1256_calibration_t cal_out[4] = {0};
    TEST_ASSERT_TRUE(ads1256_get_calibration(s_wrapper, cal_out));

    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_EQUAL_INT32(cal_in[i].zero_offset, cal_out[i].zero_offset);
        TEST_ASSERT_EQUAL_FLOAT(cal_in[i].factor, cal_out[i].factor);
    }
}

/* =========================================================================
 * 3. Test raw_data_to_value conversion
 * ========================================================================= */

void test_wrapper_raw_data_to_value_positive(void) {
    ads1256_calibration_t cal[4] = {
        {100, 2.0f}, // Channel 0: offset=100, factor=2.0
        {0, 1.0f},
        {0, 1.0f},
        {0, 1.0f}
    };
    ads1256_load_calibration(s_wrapper, cal);

    // Raw = 512 (0x000200)
    uint8_t raw[3] = {0x00, 0x02, 0x00};
    float val = 0.0f;

    // diff = 512 - 100 = 412, weight = 412 / 2.0 = 206.0f
    TEST_ASSERT_TRUE(ads1256_raw_data_to_value(s_wrapper, raw, &val, 0));
    TEST_ASSERT_EQUAL_FLOAT(206.0f, val);
}

void test_wrapper_raw_data_to_value_negative_sign_extension(void) {
    ads1256_calibration_t cal[4] = {
        {0, 1.0f},
        {0, 1.0f},
        {0, 1.0f},
        {0, 1.0f}
    };
    ads1256_load_calibration(s_wrapper, cal);

    // 24-bit 0xFFFFFF = -1 in 2's complement
    uint8_t raw[3] = {0xFF, 0xFF, 0xFF};
    float val = 0.0f;

    TEST_ASSERT_TRUE(ads1256_raw_data_to_value(s_wrapper, raw, &val, 0));
    TEST_ASSERT_EQUAL_FLOAT(-1.0f, val);
}

void test_wrapper_raw_data_to_value_null_check(void) {
    float val = 0.0f;
    uint8_t raw[3] = {0};
    TEST_ASSERT_FALSE(ads1256_raw_data_to_value(s_wrapper, NULL, &val, 0));
    TEST_ASSERT_FALSE(ads1256_raw_data_to_value(s_wrapper, raw, NULL, 0));
}

/* =========================================================================
 * 4. Test set_zero_offset
 * ========================================================================= */

void test_wrapper_set_zero_offset(void) {
    TEST_ASSERT_FALSE(ads1256_set_zero_offset(s_wrapper, 500, 4)); // Invalid channel

    TEST_ASSERT_TRUE(ads1256_set_zero_offset(s_wrapper, -999, 2));

    ads1256_calibration_t cal[4];
    ads1256_get_calibration(s_wrapper, cal);
    TEST_ASSERT_EQUAL_INT32(-999, cal[2].zero_offset);
}

/* =========================================================================
 * 5. Test update_data_struct & median filtering
 * ========================================================================= */

void test_wrapper_update_data_struct_median_filter(void) {
    ads1256_data_t samples[5] = {
        {{10.0f, 100.0f, 0.0f, -50.0f}},
        {{50.0f, 500.0f, 0.0f,  10.0f}},
        {{30.0f, 300.0f, 0.0f,   0.0f}},
        {{20.0f, 200.0f, 0.0f,  20.0f}},
        {{40.0f, 400.0f, 0.0f, -10.0f}}
    };

    // Before update, copy fails because data is invalid
    ads1256_data_t out;
    TEST_ASSERT_FALSE(ads1256_get_data_struct_copy(s_wrapper, &out));

    ads1256_update_data_struct(s_wrapper, samples, 5);

    TEST_ASSERT_TRUE(ads1256_get_data_struct_copy(s_wrapper, &out));
    // Medians for sorted arrays:
    // Ch 0: [10, 20, 30, 40, 50] -> 30.0
    // Ch 1: [100, 200, 300, 400, 500] -> 300.0
    // Ch 2: [0, 0, 0, 0, 0] -> 0.0
    // Ch 3: [-50, -10, 0, 10, 20] -> 0.0
    TEST_ASSERT_EQUAL_FLOAT(30.0f, out.weight[0]);
    TEST_ASSERT_EQUAL_FLOAT(300.0f, out.weight[1]);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, out.weight[2]);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, out.weight[3]);
}

/* =========================================================================
 * 6. Test tare_all
 * ========================================================================= */

void test_wrapper_tare_all(void) {
    ads1256_calibration_t cal[4] = {
        {100, 2.0f}, // zero=100, factor=2.0
        {50, 1.0f},
        {0, 1.0f},
        {0, 1.0f}
    };
    ads1256_load_calibration(s_wrapper, cal);

    ads1256_data_t samples[1] = {
        {{10.0f, 20.0f, 0.0f, 0.0f}}
    };
    ads1256_update_data_struct(s_wrapper, samples, 1);

    TEST_ASSERT_TRUE(ads1256_tare_all(s_wrapper));

    ads1256_calibration_t cal_out[4];
    ads1256_get_calibration(s_wrapper, cal_out);

    // new_zero = (weight * factor) + old_zero
    // Ch 0: (10 * 2.0) + 100 = 120
    // Ch 1: (20 * 1.0) + 50 = 70
    TEST_ASSERT_EQUAL_INT32(120, cal_out[0].zero_offset);
    TEST_ASSERT_EQUAL_INT32(70, cal_out[1].zero_offset);
}

/* =========================================================================
 * 6b. Test tare_channel
 * ========================================================================= */

void test_wrapper_tare_channel(void) {
    ads1256_calibration_t cal[4] = {
        {100, 2.0f}, // zero=100, factor=2.0
        {50, 1.0f},  // zero=50, factor=1.0
        {0, 1.0f},
        {0, 1.0f}
    };
    ads1256_load_calibration(s_wrapper, cal);

    ads1256_data_t samples[1] = {
        {{10.0f, 20.0f, 0.0f, 0.0f}}
    };
    ads1256_update_data_struct(s_wrapper, samples, 1);

    // Invalid channel validation
    TEST_ASSERT_FALSE(ads1256_tare_channel(s_wrapper, 4));

    // Tare channel 1 only
    TEST_ASSERT_TRUE(ads1256_tare_channel(s_wrapper, 1));

    ads1256_calibration_t cal_out[4];
    ads1256_get_calibration(s_wrapper, cal_out);

    // Channel 0 remains unchanged: 100
    // Channel 1 updated: (20 * 1.0) + 50 = 70
    TEST_ASSERT_EQUAL_INT32(100, cal_out[0].zero_offset);
    TEST_ASSERT_EQUAL_INT32(70, cal_out[1].zero_offset);
}

/* =========================================================================
 * 7. Test calibrate_channel
 * ========================================================================= */

void test_wrapper_calibrate_channel(void) {
    ads1256_calibration_t cal[4] = {
        {0, 2.0f}, // zero=0, factor=2.0
        {0, 1.0f},
        {0, 1.0f},
        {0, 1.0f}
    };
    ads1256_load_calibration(s_wrapper, cal);

    ads1256_data_t samples[1] = {
        {{50.0f, 0.0f, 0.0f, 0.0f}} // Current measured weight = 50.0
    };
    ads1256_update_data_struct(s_wrapper, samples, 1);

    // Known calibration weight = 200.0f
    // raw_diff = (int32_t)(50.0 * 2.0) = 100
    // new_factor = 100 / 200.0f = 0.5f
    TEST_ASSERT_TRUE(ads1256_calibrate_channel(s_wrapper, 0, 200.0f));

    ads1256_calibration_t cal_out[4];
    ads1256_get_calibration(s_wrapper, cal_out);
    TEST_ASSERT_EQUAL_FLOAT(0.5f, cal_out[0].factor);

    // Validation checks
    TEST_ASSERT_FALSE(ads1256_calibrate_channel(s_wrapper, 4, 100.0f)); // Invalid channel
    TEST_ASSERT_FALSE(ads1256_calibrate_channel(s_wrapper, 0, -10.0f)); // Invalid weight
}

/* =========================================================================
 * 8. Test change_channel
 * ========================================================================= */

void test_wrapper_change_channel(void) {
    TEST_ASSERT_FALSE(ads1256_change_channel(s_wrapper, 5)); // Invalid channel

    TEST_ASSERT_TRUE(ads1256_change_channel(s_wrapper, 2));
}

/* =========================================================================
 * Main Runner
 * ========================================================================= */

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_wrapper_init_and_get_dev);
    RUN_TEST(test_wrapper_load_and_get_calibration);
    RUN_TEST(test_wrapper_raw_data_to_value_positive);
    RUN_TEST(test_wrapper_raw_data_to_value_negative_sign_extension);
    RUN_TEST(test_wrapper_raw_data_to_value_null_check);
    RUN_TEST(test_wrapper_set_zero_offset);
    RUN_TEST(test_wrapper_update_data_struct_median_filter);
    RUN_TEST(test_wrapper_tare_all);
    RUN_TEST(test_wrapper_tare_channel);
    RUN_TEST(test_wrapper_calibrate_channel);
    RUN_TEST(test_wrapper_change_channel);

    return UNITY_END();
}
