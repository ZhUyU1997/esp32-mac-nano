#include "settings_persist.h"

#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>

#include "esp_err.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

#define UI_FLOPPY_NVS_NS "mini_mac"
#define UI_FLOPPY_NVS_KEY "last_floppy"
#define UI_SETTINGS_BACKLIGHT_KEY "backlight"
#define UI_SETTINGS_SOUND_VOLUME_KEY "sound_volume"
#define UI_SETTINGS_SOUND_MUTE_KEY "sound_mute"
#define UI_FLASH_MODE_KEY "flash_mode"
#define FLOPPY_SIZE_400K (400u * 1024u)
#define FLOPPY_SIZE_800K (800u * 1024u)
static const char *k_tag = "settings_persist";

block_t *block_create_from_file(const char *path);

static bool persist_init_once(void)
{
	static bool ready = false;
	if (ready) {
		return true;
	}
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_LOGW(k_tag, "nvs init requires erase: %s", esp_err_to_name(err));
		(void)nvs_flash_erase();
		err = nvs_flash_init();
	}
	if (err != ESP_OK) {
		ESP_LOGE(k_tag, "nvs init failed: %s", esp_err_to_name(err));
		return false;
	}
	ESP_LOGI(k_tag, "nvs init ok");
	ready = true;
	return true;
}

static bool persist_set_u8(const char *key, uint8_t value)
{
	if (key == NULL || key[0] == '\0' || !persist_init_once()) {
		ESP_LOGW(k_tag, "set_u8 skipped (invalid key or nvs not ready)");
		return false;
	}
	nvs_handle_t h = 0;
	const esp_err_t err_open = nvs_open(UI_FLOPPY_NVS_NS, NVS_READWRITE, &h);
	if (err_open != ESP_OK) {
		ESP_LOGE(k_tag, "nvs_open(set_u8:%s) failed: %s", key, esp_err_to_name(err_open));
		return false;
	}
	const esp_err_t err_set = nvs_set_u8(h, key, value);
	const esp_err_t err_commit = (err_set == ESP_OK) ? nvs_commit(h) : err_set;
	nvs_close(h);
	if (err_commit != ESP_OK) {
		ESP_LOGE(k_tag, "set_u8(%s=%u) failed: %s", key, (unsigned)value, esp_err_to_name(err_commit));
		return false;
	}
	ESP_LOGI(k_tag, "set_u8(%s=%u) ok", key, (unsigned)value);
	return true;
}

static bool persist_get_u8(const char *key, uint8_t *out_value)
{
	if (key == NULL || key[0] == '\0' || out_value == NULL || !persist_init_once()) {
		ESP_LOGW(k_tag, "get_u8 skipped (invalid args or nvs not ready)");
		return false;
	}
	nvs_handle_t h = 0;
	const esp_err_t err_open = nvs_open(UI_FLOPPY_NVS_NS, NVS_READONLY, &h);
	if (err_open != ESP_OK) {
		ESP_LOGE(k_tag, "nvs_open(get_u8:%s) failed: %s", key, esp_err_to_name(err_open));
		return false;
	}
	uint8_t value = 0;
	const esp_err_t err = nvs_get_u8(h, key, &value);
	nvs_close(h);
	if (err != ESP_OK) {
		ESP_LOGI(k_tag, "get_u8(%s) miss: %s", key, esp_err_to_name(err));
		return false;
	}
	*out_value = value;
	ESP_LOGI(k_tag, "get_u8(%s=%u) hit", key, (unsigned)value);
	return true;
}

static bool persist_set_str(const char *key, const char *value)
{
	if (key == NULL || key[0] == '\0' || value == NULL || value[0] == '\0' || !persist_init_once()) {
		ESP_LOGW(k_tag, "set_str skipped (invalid key/value or nvs not ready)");
		return false;
	}
	nvs_handle_t h = 0;
	const esp_err_t err_open = nvs_open(UI_FLOPPY_NVS_NS, NVS_READWRITE, &h);
	if (err_open != ESP_OK) {
		ESP_LOGE(k_tag, "nvs_open(set_str:%s) failed: %s", key, esp_err_to_name(err_open));
		return false;
	}
	const esp_err_t err_set = nvs_set_str(h, key, value);
	const esp_err_t err_commit = (err_set == ESP_OK) ? nvs_commit(h) : err_set;
	nvs_close(h);
	if (err_commit == ESP_OK) {
		ESP_LOGI(k_tag, "set_str(%s=%s) ok", key, value);
	} else {
		ESP_LOGE(k_tag, "set_str(%s) failed: %s", key, esp_err_to_name(err_commit));
	}
	return err_commit == ESP_OK;
}

static bool persist_get_str(const char *key, char *out_value, size_t out_len)
{
	if (key == NULL || key[0] == '\0' || out_value == NULL || out_len < 2 || !persist_init_once()) {
		ESP_LOGW(k_tag, "get_str skipped (invalid args or nvs not ready)");
		return false;
	}
	out_value[0] = '\0';
	nvs_handle_t h = 0;
	const esp_err_t err_open = nvs_open(UI_FLOPPY_NVS_NS, NVS_READONLY, &h);
	if (err_open != ESP_OK) {
		ESP_LOGE(k_tag, "nvs_open(get_str:%s) failed: %s", key, esp_err_to_name(err_open));
		return false;
	}
	size_t req = out_len;
	const esp_err_t err = nvs_get_str(h, key, out_value, &req);
	nvs_close(h);
	if (err != ESP_OK || out_value[0] == '\0') {
		if (err != ESP_OK) {
			ESP_LOGI(k_tag, "get_str(%s) miss: %s", key, esp_err_to_name(err));
		}
		return false;
	}
	ESP_LOGI(k_tag, "get_str(%s=%s) hit", key, out_value);
	return true;
}

static bool persist_erase_key(const char *key)
{
	if (key == NULL || key[0] == '\0' || !persist_init_once()) {
		ESP_LOGW(k_tag, "erase_key skipped (invalid key or nvs not ready)");
		return false;
	}
	nvs_handle_t h = 0;
	const esp_err_t err_open = nvs_open(UI_FLOPPY_NVS_NS, NVS_READWRITE, &h);
	if (err_open != ESP_OK) {
		ESP_LOGE(k_tag, "nvs_open(erase:%s) failed: %s", key, esp_err_to_name(err_open));
		return false;
	}
	const esp_err_t err_erase = nvs_erase_key(h, key);
	if (err_erase == ESP_OK || err_erase == ESP_ERR_NVS_NOT_FOUND) {
		const esp_err_t err_commit = nvs_commit(h);
		if (err_commit == ESP_OK) {
			ESP_LOGI(k_tag, "erase_key(%s) ok", key);
		} else {
			ESP_LOGE(k_tag, "erase_key(%s) commit failed: %s", key, esp_err_to_name(err_commit));
		}
		nvs_close(h);
		return err_commit == ESP_OK;
	} else {
		ESP_LOGE(k_tag, "erase_key(%s) failed: %s", key, esp_err_to_name(err_erase));
	}
	nvs_close(h);
	return false;
}

bool mach_s3_settings_persist_set_floppy_path(const char *path)
{
	return persist_set_str(UI_FLOPPY_NVS_KEY, path);
}

bool mach_s3_settings_persist_clear_floppy_path(void)
{
	return persist_erase_key(UI_FLOPPY_NVS_KEY);
}

bool mach_s3_settings_persist_get_floppy_path(char *out_path, size_t out_len)
{
	return persist_get_str(UI_FLOPPY_NVS_KEY, out_path, out_len);
}

bool mach_s3_settings_persist_get_restored_floppy_path(char *out_path, size_t out_len)
{
	if (out_path == NULL)
		return false;
	out_path[0] = '\0';

	if (!mach_s3_settings_persist_get_floppy_path(out_path, out_len)) {
		ESP_LOGI(k_tag, "load_block: no persisted path");
		return false;
	}

	struct stat st;
	if (stat(out_path, &st) != 0 || !S_ISREG(st.st_mode)) {
		ESP_LOGW(k_tag, "load_block: invalid path, clearing: %s", out_path);
		goto clear;
	}

	const uint32_t s = (uint32_t)st.st_size;
	if (s != FLOPPY_SIZE_400K && s != FLOPPY_SIZE_800K) {
		ESP_LOGW(k_tag, "load_block: invalid floppy size=%u, clearing: %s", s, out_path);
		goto clear;
	}

	ESP_LOGI(k_tag, "load_block: restored path ok: %s", out_path);
	return true;

clear:
	(void)mach_s3_settings_persist_clear_floppy_path();
	out_path[0] = '\0';
	return false;
}

bool mach_s3_settings_persist_set_backlight(uint8_t value)
{
	if (value > 100u) {
		value = 100u;
	}
	return persist_set_u8(UI_SETTINGS_BACKLIGHT_KEY, value);
}

bool mach_s3_settings_persist_get_backlight(uint8_t *out_value)
{
	if (out_value == NULL) {
		return false;
	}
	return persist_get_u8(UI_SETTINGS_BACKLIGHT_KEY, out_value);
}

bool mach_s3_settings_persist_set_sound_volume(uint8_t value)
{
	if (value > 100u) {
		value = 100u;
	}
	return persist_set_u8(UI_SETTINGS_SOUND_VOLUME_KEY, value);
}

bool mach_s3_settings_persist_get_sound_volume(uint8_t *out_value)
{
	if (out_value == NULL) {
		return false;
	}
	return persist_get_u8(UI_SETTINGS_SOUND_VOLUME_KEY, out_value);
}

bool mach_s3_settings_persist_set_sound_mute(bool muted)
{
	return persist_set_u8(UI_SETTINGS_SOUND_MUTE_KEY, muted ? 1u : 0u);
}

bool mach_s3_settings_persist_get_sound_mute(bool *out_muted)
{
	if (out_muted == NULL) {
		return false;
	}
	uint8_t value = 0;
	if (!persist_get_u8(UI_SETTINGS_SOUND_MUTE_KEY, &value)) {
		return false;
	}
	*out_muted = (value != 0u);
	return true;
}

bool mach_s3_settings_persist_set_flash_mode(bool enabled)
{
	return persist_set_u8(UI_FLASH_MODE_KEY, enabled ? 1u : 0u);
}

bool mach_s3_settings_persist_get_flash_mode(bool *out_enabled)
{
	if (out_enabled == NULL) {
		return false;
	}
	uint8_t value = 0;
	if (!persist_get_u8(UI_FLASH_MODE_KEY, &value)) {
		return false;
	}
	*out_enabled = (value != 0u);
	return true;
}

#define UI_SETTINGS_PRAM_VOL_KEY "pram_vol"

bool mach_s3_settings_persist_set_pram_vol(uint8_t vol)
{
	return persist_set_u8(UI_SETTINGS_PRAM_VOL_KEY, vol);
}

bool mach_s3_settings_persist_get_pram_vol(uint8_t *out_vol)
{
	if (out_vol == NULL) return false;
	return persist_get_u8(UI_SETTINGS_PRAM_VOL_KEY, out_vol);
}
