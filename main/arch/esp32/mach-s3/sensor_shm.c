#include "sensors/sensor_shm.h"

#include <string.h>
#include "esp_log.h"
#include "sensors/sensors.h"

static const char *TAG = "sensor_shm";

static void shm_store_time(uint8_t *shm, const struct tm *t)
{
	shm[SENSOR_SHM_TIME_OFFSET + 0] = ((t->tm_sec  / 10) << 4) | (t->tm_sec  % 10);
	shm[SENSOR_SHM_TIME_OFFSET + 1] = ((t->tm_min  / 10) << 4) | (t->tm_min  % 10);
	shm[SENSOR_SHM_TIME_OFFSET + 2] = ((t->tm_hour / 10) << 4) | (t->tm_hour % 10);
	shm[SENSOR_SHM_TIME_OFFSET + 3] = ((t->tm_mday / 10) << 4) | (t->tm_mday % 10);
	shm[SENSOR_SHM_TIME_OFFSET + 4] = (((t->tm_mon + 1) / 10) << 4) | ((t->tm_mon + 1) % 10);
	shm[SENSOR_SHM_TIME_OFFSET + 5] = (((t->tm_year % 100) / 10) << 4) | ((t->tm_year % 100) % 10);
	shm[SENSOR_SHM_TIME_OFFSET + 6] = (uint8_t)t->tm_wday;
}

static void sensor_shm_handler(macplus_t *sim, unsigned int value)
{
	uint8_t *shm = sim->shm_region;

	/* Hook 24 (RTC read) is a shortcut — force command = GET_TIME */
	if (value == SENSOR_HOOK_RTC) {
		shm[SENSOR_SHM_CMD_OFFSET] = SENSOR_CMD_GET_TIME;
	}

	uint8_t cmd = shm[SENSOR_SHM_CMD_OFFSET];

	switch (cmd) {
	case SENSOR_CMD_GET_MEASUREMENT: {
		sht40_measurement_t m;
		if (sensors_get_measurement(&m) == ESP_OK) {
			uint16_t st = (int16_t)(m.temperature_c * 10.0f);
			uint16_t sh = (int16_t)(m.humidity_rh * 10.0f);
			put_unaligned_be16(st, &shm[SENSOR_SHM_TEMP_OFFSET]);
			put_unaligned_be16(sh, &shm[SENSOR_SHM_HUMI_OFFSET]);
		} else {
			ESP_LOGW(TAG, "measurement not ready, using placeholder");
			put_unaligned_be16(880, &shm[SENSOR_SHM_TEMP_OFFSET]);
			put_unaligned_be16(0,   &shm[SENSOR_SHM_HUMI_OFFSET]);
		}
		break;
	}

	case SENSOR_CMD_GET_TIME: {
		struct tm t;
		if (sensors_get_time(&t) == ESP_OK) {
			shm_store_time(shm, &t);
		} else {
			ESP_LOGW(TAG, "time not ready, using placeholder");
			memset(&t, 0, sizeof(t));
			t.tm_year = 125;
			t.tm_mon  = 0;
			t.tm_mday = 1;
			t.tm_wday = 0;
			shm_store_time(shm, &t);
		}
		break;
	}

	case SENSOR_CMD_SET_TIME: {
		ESP_LOGI(TAG, "CMD_SET_TIME");
		uint8_t bcd_sec  = shm[SENSOR_SHM_TIME_OFFSET + 0];
		uint8_t bcd_min  = shm[SENSOR_SHM_TIME_OFFSET + 1];
		uint8_t bcd_hr   = shm[SENSOR_SHM_TIME_OFFSET + 2];
		uint8_t bcd_day  = shm[SENSOR_SHM_TIME_OFFSET + 3];
		uint8_t bcd_mon  = shm[SENSOR_SHM_TIME_OFFSET + 4];
		uint8_t bcd_year = shm[SENSOR_SHM_TIME_OFFSET + 5];

		struct tm t = {0};
		t.tm_sec  = ((bcd_sec  >> 4) * 10) + (bcd_sec  & 0x0F);
		t.tm_min  = ((bcd_min  >> 4) * 10) + (bcd_min  & 0x0F);
		t.tm_hour = ((bcd_hr   >> 4) * 10) + (bcd_hr   & 0x0F);
		t.tm_mday = ((bcd_day  >> 4) * 10) + (bcd_day  & 0x0F);
		t.tm_mon  = ((bcd_mon  >> 4) * 10) + (bcd_mon  & 0x0F) - 1;
		t.tm_year = ((bcd_year >> 4) * 10) + (bcd_year & 0x0F) + 100;

		esp_err_t err = sensors_set_time(&t);
		if (err == ESP_OK) {
			ESP_LOGI(TAG, "time set OK");
		} else {
			ESP_LOGW(TAG, "time set failed: %s", esp_err_to_name(err));
		}
		break;
	}

	case SENSOR_CMD_GET_GEN: {
		put_unaligned_be32(sensors_get_cache_gen(), &shm[SENSOR_SHM_GEN_OFFSET]);
		break;
	}

	default:
		if (cmd != 0) {
			ESP_LOGW(TAG, "unknown cmd %u", cmd);
		}
		{
			put_unaligned_be16(880, &shm[SENSOR_SHM_TEMP_OFFSET]);
			put_unaligned_be16(0,   &shm[SENSOR_SHM_HUMI_OFFSET]);
		}
		break;
	}

	shm[SENSOR_SHM_STATUS_OFFSET] = SENSOR_STATUS_DONE;
}

void sensor_shm_init(macplus_t *sim)
{
	if (sim == NULL) return;
	mac_register_hook(sim, SENSOR_HOOK_READ, sensor_shm_handler);
	mac_register_hook(sim, SENSOR_HOOK_RTC, sensor_shm_handler);
	ESP_LOGI(TAG, "hooks registered (23=%d, 24=%d)", SENSOR_HOOK_READ, SENSOR_HOOK_RTC);
}
