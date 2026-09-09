#ifndef MOCK_GPIO_H
#define MOCK_GPIO_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

typedef enum {
    GPIO_NUM_0 = 0,
    GPIO_NUM_15 = 15,
    GPIO_NUM_16 = 16,
    GPIO_NUM_17 = 17,
    GPIO_NUM_18 = 18,
} gpio_num_t;

typedef enum {
    GPIO_MODE_DISABLE = 0,
    GPIO_MODE_INPUT = 1,
    GPIO_MODE_OUTPUT = 2,
} gpio_mode_t;

typedef enum {
    GPIO_PULLUP_DISABLE = 0,
    GPIO_PULLUP_ENABLE = 1,
} gpio_pullup_t;

typedef enum {
    GPIO_PULLDOWN_DISABLE = 0,
    GPIO_PULLDOWN_ENABLE = 1,
} gpio_pulldown_t;

typedef enum {
    GPIO_INTR_DISABLE = 0,
    GPIO_INTR_NEGEDGE = 2,
} gpio_int_type_t;

typedef struct {
    uint64_t pin_bit_mask;
    gpio_mode_t mode;
    gpio_pullup_t pull_up_en;
    gpio_pulldown_t pull_down_en;
    gpio_int_type_t intr_type;
} gpio_config_t;

typedef void (*gpio_isr_t)(void* arg);

static inline esp_err_t gpio_config(const gpio_config_t *pGPIOConfig) { (void)pGPIOConfig; return ESP_OK; }
static inline esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level) { (void)gpio_num; (void)level; return ESP_OK; }
static inline esp_err_t gpio_install_isr_service(int intr_alloc_flags) { (void)intr_alloc_flags; return ESP_OK; }
static inline esp_err_t gpio_isr_handler_add(gpio_num_t gpio_num, gpio_isr_t isr_handler, void* args) { (void)gpio_num; (void)isr_handler; (void)args; return ESP_OK; }
static inline esp_err_t gpio_isr_handler_remove(gpio_num_t gpio_num) { (void)gpio_num; return ESP_OK; }
static inline esp_err_t gpio_intr_enable(gpio_num_t gpio_num) { (void)gpio_num; return ESP_OK; }
static inline esp_err_t gpio_intr_disable(gpio_num_t gpio_num) { (void)gpio_num; return ESP_OK; }

#endif
