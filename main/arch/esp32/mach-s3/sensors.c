#include "sensors/sensors.h"

#include <string.h>
#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "sensors/i2c_bus.h"
#include "sensors/sht40.h"
#include "sensors/ds3231.h"
#include "sensors/sensor_shm.h"

static const char *TAG = "sensors";

#define I2C_PORT I2C_NUM_0
#define I2C_SDA  GPIO_NUM_1
#define I2C_SCL  GPIO_NUM_2

/* ------------------------------------------------------------------ */
/* Sensor handles (initialised once)                                    */
/* ------------------------------------------------------------------ */
static i2c_master_bus_handle_t s_bus = NULL;
static sht40_handle_t          s_sht40 = {0};
static ds3231_handle_t         s_ds3231 = {0};

/* ------------------------------------------------------------------ */
/* Mutex-protected cache                                                */
/* ------------------------------------------------------------------ */
static SemaphoreHandle_t s_lock        = NULL;
static struct tm         s_cache_time  = {0};
static sht40_measurement_t s_cache_meas = {0};
static bool              s_time_valid  = false;
static bool              s_meas_valid  = false;

/* Cache generation — incremented on each DS3231 update, written to shm_region */
static uint32_t          s_cache_gen   = 0;

/* ------------------------------------------------------------------ */
/* Init (idempotent)                                                    */
/* ------------------------------------------------------------------ */
static esp_err_t sensors_init_once(void)
{
	if (s_bus != NULL) {
		return ESP_OK;
	}

	i2c_bus_create_config_t bus_cfg = {
	        .port = I2C_PORT,
	        .sda_io_num = I2C_SDA,
	        .scl_io_num = I2C_SCL,
	        .enable_internal_pullup = true,
	        .glitch_ignore_cnt = 7,
	        .clk_source = I2C_CLK_SRC_DEFAULT,
	};
	ESP_RETURN_ON_ERROR(i2c_bus_create(&bus_cfg, &s_bus), TAG, "create i2c bus failed");
	ESP_RETURN_ON_ERROR(sht40_attach(&s_sht40, s_bus, NULL), TAG, "attach sht40 failed");
	ESP_RETURN_ON_ERROR(ds3231_attach(&s_ds3231, s_bus, NULL), TAG, "attach ds3231 failed");

	/* Warm-up: discard SHT40 first reading (bus recovery for ESP32-S3) */
	{
		sht40_measurement_t m;
		sht40_read_measurement(&s_sht40, &m);
	}
	/* Warm-up: DS3231 first read so cache is ready on return.
	 * Safe to write cache without lock — no tasks running yet. */
	{
		struct tm t;
		if (ds3231_read_time(&s_ds3231, &t) == ESP_OK) {
			s_cache_time  = t;
			s_time_valid  = true;
		}
	}

	s_lock = xSemaphoreCreateMutex();

	ESP_LOGI(TAG, "init done");
	return ESP_OK;
}

/* ------------------------------------------------------------------ */
/* Non-blocking getters                                                 */
/* ------------------------------------------------------------------ */
esp_err_t sensors_get_time(struct tm *out_time)
{
	if (s_lock == NULL || out_time == NULL) {
		return ESP_ERR_INVALID_STATE;
	}
	xSemaphoreTake(s_lock, portMAX_DELAY);
	if (!s_time_valid) {
		xSemaphoreGive(s_lock);
		return ESP_ERR_NOT_FOUND;
	}
	*out_time = s_cache_time;
	xSemaphoreGive(s_lock);
	return ESP_OK;
}

esp_err_t sensors_get_measurement(sht40_measurement_t *out)
{
	if (s_lock == NULL || out == NULL) {
		return ESP_ERR_INVALID_STATE;
	}
	xSemaphoreTake(s_lock, portMAX_DELAY);
	if (!s_meas_valid) {
		xSemaphoreGive(s_lock);
		return ESP_ERR_NOT_FOUND;
	}
	*out = s_cache_meas;
	xSemaphoreGive(s_lock);
	return ESP_OK;
}

esp_err_t sensors_set_time(const struct tm *in_time)
{
	if (s_lock == NULL || in_time == NULL) {
		return ESP_ERR_INVALID_STATE;
	}

	esp_err_t err = ds3231_set_time(&s_ds3231, in_time);
	if (err == ESP_OK) {
		xSemaphoreTake(s_lock, portMAX_DELAY);
		s_time_valid = false;  /* invalidate cache */
		xSemaphoreGive(s_lock);
	}
	return err;
}

/* ------------------------------------------------------------------ */
/* Background cache task                                                */
/* ------------------------------------------------------------------ */
static void cache_task(void *arg)
{
	(void)arg;

	ESP_LOGI(TAG, "cache task started");

	while (1) {
		/* --- DS3231 (disabled after 5 consecutive failures) --- */
		static int  ds3231_fail_cnt;
		static bool ds3231_dead;
		if (!ds3231_dead) {
			struct tm t;
			if (ds3231_read_time(&s_ds3231, &t) == ESP_OK) {
				ds3231_fail_cnt = 0;
				xSemaphoreTake(s_lock, portMAX_DELAY);
				s_cache_time  = t;
				s_time_valid  = true;
				xSemaphoreGive(s_lock);
				s_cache_gen++;
			} else {
				ds3231_fail_cnt++;
				if (ds3231_fail_cnt >= 5) {
					ds3231_dead = true;
					ESP_LOGW(TAG, "DS3231 disabled after %u failures",
					         ds3231_fail_cnt);
				} else {
					ESP_LOGW(TAG, "DS3231 read failed (%u/%u)",
					         ds3231_fail_cnt, 5);
				}
			}
		}

		/* --- SHT40 (every 10 s, disabled after 5 consecutive failures) --- */
		{
			static uint32_t s_cycle = 0;
			static int      sht40_fail_cnt;
			static bool     sht40_dead;
			s_cycle++;
			if (!sht40_dead && s_cycle % 10 == 0) {
				sht40_measurement_t m;
				if (sht40_read_measurement(&s_sht40, &m) == ESP_OK) {
					sht40_fail_cnt = 0;
					xSemaphoreTake(s_lock, portMAX_DELAY);
					s_cache_meas  = m;
					s_meas_valid  = true;
					xSemaphoreGive(s_lock);
					ESP_LOGI(TAG, "[SHT40] T=%.2fC RH=%.2f%%", m.temperature_c, m.humidity_rh);
				} else {
					sht40_fail_cnt++;
					if (sht40_fail_cnt >= 5) {
						sht40_dead = true;
						ESP_LOGW(TAG, "SHT40 disabled after %u failures",
						         sht40_fail_cnt);
					} else {
						ESP_LOGW(TAG, "SHT40 read failed (%u/%u)",
						         sht40_fail_cnt, 5);
					}
				}
			}
		}

		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

uint32_t sensors_get_cache_gen(void)
{
	return s_cache_gen;
}

/* ------------------------------------------------------------------ */
/* Public entry point                                                   */
/* ------------------------------------------------------------------ */
esp_err_t sensors_init(void)
{
	esp_err_t ret = sensors_init_once();
	if (ret != ESP_OK) {
		return ret;
	}

	BaseType_t r = xTaskCreatePinnedToCore(
	        cache_task, "sensors-cache",
	        3072, NULL,
	        1, NULL, 0);
	if (r != pdPASS) {
		return ESP_ERR_NO_MEM;
	}

	return ESP_OK;
}
