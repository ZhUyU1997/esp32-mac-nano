/*
 * task_stats.c — per-task CPU usage and stack stats (debug aid)
 *
 * Requires these sdkconfig options:
 *   CONFIG_FREERTOS_USE_TRACE_FACILITY=y
 *   CONFIG_FREERTOS_USE_STATS_FORMATTING=y
 *   CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS=y
 *
 * When any of these is missing, task_stats_print() is a no-op.
 */

#include "task_stats.h"
#include "sdkconfig.h"

#if CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS && CONFIG_FREERTOS_USE_STATS_FORMATTING

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "task_stats";

void task_stats_print(void)
{
	char buf[1024];
	vTaskGetRunTimeStats(buf);
	ESP_LOGI(TAG, "=== task stats ===\n%s", buf);
}

#else

void task_stats_print(void) {}

#endif
