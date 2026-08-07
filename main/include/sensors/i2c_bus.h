#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"

typedef struct {
	i2c_port_t port;
	gpio_num_t sda_io_num;
	gpio_num_t scl_io_num;
	bool enable_internal_pullup;
	uint8_t glitch_ignore_cnt;
	i2c_clock_source_t clk_source;
} i2c_bus_create_config_t;

esp_err_t i2c_bus_create(const i2c_bus_create_config_t *config, i2c_master_bus_handle_t *out_bus);
