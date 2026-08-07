#include "flash_mode.h"

#include "esp_log.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "settings_persist.h"
#include "machine_backend.h"

/* Physical key used for the startup forced entry — one of the existing
 * keys (7/15/16 in mach-s3.json). Adjust to the hardware layout. */
#define FLASH_MODE_FORCE_GPIO GPIO_NUM_15

static const char *TAG = "flash_mode";
static bool s_flash_mode_active = false;

void mach_s3_flash_mode_enter(void)
{
	mach_s3_settings_persist_set_flash_mode(true);
	machine_backend_reboot();
}

bool mach_s3_flash_mode_active(void)
{
	return s_flash_mode_active;
}

/* Held-key check: 3 consecutive low samples 10ms apart (keys are
 * active-low with pull-up). Pure GPIO, no driver/UI dependency. */
static bool flash_mode_force_key_held(void)
{
	for (int i = 0; i < 3; i++) {
		if (gpio_get_level(FLASH_MODE_FORCE_GPIO) != 0) {
			return false;
		}
		vTaskDelay(pdMS_TO_TICKS(10));
	}
	return true;
}

bool mach_s3_flash_mode_check(void)
{
	/* 1) Forced entry: physical key held at boot — pure GPIO, no NVS/UI/
	 * emulator/USB dependency. The only way in when the menu is
	 * unreachable (UI dead / boot loop before menu init). */
	gpio_set_direction(FLASH_MODE_FORCE_GPIO, GPIO_MODE_INPUT);
	gpio_set_pull_mode(FLASH_MODE_FORCE_GPIO, GPIO_PULLUP_ONLY);
	if (flash_mode_force_key_held()) {
		s_flash_mode_active = true;
		ESP_LOGW(TAG, "FLASH MODE: forced by key hold at boot");
		return true;
	}

	/* 2) Menu path: one-shot NVS flag, cleared immediately so the NEXT
	 * reset (power cycle or esptool hard_reset after flashing) boots
	 * normally. */
	bool enabled = false;
	if (!mach_s3_settings_persist_get_flash_mode(&enabled)) {
		return false;
	}
	if (!enabled) {
		return false;
	}
	mach_s3_settings_persist_set_flash_mode(false);
	s_flash_mode_active = true;
	ESP_LOGW(TAG, "FLASH MODE: USB Host skipped, USB-Serial-JTAG exposed for browser flashing");
	return true;
}
