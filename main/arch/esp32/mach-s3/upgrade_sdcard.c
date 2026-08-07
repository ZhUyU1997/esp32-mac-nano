#include "upgrade_sdcard.h"

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "sdcard_upgrade";

/* ── upgrade.bin file format ───────────────────────────────────────────
 *
 *  Offset  Size  Field
 *  0       4     Magic "ESUP"
 *  4       2     Version (uint16 LE)
 *  6       2     Entry count (uint16 LE)
 *  8       2     Reserved (CRC placeholder, 0)
 *  10      N*16  Entry table (N = count)
 *
 *  Entry format (16 bytes each):
 *    [0..3]   data_offset (uint32 LE, from start of file)
 *    [4..7]   data_size   (uint32 LE)
 *    [8..15]  partition name (char[8], null-padded)
 *
 *  After the entry table comes the raw data blobs.
 * ────────────────────────────────────────────────────────────────────────
 */

#define ESUP_MAGIC "ESUP"
#define ESUP_HEADER_SZ 10 /* magic4 + ver2 + count2 + rsvd2 */
#define ESUP_ENTRY_SZ 16
#define OTA_BUF_SIZE 4096

/* Convenience: report progress / error only when callback is set */
#define PROGRESS(pct, msg)                                                                                                                                     \
	do {                                                                                                                                                   \
		if (progress_cb)                                                                                                                               \
			progress_cb(pct, msg);                                                                                                                 \
	} while (0)
#define PROGRESS_ERR(msg) PROGRESS(-1, msg)

/* ── Public API ────────────────────────────────────────────────────────── */

void upgrade_mark_app_valid(void)
{
	const esp_partition_t *running = esp_ota_get_running_partition();
	if (running == NULL)
		return;

	esp_ota_img_states_t state;
	if (esp_ota_get_state_partition(running, &state) != ESP_OK)
		return;

	if (state == ESP_OTA_IMG_PENDING_VERIFY) {
		ESP_LOGI(TAG, "Marking current firmware (%s) as valid", running->label);
		esp_ota_mark_app_valid_cancel_rollback();
	}
}

bool upgrade_file_exists(const char *path)
{
	FILE *fp = fopen(path, "rb");
	if (fp == NULL)
		return false;
	fclose(fp);
	return true;
}

/**
 * @brief Internal: run upgrade with optional progress callback.
 */
static esp_err_t upgrade_do_run(const char *upgrade_path, upgrade_progress_fn progress_cb)
{
	FILE *fp = fopen(upgrade_path, "rb");
	if (fp == NULL) {
		ESP_LOGW(TAG, "No upgrade file found: %s", upgrade_path);
		return ESP_ERR_NOT_FOUND;
	}

	ESP_LOGI(TAG, "Upgrade file found: %s", upgrade_path);

	/* ── Read header ──────────────────────────────────────────────── */
	uint8_t header[ESUP_HEADER_SZ];
	if (fread(header, 1, ESUP_HEADER_SZ, fp) != ESUP_HEADER_SZ) {
		ESP_LOGE(TAG, "Failed to read header");
		PROGRESS_ERR("Read header failed");
		fclose(fp);
		return ESP_FAIL;
	}

	if (memcmp(header, ESUP_MAGIC, 4) != 0) {
		ESP_LOGE(TAG, "Bad magic: expected 'ESUP', got '%.4s'", header);
		PROGRESS_ERR("Invalid upgrade file");
		fclose(fp);
		return ESP_ERR_INVALID_VERSION;
	}

	uint16_t version = header[4] | ((uint16_t)header[5] << 8);
	uint16_t count = header[6] | ((uint16_t)header[7] << 8);

	if (version != 1) {
		ESP_LOGE(TAG, "Unsupported version: %u", version);
		PROGRESS_ERR("Unsupported upgrade version");
		fclose(fp);
		return ESP_ERR_INVALID_VERSION;
	}

	if (count == 0) {
		ESP_LOGE(TAG, "No entries in upgrade file");
		PROGRESS_ERR("Empty upgrade file");
		fclose(fp);
		return ESP_FAIL;
	}

	ESP_LOGI(TAG, "Upgrade file: version=%u, entries=%u", version, count);

	/* ── Read entry table ─────────────────────────────────────────── */
	size_t entry_table_sz = count * ESUP_ENTRY_SZ;
	uint8_t *entries = malloc(entry_table_sz);
	if (entries == NULL) {
		ESP_LOGE(TAG, "Failed to allocate entry table (%zu bytes)", entry_table_sz);
		PROGRESS_ERR("Out of memory");
		fclose(fp);
		return ESP_ERR_NO_MEM;
	}

	if (fread(entries, 1, entry_table_sz, fp) != entry_table_sz) {
		ESP_LOGE(TAG, "Failed to read entry table");
		PROGRESS_ERR("Read entry table failed");
		free(entries);
		fclose(fp);
		return ESP_FAIL;
	}

	/* ── Process each entry (streaming: no large malloc) ─────────────── */
	esp_err_t overall_err = ESP_OK;
	bool reboot_needed = false;
	uint8_t *buf = malloc(OTA_BUF_SIZE);
	if (buf == NULL) {
		ESP_LOGE(TAG, "Failed to allocate I/O buffer (%d bytes)", OTA_BUF_SIZE);
		PROGRESS_ERR("Out of memory");
		free(entries);
		fclose(fp);
		return ESP_ERR_NO_MEM;
	}

	for (uint16_t i = 0; i < count; i++) {
		const uint8_t *ent = entries + (i * ESUP_ENTRY_SZ);
		uint32_t data_offset = ent[0] | ((uint32_t)ent[1] << 8) | ((uint32_t)ent[2] << 16) | ((uint32_t)ent[3] << 24);
		uint32_t data_size = ent[4] | ((uint32_t)ent[5] << 8) | ((uint32_t)ent[6] << 16) | ((uint32_t)ent[7] << 24);
		char part_name[9];
		memcpy(part_name, ent + 8, 8);
		part_name[8] = '\0';

		ESP_LOGI(TAG, "Entry %u: name='%s' offset=%" PRIu32 " size=%" PRIu32, i, part_name, data_offset, data_size);

		/* Seek to the data in the file */
		if (fseek(fp, (long)data_offset, SEEK_SET) != 0) {
			ESP_LOGE(TAG, "Seek to offset %" PRIu32 " failed", data_offset);
			PROGRESS_ERR("SD card seek error");
			overall_err = ESP_FAIL;
			break;
		}

		/* ── Write firmware (OTA) ─────────────────────────────────── */
		if (strcmp(part_name, "factory") == 0 || strcmp(part_name, "ota_0") == 0) {
			/* Dual OTA: write to the inactive slot */
			const esp_partition_t *target = esp_ota_get_next_update_partition(NULL);
			if (target == NULL) {
				ESP_LOGE(TAG, "No inactive OTA partition available");
				PROGRESS_ERR("No OTA slot available");
				overall_err = ESP_FAIL;
				break;
			}
			if (data_size > target->size) {
				ESP_LOGE(TAG, "Firmware too large: %" PRIu32 " > %" PRIu32, data_size, target->size);
				PROGRESS_ERR("Firmware too large for partition");
				overall_err = ESP_ERR_INVALID_SIZE;
				break;
			}

			const esp_partition_t *running = esp_ota_get_running_partition();
			ESP_LOGI(TAG, "Running from %s, writing to %s (0x%08" PRIx32 ")", running ? running->label : "?", target->label, target->address);

			esp_ota_handle_t ota_handle;
			overall_err = esp_ota_begin(target, data_size, &ota_handle);
			if (overall_err != ESP_OK) {
				ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(overall_err));
				PROGRESS_ERR(esp_err_to_name(overall_err));
				break;
			}

			/* Stream firmware from SD card → OTA flash */
			PROGRESS(0, "Writing firmware...");
			uint32_t remaining = data_size;
			int last_pct = -1;
			while (remaining > 0) {
				size_t chunk = (remaining < OTA_BUF_SIZE) ? remaining : OTA_BUF_SIZE;
				if (fread(buf, 1, chunk, fp) != chunk) {
					ESP_LOGE(TAG, "Read error at offset %" PRIu32, data_offset);
					PROGRESS_ERR("SD card read error");
					esp_ota_abort(ota_handle);
					overall_err = ESP_FAIL;
					break;
				}
				overall_err = esp_ota_write(ota_handle, buf, chunk);
				if (overall_err != ESP_OK) {
					ESP_LOGE(TAG, "esp_ota_write failed: %s", esp_err_to_name(overall_err));
					PROGRESS_ERR(esp_err_to_name(overall_err));
					esp_ota_abort(ota_handle);
					break;
				}
				remaining -= chunk;
				int pct = (int)((data_size - remaining) * 100 / data_size);
				if (pct != last_pct) {
					ESP_LOGI(TAG, "  firmware: %d%%", pct);
					PROGRESS(pct, "Writing firmware...");
					last_pct = pct;
				}
			}
			if (overall_err != ESP_OK)
				break;

			overall_err = esp_ota_end(ota_handle);
			if (overall_err != ESP_OK) {
				ESP_LOGE(TAG, "esp_ota_end failed: %s", esp_err_to_name(overall_err));
				PROGRESS_ERR(esp_err_to_name(overall_err));
				break;
			}

			overall_err = esp_ota_set_boot_partition(target);
			if (overall_err != ESP_OK) {
				ESP_LOGE(TAG, "esp_ota_set_boot_partition failed: %s", esp_err_to_name(overall_err));
				PROGRESS_ERR(esp_err_to_name(overall_err));
				break;
			}
			ESP_LOGI(TAG, "Firmware upgrade OK, boot partition set to %s", target->label);
			reboot_needed = true;

			/* ── Write HD image (data partition) ──────────────────────── */
		} else if (strcmp(part_name, "hd") == 0) {
			const esp_partition_t *hd_part = esp_partition_find_first((esp_partition_type_t)0x40, (esp_partition_subtype_t)0x02, "hd");
			if (hd_part == NULL) {
				ESP_LOGE(TAG, "HD partition not found");
				PROGRESS_ERR("HD partition not found");
				overall_err = ESP_FAIL;
				break;
			}
			if (data_size > hd_part->size) {
				ESP_LOGE(TAG, "HD image too large: %" PRIu32 " > %" PRIu32, data_size, hd_part->size);
				PROGRESS_ERR("HD image too large");
				overall_err = ESP_ERR_INVALID_SIZE;
				break;
			}

			PROGRESS(0, "Erasing HD...");
			ESP_LOGI(TAG, "Erasing HD partition (%" PRIu32 " bytes)...", hd_part->size);
			overall_err = esp_partition_erase_range(hd_part, 0, hd_part->size);
			if (overall_err != ESP_OK) {
				ESP_LOGE(TAG, "Erase HD failed: %s", esp_err_to_name(overall_err));
				PROGRESS_ERR("HD erase failed");
				break;
			}

			/* Stream HD image from SD card → HD flash partition */
			PROGRESS(0, "Writing HD image...");
			uint32_t remaining = data_size;
			uint32_t written = 0;
			int last_pct = -1;
			while (remaining > 0) {
				size_t chunk = (remaining < OTA_BUF_SIZE) ? remaining : OTA_BUF_SIZE;
				if (fread(buf, 1, chunk, fp) != chunk) {
					ESP_LOGE(TAG, "Read error at HD offset %" PRIu32, written);
					PROGRESS_ERR("SD card read error");
					overall_err = ESP_FAIL;
					break;
				}
				overall_err = esp_partition_write(hd_part, written, buf, chunk);
				if (overall_err != ESP_OK) {
					ESP_LOGE(TAG, "Write HD failed at offset %" PRIu32 ": %s", written, esp_err_to_name(overall_err));
					PROGRESS_ERR("HD write failed");
					break;
				}
				remaining -= chunk;
				written += chunk;
				int pct = (int)(written * 100 / data_size);
				if (pct != last_pct) {
					ESP_LOGI(TAG, "  hd: %d%%", pct);
					PROGRESS(pct, "Writing HD image...");
					last_pct = pct;
				}
			}
			if (overall_err != ESP_OK)
				break;

			ESP_LOGI(TAG, "HD image upgrade OK (%" PRIu32 " bytes written)", written);

		} else {
			ESP_LOGW(TAG, "Unknown partition target '%s' — skipping", part_name);
		}
	}

	free(buf);
	free(entries);
	fclose(fp);

	/* ── Cleanup ──────────────────────────────────────────────────── */
	if (overall_err == ESP_OK) {
		/* Delete the upgrade file so it doesn't re-run on next boot */
		if (remove(upgrade_path) == 0) {
			ESP_LOGI(TAG, "Deleted %s", upgrade_path);
		} else {
			ESP_LOGW(TAG, "Could not delete %s (will re-check on next boot)", upgrade_path);
		}

		if (reboot_needed) {
			ESP_LOGI(TAG, "Upgrade complete. Rebooting in 1 second...");
			vTaskDelay(pdMS_TO_TICKS(1000));
			esp_restart();
		}
	} else {
		ESP_LOGE(TAG, "Upgrade failed: %s", esp_err_to_name(overall_err));
		/* Leave the file on SD for debugging / retry */
	}

	return overall_err;
}

esp_err_t upgrade_check_and_run(const char *upgrade_path)
{
	return upgrade_do_run(upgrade_path, NULL);
}

esp_err_t upgrade_run_with_progress(const char *upgrade_path, upgrade_progress_fn progress_cb)
{
	return upgrade_do_run(upgrade_path, progress_cb);
}
