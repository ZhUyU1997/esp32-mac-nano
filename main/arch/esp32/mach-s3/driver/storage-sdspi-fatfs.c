#include "storage_platform.h"

#include <stdio.h>
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "driver/spi_common.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"
#include "ramfs.h"

static const char *TAG = "storage_platform";

#define SD_MOUNT_POINT "/sdcard"
#ifndef SD_SPI_HOST
#define SD_SPI_HOST SPI2_HOST
#endif
#ifndef SD_PIN_NUM_MISO
#define SD_PIN_NUM_MISO 17
#endif
#ifndef SD_PIN_NUM_MOSI
#define SD_PIN_NUM_MOSI 8
#endif
#ifndef SD_PIN_NUM_CLK
#define SD_PIN_NUM_CLK 18
#endif
#ifndef SD_PIN_NUM_CS
#define SD_PIN_NUM_CS 3
#endif

esp_err_t platform_storage_mount_sdcard(void)
{
	esp_err_t ret;
	sdmmc_card_t *card = NULL;
	static bool mounted = false;

	if (mounted) {
		return ESP_OK;
	}

	sdmmc_host_t host = SDSPI_HOST_DEFAULT();
	host.slot = SD_SPI_HOST;

	spi_bus_config_t bus_cfg = {
	        .mosi_io_num = SD_PIN_NUM_MOSI,
	        .miso_io_num = SD_PIN_NUM_MISO,
	        .sclk_io_num = SD_PIN_NUM_CLK,
	        .quadwp_io_num = -1,
	        .quadhd_io_num = -1,
	        .max_transfer_sz = 4096,
	};
	ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CH_AUTO);
	if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
		ESP_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(ret));
		return ret;
	}

	sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
	slot_config.gpio_cs = SD_PIN_NUM_CS;
	slot_config.host_id = host.slot;

	esp_vfs_fat_sdmmc_mount_config_t mount_config = {
	        .format_if_mount_failed = false,
	        .max_files = 8,
	        .allocation_unit_size = 16 * 1024,
	};

	ret = esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &host, &slot_config, &mount_config, &card);
	if (ret != ESP_OK) {
		/* No SD card: /sdcard stays unmounted. Register an independent
		 * RAMFS at /ram for volatile temp storage (separate mount point). */
		ESP_LOGW(TAG, "SD mount failed: %s", esp_err_to_name(ret));
		const ramfs_config_t rcfg = {
		        .base_path = "/ram",
		        .max_files = 8,
		        .max_bytes = 1024 * 1024,
		        .caps = MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT,
		};
		if (ramfs_register(&rcfg) != ESP_OK) {
			ESP_LOGE(TAG, "ramfs register failed");
			return ESP_FAIL;
		}
		ESP_LOGI(TAG, "RAMFS mounted at /ram (no SD card)");
		return ESP_OK;
	}
	mounted = true;
	sdmmc_card_print_info(stdout, card);
	ESP_LOGI(TAG, "SD mounted at %s", SD_MOUNT_POINT);
	return ESP_OK;
}
