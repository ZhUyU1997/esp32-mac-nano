#include "machine_backend.h"

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "esp_system.h"
#include "framebuffer.h"
#include "macplus.h"
#include "msg.h"
#include "sony.h"
#include "settings_ui.h"
#include "settings_persist.h"
#include "ui_strings.h"

#define FLOPPY_DIR "/sdcard"
#define BACKLIGHT_HW_MIN 1u

#define FLOPPY_SIZE_400K (400u * 1024u)
#define FLOPPY_SIZE_800K (800u * 1024u)

block_t *block_create_from_file(const char *path);
block_t *block_create_from_file(const char *path);

/* ── Helpers ────────────────────────────────────────────────────────── */

static bool name_match(const char *name)
{
	if (name == NULL)
		return false;
	const size_t n = strlen(name);
	if (n < 5)
		return false;
	const char *ext = name + n - 4;
	return strcmp(ext, ".img") == 0 || strcmp(ext, ".dsk") == 0;
}

static bool is_floppy_image_size(off_t sz)
{
	if (sz <= 0)
		return false;
	const uint32_t s = (uint32_t)sz;
	return (s == FLOPPY_SIZE_400K) || (s == FLOPPY_SIZE_800K);
}

static uint8_t backlight_ui_to_hw(uint8_t ui_value)
{
	uint32_t v = ui_value;
	if (v > 100u)
		v = 100u;
	const uint32_t span = 100u - BACKLIGHT_HW_MIN;
	const uint32_t hw = BACKLIGHT_HW_MIN + ((span * v * v + 5000u) / 10000u);
	return (uint8_t)hw;
}

static uint8_t backlight_hw_to_ui(uint8_t hw_value)
{
	uint32_t hw = hw_value;
	if (hw > 100u)
		hw = 100u;
	if (hw <= BACKLIGHT_HW_MIN)
		return 0u;
	uint32_t lo = 0, hi = 100;
	while (lo < hi) {
		const uint32_t mid = (lo + hi + 1u) / 2u;
		const uint32_t mapped = (mid * mid + 50u) / 100u;
		if (mapped <= hw)
			lo = mid;
		else
			hi = mid - 1u;
	}
	return (uint8_t)lo;
}

/* ── Try to map a persisted floppy path to an entry index ──────────── */

static void try_map_inserted_index(machine_backend_t *fb)
{
	assert(fb != NULL);
	if (fb->mac == NULL || fb->count == 0 || fb->inserted_idx >= 0)
		return;
	if (!mac_sony_disk_in_place(&fb->mac->sony, 1))
		return;

	char persisted_path[FLOPPY_MAX_PATH];
	if (!mach_s3_settings_persist_get_floppy_path(persisted_path, sizeof(persisted_path)))
		return;
	for (uint16_t i = 0; i < fb->count; i++) {
		if (strcmp(fb->entries[i].path, persisted_path) == 0) {
			fb->inserted_idx = (int16_t)i;
			return;
		}
	}
}

/* ── Scan SD card for floppy images ─────────────────────────────────── */

static void scan(machine_backend_t *fb)
{
	assert(fb != NULL);
	fb->count = 0;
	DIR *dir = opendir(FLOPPY_DIR);
	if (dir == NULL)
		return;

	for (;;) {
		if (fb->count >= FLOPPY_MAX_ENTRIES)
			break;
		struct dirent *de = readdir(dir);
		if (de == NULL)
			break;
		if (!name_match(de->d_name))
			continue;

		char path[FLOPPY_MAX_PATH];
		const int n = snprintf(path, sizeof(path), "%s/%s", FLOPPY_DIR, de->d_name);
		if (n <= 0 || n >= (int)sizeof(path))
			continue;

		struct stat st;
		if (stat(path, &st) != 0 || !S_ISREG(st.st_mode))
			continue;
		if (!is_floppy_image_size(st.st_size))
			continue;

		floppy_entry_t *e = &fb->entries[fb->count];
		(void)snprintf(e->name, sizeof(e->name), "%s", de->d_name);
		(void)snprintf(e->path, sizeof(e->path), "%s", path);
		fb->count++;
	}
	(void)closedir(dir);

	/* insertion sort by name */
	for (uint16_t i = 1; i < fb->count; i++) {
		floppy_entry_t key = fb->entries[i];
		uint16_t j = i;
		while (j > 0 && strcmp(fb->entries[j - 1].name, key.name) > 0) {
			fb->entries[j] = fb->entries[j - 1];
			j--;
		}
		fb->entries[j] = key;
	}

	if (fb->inserted_idx >= (int16_t)fb->count)
		fb->inserted_idx = -1;
	try_map_inserted_index(fb);
}

/* ── Callback implementations ──────────────────────────────────────────── */

static uint16_t cb_get_count(machine_backend_t *fb)
{
	assert(fb != NULL);
	scan(fb);
	return fb->count;
}

static const char *cb_get_name(machine_backend_t *fb, uint16_t idx)
{
	assert(fb != NULL);
	if (idx >= fb->count)
		return NULL;
	return fb->entries[idx].name;
}

static bool insert_now(machine_backend_t *fb, uint16_t idx);

static int16_t cb_get_inserted(machine_backend_t *fb)
{
	assert(fb != NULL);
	if (fb->mac == NULL) {
		fb->inserted_idx = -1;
		return -1;
	}
	if (!mac_sony_disk_in_place(&fb->mac->sony, 1)) {
		if (fb->inserted_idx >= 0 || mac_floppy_get(fb->mac) != NULL)
			(void)mach_s3_settings_persist_clear_floppy_path();
		fb->inserted_idx = -1;
		return -1;
	}
	if (fb->inserted_idx < 0)
		return -2;
	return fb->inserted_idx;
}

static bool cb_insert(machine_backend_t *fb, uint16_t idx, char *msg, size_t msg_len)
{
	assert(fb != NULL);
	if (msg != NULL && msg_len > 0)
		msg[0] = '\0';
	if (idx >= fb->count || fb->mac == NULL) {
		if (msg != NULL && msg_len > 0)
			(void)snprintf(msg, msg_len, UI_STR_CANNOT_INSERT_INVALID);
		return false;
	}
	if (fb->insert_selected) {
		if (msg != NULL && msg_len > 0)
			(void)snprintf(msg, msg_len, UI_STR_DISK_OCCUPIED);
		return false;
	}
	if (mac_sony_disk_in_place(&fb->mac->sony, 1)) {
		if (msg != NULL && msg_len > 0)
			(void)snprintf(msg, msg_len, UI_STR_DISK_OCCUPIED);
		return false;
	}
	(void)insert_now(fb, idx);
	fb->insert_selected = true;
	if (msg != NULL && msg_len > 0)
		(void)snprintf(msg, msg_len, UI_STR_INSERTED);
	return true;
}

static bool cb_eject(machine_backend_t *fb, char *msg, size_t msg_len)
{
	if (msg != NULL && msg_len > 0)
		(void)snprintf(msg, msg_len, UI_STR_CANNOT_EJECT_HERE);
	return false;
}

static uint8_t cb_backlight_get(machine_backend_t *fb)
{
	assert(fb != NULL);
	if (fb->lcd == NULL)
		return 75u;
	const int v = framebuffer_get_backlight(fb->lcd);
	if (v < 0)
		return 75u;
	if (v > 100)
		return 100u;
	return backlight_hw_to_ui((uint8_t)v);
}

static void cb_backlight_set(machine_backend_t *fb, uint8_t value)
{
	assert(fb != NULL);
	if (value > 100u)
		value = 100u;
	if (fb->lcd == NULL)
		return;
	const uint8_t hw = backlight_ui_to_hw(value);
	framebuffer_set_backlight(fb->lcd, (int)hw);
}

static void cb_backlight_commit(machine_backend_t *fb, uint8_t value)
{
	if (value > 100u)
		value = 100u;
	(void)mach_s3_settings_persist_set_backlight(value);
}

static uint8_t cb_volume_get(machine_backend_t *fb)
{
	assert(fb != NULL);
	if (fb->sound_volume > 25u)
		return 25u;
	return fb->sound_volume;
}

static void cb_volume_set(machine_backend_t *fb, uint8_t value)
{
	assert(fb != NULL);
	if (value > 100u)
		value = 100u;
	if (value > 25u)
		value = 25u;
	fb->sound_volume = value;
	if (fb->mac != NULL)
		mac_sound_set_master_volume(&fb->mac->sound, (unsigned)value);
}

static void cb_volume_commit(machine_backend_t *fb, uint8_t value)
{
	if (value > 100u)
		value = 100u;
	if (value > 25u)
		value = 25u;
	(void)mach_s3_settings_persist_set_sound_volume(value);
}

static bool cb_mute_get(machine_backend_t *fb)
{
	assert(fb != NULL);
	return fb->sound_mute;
}

static void cb_mute_set(machine_backend_t *fb, bool enabled)
{
	assert(fb != NULL);
	fb->sound_mute = enabled;
	(void)mach_s3_settings_persist_set_sound_mute(enabled);
}

/* Reboot: backlight off (restart flash invisible) → esp_restart(). */
void machine_backend_reboot(void)
{
	framebuffer_t *lcd = framebuffer_lookup("lcd");
	if (lcd != NULL) {
		framebuffer_set_backlight(lcd, 0);
	}
	esp_restart();
}

static void cb_reboot(machine_backend_t *fb)
{
	(void)fb;
	machine_backend_reboot();
}

/* ── Insert now: submit immediately (paused: queued — dispatched on
 * resume). One insert per pause-menu session — insert_selected locks. */

static bool insert_now(machine_backend_t *fb, uint16_t idx)
{
	/* All floppy inserts go through the message queue (dispatched on the
	 * emulator thread) — one path for every producer. UI state (index +
	 * persisted path) is set here; the insert itself happens async. */
	mac_msg_submit("floppy.insert", fb->entries[idx].path);
	fb->inserted_idx = (int16_t)idx;
	(void)mach_s3_settings_persist_set_floppy_path(fb->entries[idx].path);
	return true;
}

/* ── Public API ──────────────────────────────────────────────────────── */

machine_backend_t *machine_backend_create(struct macplus *mac, struct framebuffer_t *lcd)
{
	machine_backend_t *fb = (machine_backend_t *)calloc(1, sizeof(*fb));
	if (fb == NULL)
		return NULL;

	fb->mac = mac;
	fb->lcd = lcd;
	fb->get_count = cb_get_count;
	fb->get_name = cb_get_name;
	fb->get_inserted = cb_get_inserted;
	fb->insert = cb_insert;
	fb->eject = cb_eject;
	fb->get_backlight = cb_backlight_get;
	fb->set_backlight = cb_backlight_set;
	fb->commit_backlight = cb_backlight_commit;
	fb->get_volume = cb_volume_get;
	fb->set_volume = cb_volume_set;
	fb->commit_volume = cb_volume_commit;
	fb->get_mute = cb_mute_get;
	fb->set_mute = cb_mute_set;
	fb->reboot = cb_reboot;
	fb->inserted_idx = -1;
	fb->sound_volume = 15u;
	fb->sound_mute = false;

	fb->mac->sony.enable = 1;

	/* Restore persisted backlight */
	{
		uint8_t persisted = 0;
		if (mach_s3_settings_persist_get_backlight(&persisted)) {
			framebuffer_set_backlight(lcd, (int)backlight_ui_to_hw(persisted));
		} else {
			framebuffer_set_backlight(lcd, (int)backlight_ui_to_hw(75u));
		}
	}

	/* Restore persisted volume & mute */
	{
		uint8_t persisted = 0;
		if (mach_s3_settings_persist_get_sound_volume(&persisted))
			fb->sound_volume = persisted;
		if (fb->sound_volume > 25u)
			fb->sound_volume = 25u;
		mac_sound_set_master_volume(&fb->mac->sound, (unsigned)fb->sound_volume);
		bool persisted_mute = false;
		if (mach_s3_settings_persist_get_sound_mute(&persisted_mute))
			fb->sound_mute = persisted_mute;
	}

	return fb;
}

void machine_backend_reset_insert_selected(machine_backend_t *fb)
{
	assert(fb != NULL);
	fb->insert_selected = false;
}
