#pragma once

#include <time.h>

#include "esp_err.h"

#include "sensors/sht40.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialise I2C sensors and start background cache task.
 *
 * Safe to call multiple times.  Returns immediately after starting the
 * cache task — actual sensor reads happen on core 0 every 5 s.
 */
esp_err_t sensors_init(void);

/**
 * @brief Non-blocking read of the last cached RTC time.
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if cache not ready yet.
 */
esp_err_t sensors_get_time(struct tm *out_time);

/**
 * @brief Non-blocking read of the last cached SHT40 measurement.
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if cache not ready yet.
 */
esp_err_t sensors_get_measurement(sht40_measurement_t *out);

/**
 * @brief Write time to DS3231 RTC directly.
 *
 * Invalidates the cache — the background task will re-read on next cycle.
 */
esp_err_t sensors_set_time(const struct tm *in_time);

/**
 * @brief Get the cache generation counter (increments on each DS3231 update).
 */
uint32_t sensors_get_cache_gen(void);

#ifdef __cplusplus
}
#endif
