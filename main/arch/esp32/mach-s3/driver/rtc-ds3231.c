#include "sensors/ds3231.h"

#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ds3231";

static uint8_t bcd_to_dec(uint8_t bcd)
{
	return (uint8_t)(((bcd >> 4) * 10U) + (bcd & 0x0FU));
}

static uint8_t dec_to_bcd(uint8_t dec)
{
	return (uint8_t)(((dec / 10U) << 4) | (dec % 10U));
}

static ds3231_config_t ds3231_default_config(void)
{
	ds3231_config_t cfg = {
	        .timeout_ms = 200,
	        .retry_count = 1,
	        .scl_speed_hz = 400000,
	        .i2c_addr = DS3231_DEFAULT_I2C_ADDR,
	};
	return cfg;
}

static esp_err_t ds3231_retry_transmit(ds3231_handle_t *handle, const uint8_t *buffer, size_t len)
{
	esp_err_t ret = ESP_FAIL;
	for (uint16_t attempt = 0; attempt <= handle->cfg.retry_count; attempt++) {
		ret = i2c_master_transmit(handle->dev, buffer, len, pdMS_TO_TICKS(handle->cfg.timeout_ms));
		if (ret == ESP_OK) {
			return ESP_OK;
		}
		ESP_LOGW(TAG, "transmit failed (attempt=%u): %s (%d)", attempt + 1, esp_err_to_name(ret), ret);
		(void)i2c_master_bus_reset(handle->bus);
	}
	return ret;
}

static esp_err_t ds3231_retry_receive(ds3231_handle_t *handle, uint8_t *buffer, size_t len)
{
	esp_err_t ret = ESP_FAIL;
	for (uint16_t attempt = 0; attempt <= handle->cfg.retry_count; attempt++) {
		ret = i2c_master_receive(handle->dev, buffer, len, pdMS_TO_TICKS(handle->cfg.timeout_ms));
		if (ret == ESP_OK) {
			return ESP_OK;
		}
		ESP_LOGW(TAG, "receive failed (attempt=%u): %s (%d)", attempt + 1, esp_err_to_name(ret), ret);
		(void)i2c_master_bus_reset(handle->bus);
	}
	return ret;
}

esp_err_t ds3231_attach(ds3231_handle_t *handle, i2c_master_bus_handle_t bus, const ds3231_config_t *config)
{
	if (handle == NULL || bus == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	if (handle->dev != NULL) {
		if (handle->bus == bus) {
			return ESP_OK;
		}
		return ESP_ERR_INVALID_STATE;
	}

	memset(handle, 0, sizeof(*handle));
	handle->cfg = (config != NULL) ? *config : ds3231_default_config();
	handle->bus = bus;

	i2c_device_config_t dev_config = {
	        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
	        .device_address = handle->cfg.i2c_addr,
	        .scl_speed_hz = handle->cfg.scl_speed_hz,
	};

	esp_err_t ret = i2c_master_bus_add_device(bus, &dev_config, &handle->dev);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "add device failed: %s (%d)", esp_err_to_name(ret), ret);
		return ret;
	}

	return ESP_OK;
}

esp_err_t ds3231_set_time(ds3231_handle_t *handle, const struct tm *in_time)
{
	if (handle == NULL || in_time == NULL || handle->dev == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	if (in_time->tm_mon > 11 || in_time->tm_mday > 31 || in_time->tm_hour > 23 || in_time->tm_min > 59 || in_time->tm_sec > 59 || in_time->tm_wday > 6) {
		return ESP_ERR_INVALID_ARG;
	}

	uint8_t ds3231_year = (uint8_t)((in_time->tm_year - 100) % 100);
	uint8_t ds3231_month = (uint8_t)(in_time->tm_mon + 1);
	uint8_t ds3231_week = (in_time->tm_wday == 0) ? 7 : (uint8_t)in_time->tm_wday;

	uint8_t time_data[8] = {0};
	time_data[0] = DS3231_TIME_REG_ADDR;
	time_data[1] = dec_to_bcd((uint8_t)in_time->tm_sec);
	time_data[2] = dec_to_bcd((uint8_t)in_time->tm_min);
	time_data[3] = dec_to_bcd((uint8_t)in_time->tm_hour);
	time_data[4] = dec_to_bcd(ds3231_week);
	time_data[5] = dec_to_bcd((uint8_t)in_time->tm_mday);
	time_data[6] = dec_to_bcd(ds3231_month);
	time_data[7] = dec_to_bcd(ds3231_year);

	return ds3231_retry_transmit(handle, time_data, sizeof(time_data));
}

esp_err_t ds3231_read_time(ds3231_handle_t *handle, struct tm *out_time)
{
	if (handle == NULL || out_time == NULL || handle->dev == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	uint8_t reg_addr = DS3231_TIME_REG_ADDR;
	esp_err_t ret = ds3231_retry_transmit(handle, &reg_addr, 1);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "set register addr failed: %s (%d)", esp_err_to_name(ret), ret);
		return ret;
	}

	uint8_t time_data[7] = {0};
	ret = ds3231_retry_receive(handle, time_data, sizeof(time_data));
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "read time failed: %s (%d)", esp_err_to_name(ret), ret);
		return ret;
	}

	memset(out_time, 0, sizeof(*out_time));
	out_time->tm_sec = bcd_to_dec(time_data[0]);
	out_time->tm_min = bcd_to_dec(time_data[1]);
	out_time->tm_hour = bcd_to_dec(time_data[2]);
	out_time->tm_mday = bcd_to_dec(time_data[4]);
	out_time->tm_mon = bcd_to_dec(time_data[5]) - 1;
	out_time->tm_year = 100 + bcd_to_dec(time_data[6]);

	uint8_t ds3231_week = bcd_to_dec(time_data[3]);
	out_time->tm_wday = (ds3231_week == 7) ? 0 : ds3231_week;
	out_time->tm_isdst = 0;
	out_time->tm_yday = 0;

	return ESP_OK;
}
