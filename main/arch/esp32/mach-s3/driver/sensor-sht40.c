#include "sensors/sht40.h"

#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "sht40";

static uint8_t sht40_crc8(const uint8_t *data, uint8_t len)
{
	uint8_t crc = 0xFF;
	while (len--) {
		crc ^= *data++;
		for (uint8_t i = 0; i < 8; i++) {
			crc = (crc << 1) ^ ((crc & 0x80) ? 0x31 : 0);
		}
	}
	return crc;
}

static sht40_config_t sht40_default_config(void)
{
	sht40_config_t cfg = {
	        .timeout_ms = 200,
	        .measure_delay_ms = 20,
	        .retry_count = 1,
	        .scl_speed_hz = 400000,
	        .i2c_addr = SHT40_DEFAULT_I2C_ADDR,
	};
	return cfg;
}

static esp_err_t sht40_retry_transmit(sht40_handle_t *handle, const uint8_t *buffer, size_t len)
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

static esp_err_t sht40_retry_receive(sht40_handle_t *handle, uint8_t *buffer, size_t len)
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

esp_err_t sht40_attach(sht40_handle_t *handle, i2c_master_bus_handle_t bus, const sht40_config_t *config)
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
	handle->cfg = (config != NULL) ? *config : sht40_default_config();
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

esp_err_t sht40_read_measurement(sht40_handle_t *handle, sht40_measurement_t *out_measurement)
{
	if (handle == NULL || out_measurement == NULL || handle->dev == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	uint8_t cmd = SHT40_CMD_MEASURE_HIGH_PRECISION;
	esp_err_t ret = sht40_retry_transmit(handle, &cmd, 1);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "send measurement cmd failed: %s (%d)", esp_err_to_name(ret), ret);
		return ret;
	}

	vTaskDelay(pdMS_TO_TICKS(handle->cfg.measure_delay_ms));

	uint8_t raw[6] = {0};
	ret = sht40_retry_receive(handle, raw, sizeof(raw));
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "read measurement failed: %s (%d)", esp_err_to_name(ret), ret);
		return ret;
	}

	uint8_t temp_crc = sht40_crc8(&raw[0], 2);
	uint8_t humi_crc = sht40_crc8(&raw[3], 2);
	if (temp_crc != raw[2] || humi_crc != raw[5]) {
		ESP_LOGE(TAG, "crc mismatch: temp(0x%02X/0x%02X) humi(0x%02X/0x%02X)", temp_crc, raw[2], humi_crc, raw[5]);
		return ESP_ERR_INVALID_CRC;
	}

	uint16_t temp_raw = ((uint16_t)raw[0] << 8) | raw[1];
	uint16_t humi_raw = ((uint16_t)raw[3] << 8) | raw[4];
	out_measurement->temperature_c = (float)temp_raw * 175.0f / 65535.0f - 45.0f;
	out_measurement->humidity_rh = (float)humi_raw * 125.0f / 65535.0f - 6.0f;

	return ESP_OK;
}
