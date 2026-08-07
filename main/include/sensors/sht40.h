#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_master.h"

#define SHT40_DEFAULT_I2C_ADDR 0x44
#define SHT40_CMD_MEASURE_HIGH_PRECISION 0xFD

typedef struct {
	uint16_t timeout_ms;
	uint16_t measure_delay_ms;
	uint16_t retry_count;
	uint32_t scl_speed_hz;
	uint16_t i2c_addr;
} sht40_config_t;

typedef struct {
	i2c_master_bus_handle_t bus;
	i2c_master_dev_handle_t dev;
	sht40_config_t cfg;
} sht40_handle_t;

typedef struct {
	float temperature_c;
	float humidity_rh;
} sht40_measurement_t;

esp_err_t sht40_attach(sht40_handle_t *handle, i2c_master_bus_handle_t bus, const sht40_config_t *config);
esp_err_t sht40_read_measurement(sht40_handle_t *handle, sht40_measurement_t *out_measurement);
