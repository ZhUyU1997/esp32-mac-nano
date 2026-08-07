#ifndef UPGRADE_SDCARD_H
#define UPGRADE_SDCARD_H

#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"

/**
 * @brief Progress callback for upgrade UI.
 * @param percent  0–100
 * @param stage    Short stage description, e.g. "Writing firmware..."
 */
typedef void (*upgrade_progress_fn)(int percent, const char *stage);

/**
 * @brief Check for upgrade.bin on SD card and perform upgrade if found.
 *
 * Scans the SD card for the upgrade file. If found, parses the custom
 * ESUP-format header and writes each entry to its target partition
 * (firmware → ota_0/ota_1 via esp_ota, hd → hd partition directly).
 *
 * The upgrade file is DELETED on success to prevent re-upgrade on reboot.
 * On failure, the file is left in place (or optionally renamed) so the
 * user can debug.
 *
 * @param upgrade_path  Path on SD card, e.g. "/sdcard/upgrade.bin"
 * @return
 *   - ESP_OK on success (device will be reset before return)
 *   - ESP_ERR_NOT_FOUND if the file does not exist (no action taken)
 *   - Other error codes on failure
 */
esp_err_t upgrade_check_and_run(const char *upgrade_path);

/**
 * @brief Run an SD card upgrade with a progress callback.
 *
 * Same as upgrade_check_and_run() but calls progress_cb(percent, stage)
 * during the streaming write so the caller can update a UI.
 *
 * @param upgrade_path  Path on SD card, e.g. "/sdcard/upgrade.bin"
 * @param progress_cb   Callback for progress updates (may be NULL)
 * @return
 *   - ESP_OK on success (device will be reset before return)
 *   - ESP_ERR_NOT_FOUND if the file does not exist (no action taken)
 *   - Other error codes on failure
 */
esp_err_t upgrade_run_with_progress(const char *upgrade_path, upgrade_progress_fn progress_cb);

/**
 * @brief Check if an upgrade file exists (without running it).
 * @param path  Path on SD card
 * @return true if the file exists
 */
bool upgrade_file_exists(const char *path);

/**
 * @brief Mark the currently running firmware as valid (cancel rollback).
 */
void upgrade_mark_app_valid(void);

#endif /* UPGRADE_SDCARD_H */
