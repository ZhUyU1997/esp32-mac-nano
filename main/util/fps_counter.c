#include <stdio.h>
#include "esp_timer.h"

void fps_counter_tick_print(void)
{
	static int count = 0;
	static uint64_t start;
	count++;

	if (count < 120) {
		return;
	}

	int frames = count;
	count = 0;

	uint64_t end = esp_timer_get_time();
	if (start != 0) {
		printf("FPS: %f\n", frames / ((end - start) / 1000000.0));
	}
	start = end;
}
