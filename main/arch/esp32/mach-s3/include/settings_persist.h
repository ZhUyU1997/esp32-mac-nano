#ifndef MACH_S3_SETTINGS_PERSIST_H
#define MACH_S3_SETTINGS_PERSIST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "block/block.h"

bool mach_s3_settings_persist_set_floppy_path(const char *path);
bool mach_s3_settings_persist_clear_floppy_path(void);
bool mach_s3_settings_persist_get_floppy_path(char *out_path, size_t out_len);
/* Validate the persisted floppy path (exists, 400K/800K); clears it if
 * invalid. Returns true with out_path filled. */
bool mach_s3_settings_persist_get_restored_floppy_path(char *out_path, size_t out_len);

bool mach_s3_settings_persist_set_backlight(uint8_t value);
bool mach_s3_settings_persist_get_backlight(uint8_t *out_value);
bool mach_s3_settings_persist_set_sound_volume(uint8_t value);
bool mach_s3_settings_persist_get_sound_volume(uint8_t *out_value);
bool mach_s3_settings_persist_set_sound_mute(bool muted);
bool mach_s3_settings_persist_get_sound_mute(bool *out_muted);

/* One-shot "flash mode" flag: set before reboot, consumed (cleared) at
 * the next boot's flash_mode_check(). */
bool mach_s3_settings_persist_set_flash_mode(bool enabled);
bool mach_s3_settings_persist_get_flash_mode(bool *out_enabled);

bool mach_s3_settings_persist_set_pram_vol(uint8_t vol);
bool mach_s3_settings_persist_get_pram_vol(uint8_t *out_vol);

#endif
