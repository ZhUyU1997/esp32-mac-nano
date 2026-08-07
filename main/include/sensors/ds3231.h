#pragma once

#include <stdint.h>
#include <time.h>
#include "esp_err.h"
#include "driver/i2c_master.h"

#define DS3231_DEFAULT_I2C_ADDR 0x68
#define DS3231_TIME_REG_ADDR 0x00

typedef struct {
	uint16_t timeout_ms;
	uint16_t retry_count;
	uint32_t scl_speed_hz;
	uint16_t i2c_addr;
} ds3231_config_t;

typedef struct {
	i2c_master_bus_handle_t bus;
	i2c_master_dev_handle_t dev;
	ds3231_config_t cfg;
} ds3231_handle_t;

esp_err_t ds3231_attach(ds3231_handle_t *handle, i2c_master_bus_handle_t bus, const ds3231_config_t *config);
esp_err_t ds3231_set_time(ds3231_handle_t *handle, const struct tm *in_time);
esp_err_t ds3231_read_time(ds3231_handle_t *handle, struct tm *out_time);
