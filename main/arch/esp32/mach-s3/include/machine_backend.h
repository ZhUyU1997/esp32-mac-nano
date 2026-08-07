#ifndef MACHINE_BACKEND_H
#define MACHINE_BACKEND_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "block/block.h"

struct macplus;
struct framebuffer_t;

/* ── Forward decl for callback typedefs ────────────────────────────── */

typedef struct machine_backend_s machine_backend_t;

/* ── Callback types ────────────────────────────────────────────────── */

typedef uint16_t (*machine_get_count_fn)(machine_backend_t *fb);
typedef const char *(*machine_get_name_fn)(machine_backend_t *fb, uint16_t idx);
typedef int16_t (*machine_get_inserted_fn)(machine_backend_t *fb);
typedef bool (*machine_insert_fn)(machine_backend_t *fb, uint16_t idx, char *msg, size_t msg_len);
typedef bool (*machine_eject_fn)(machine_backend_t *fb, char *msg, size_t msg_len);

typedef uint8_t (*machine_get_backlight_fn)(machine_backend_t *fb);
typedef void (*machine_set_backlight_fn)(machine_backend_t *fb, uint8_t value);
typedef void (*machine_commit_backlight_fn)(machine_backend_t *fb, uint8_t value);

typedef uint8_t (*machine_get_volume_fn)(machine_backend_t *fb);
typedef void (*machine_set_volume_fn)(machine_backend_t *fb, uint8_t value);
typedef void (*machine_commit_volume_fn)(machine_backend_t *fb, uint8_t value);
typedef bool (*machine_get_mute_fn)(machine_backend_t *fb);
typedef void (*machine_set_mute_fn)(machine_backend_t *fb, bool enabled);

typedef void (*machine_reboot_fn)(machine_backend_t *fb);

/* ── Backend struct ────────────────────────────────────────────────── */

#define FLOPPY_MAX_ENTRIES 16
#define FLOPPY_MAX_NAME 64
#define FLOPPY_MAX_PATH 256

typedef struct {
	char name[FLOPPY_MAX_NAME];
	char path[FLOPPY_MAX_PATH];
} floppy_entry_t;

struct machine_backend_s {
	machine_get_count_fn get_count;
	machine_get_name_fn get_name;
	machine_get_inserted_fn get_inserted;
	machine_insert_fn insert;
	machine_eject_fn eject;
	machine_get_backlight_fn get_backlight;
	machine_set_backlight_fn set_backlight;
	machine_commit_backlight_fn commit_backlight;
	machine_get_volume_fn get_volume;
	machine_set_volume_fn set_volume;
	machine_commit_volume_fn commit_volume;
	machine_get_mute_fn get_mute;
	machine_set_mute_fn set_mute;
	machine_reboot_fn reboot;

	/* Internal state */
	struct macplus *mac;
	floppy_entry_t entries[FLOPPY_MAX_ENTRIES];
	uint16_t count;
	int16_t inserted_idx;
	bool insert_selected;   /* one insert per pause-menu session (locked after pick) */
	struct framebuffer_t *lcd;
	uint8_t sound_volume;
	bool sound_mute;
};

machine_backend_t *machine_backend_create(struct macplus *mac, struct framebuffer_t *lcd);
void machine_backend_reset_insert_selected(machine_backend_t *backend);

/* Graceful reboot: backlight off → SD flush/unmount → esp_restart */
void machine_backend_reboot(void);

#endif /* MACHINE_BACKEND_H */
