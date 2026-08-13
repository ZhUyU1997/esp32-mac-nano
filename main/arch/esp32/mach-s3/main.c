#include <stdio.h>
#include <stddef.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "fast_attr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "framebuffer.h"
#include "storage_platform.h"
#include "macplus.h"
#include "block/block.h"
#include "probe.h"
#include "video/frame_blit.h"
#include "sound.h"
#include "input.h"
#include "gpio/gpio.h"
#include "driver/gpio.h"
#include "blit_worker.h"
#include "task_stats.h"
#include "ui.h"
#include "settings_persist.h"
#include "machine_backend.h"
#include "msg.h"
#include "sensors/sensor_shm.h"
#include "sensors/sensors.h"
#include "settings_ui.h"
#include "driver/ble/input-ble-hid.h"
#include "web-control.h"

#include "upgrade_sdcard.h"
#include "upgrade_ui.h"

#include "flash_mode.h"
#include "flash_mode_ui.h"

static const char *TAG = "mach-s3";

/* VTERM_MODE: replace the Mac pause menu (MODE_UI) with the XT-era
 * terminal emulator (telnet backend). 1 = enabled, 0 = original pause UI.
 * The vterm sources are compiled unconditionally; this only gates the
 * main-loop integration so a disabled build keeps the normal flow. */
#define VTERM_MODE 1

/* VTERM_BOOT: boot straight into vterm (debug convenience — a reset re-runs
 * the telnet test without pressing F12). Only meaningful with VTERM_MODE=1. */
#define VTERM_BOOT 0

/* Message internal copies go to PSRAM (internal RAM is scarce). */
static void *msg_alloc_psram(size_t n)
{
	return heap_caps_malloc(n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}
static void msg_free_psram(void *p)
{
	heap_caps_free(p);
}

static const uint32_t k_task_stack_words = 3 * 1024;
static const UBaseType_t k_sound_task_priority = 5;
static const UBaseType_t k_emu_task_priority = 5;

/* Pre-allocated in app_main before BT init; used by load_boot_config.
 * Mac's last 64KB RAM block (video + sound buffers) must live in internal DRAM. */
static uint8_t *s_mac_ram_last_block = NULL;

extern const uint8_t _binary_rom_bin_start[];
extern const uint8_t _binary_pcex_mmio_rom_start[];
extern const uint8_t _binary_pcex_mmio_rom_end[];
extern const uint8_t _binary_mach_s3_json_start[];
extern const uint8_t _binary_mach_s3_json_end[];

#define macplus_rom ((uint8_t *)_binary_rom_bin_start)
#define macplus_romex _binary_pcex_mmio_rom_start
#define macplus_romex_size ((size_t)(_binary_pcex_mmio_rom_end - _binary_pcex_mmio_rom_start))

typedef struct {
	mach_s3_blit_worker_t *worker;
	const uint8_t *latest_frame;
	macplus_t *mac;
	int fps;
	int blit_skipped;
} mach_s3_frame_source_t;

static inline int FAST_FUNC_ATTR fps_tick(void)
{
	static int val;
	static int cnt;
	static int64_t t0;
	cnt++;
	if (cnt >= 60) {
		int64_t now = esp_timer_get_time();
		val = (int)(cnt * 1000000LL / (now - t0));
		cnt = 0;
		t0 = now;
	}
	return val;
}

static void FAST_FUNC_ATTR mach_s3_blit_mac_cb(framebuffer_t *lcd, void *user_ctx)
{
	mach_s3_frame_source_t *s = (mach_s3_frame_source_t *)user_ctx;
	assert(s != NULL);
	assert(s->worker != NULL);
	assert(lcd != NULL);
	assert(s->latest_frame != NULL);
	assert(s->mac != NULL);

	int is_alt = (s->latest_frame == s->mac->vbuf2);
	if (is_alt) {
		/* vbuf2 (0x3ECD00) crosses a 64KB memmap block boundary and has no
		 * contiguous host pointer; skip rendering and keep the last frame.
		 * Keep the dirty flag so vbuf1 redraws immediately when selected. */
		s->blit_skipped++;
	} else if (VBUF_IS_DIRTY(s->mac)) {
		VBUF_CLEAR_DIRTY(s->mac);
		void *fb = framebuffer_get_framebuffer(lcd);
		assert(fb != NULL);
		blit_mac_mono_to_lcd_rgba(fb, s->latest_frame, lcd->height, lcd->width);
	} else {
		s->blit_skipped++;
	}
	s->fps = fps_tick();
}

static void FAST_FUNC_ATTR on_mac_frame(uint8_t *mem, void *ctx)
{
	mach_s3_frame_source_t *s = (mach_s3_frame_source_t *)ctx;
	assert(s != NULL);
	assert(s->worker != NULL);
	s->latest_frame = mem;
	(void)mach_s3_blit_worker_submit_async(s->worker, mach_s3_blit_mac_cb, s);
}

static void on_mac_floppy_eject(unsigned drive, void *ctx)
{
	(void)ctx;
	if (drive == 1u) {
		(void)mach_s3_settings_persist_clear_floppy_path();
	}
}

/* PRAM byte 0x08 bit layout (Inside Macintosh).
 * bits 7-4: mouse scaling  0=slowest, 7=fastest, factory=1
 * bit  3:   unused
 * bits 2-0: speaker volume 0=loudest, 7=silent, factory=0 */
#define PRAM_MSK_MOUSE  0xf0u
#define PRAM_MSK_VOL    0x07u

/* PRAM persist: restore volume byte from NVS (mouse speed bits untouched). */
static void pram_persist_load(macplus_t *s)
{
	uint8_t vol;
	if (mach_s3_settings_persist_get_pram_vol(&vol)) {
		uint8_t b = mac_rtc_get_byte(&s->rtc, 0x08);
		mac_rtc_set_byte(&s->rtc, 0x08, (b & PRAM_MSK_MOUSE) | (vol & PRAM_MSK_VOL));
	}
}

/* PRAM persist: every 1s, save volume nibble to NVS if changed. */
static void pram_persist_sync(const macplus_t *s)
{
	static uint8_t last_vol;
	static bool    init;

	uint8_t vol = mac_rtc_get_byte(&s->rtc, 0x08) & PRAM_MSK_VOL;
	if (!init) {
		last_vol = vol;
		init = true;
	} else if (vol != last_vol) {
		last_vol = vol;
		mach_s3_settings_persist_set_pram_vol(vol);
	}
}

typedef struct {
	macplus_t *mac;
	mach_s3_frame_source_t *frame_source;
} monitor_ctx_t;

static void monitor_task(void *arg)
{
	monitor_ctx_t *ctx = (monitor_ctx_t *)arg;

	while (true) {
		vTaskDelay(pdMS_TO_TICKS(1000));
		int snd_hz = ctx->mac->sound.out ? ctx->mac->sound.out->snd_hz : 0;
		int cpu_hz = mac_clock_cpu_hz(ctx->mac);
		int fps = ctx->frame_source->fps;
		int skipped = ctx->frame_source->blit_skipped;
		ctx->frame_source->blit_skipped = 0;
		int dram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
		int psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
		int dma = heap_caps_get_free_size(MALLOC_CAP_DMA);
		int dma_max = heap_caps_get_largest_free_block(MALLOC_CAP_DMA);
		ESP_LOGI("mon", "dram=%d ps=%d dma=%d/%d cpu=%dHz fps=%d skip=%d snd=%dHz", dram, psram, dma, dma_max, cpu_hz, fps, skipped, snd_hz);
		{
			static int tick;
			if (++tick % 10 == 0)
				task_stats_print();
		}
		pram_persist_sync(ctx->mac);
	}
}

void usb_hid_main(void);
void macplus_task(void *pvParameters);

/* arg: sound_t * (same instance as macplus_config_t.sound). */
static void sound_output_task(void *arg)
{
	sound_t *d = (sound_t *)arg;

	assert(d != NULL);
	assert(d->output_task != NULL);
	d->output_task(d);
}

static macplus_config_t load_boot_config(mach_s3_frame_source_t *frame_source)
{
	sound_t *snd = sound_lookup("snd");
	assert(snd != NULL);
	uint8_t *mac_ram_ext = heap_caps_malloc(MACPLUS_RAMSIZE - MEMMAP_ES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
	uint8_t *mac_ram_last_block = s_mac_ram_last_block;
	assert(mac_ram_ext != NULL);
	assert(mac_ram_last_block != NULL);

	/* config.fd stays NULL: no pre-created floppy block; the boot-restored
	 * floppy is opened by the core via mac_floppy_insert after boot. */
	macplus_config_t config = {
	        .rom = macplus_rom,
	        .romex = macplus_romex,
	        .romex_size = macplus_romex_size,
	        .sound = snd,
	        .hd = {
	                [6] = block_lookup("hd"),        /* SCSI ID 6: primary disk */
	                [0] = block_lookup("hd0-img"),   /* SCSI ID 0 */
	                [1] = block_lookup("hd1-img"),   /* SCSI ID 1 */
	                [2] = block_lookup("hd2-img"),   /* SCSI ID 2 */
	                [3] = block_lookup("hd3-img"),   /* SCSI ID 3 */
	                [4] = block_lookup("hd4-img"),   /* SCSI ID 4 */
	                [5] = block_lookup("hd5-img"),   /* SCSI ID 5 */
	                [7] = block_lookup("hd7-img"),   /* SCSI ID 7 */
	        },
	        .fd = NULL,
	        .frame_callback = on_mac_frame,
	        .frame_callback_ctx = frame_source,
	        .floppy_eject_callback = on_mac_floppy_eject,
	        .floppy_eject_callback_ctx = NULL,
	};
	for (unsigned int i = 0; i < (MACPLUS_RAMSIZE / MEMMAP_ES) - 1; i++) {
		config.ram[i] = (unsigned char *)&mac_ram_ext[i * MEMMAP_ES];
	}
	config.ram[(MACPLUS_RAMSIZE / MEMMAP_ES) - 1] = (unsigned char *)mac_ram_last_block;

	memset(mac_ram_last_block, 0xff, MEMMAP_ES);
	return config;
}

static void bl_adjust(void *backend, int delta)
{
	machine_backend_t *b = (machine_backend_t *)backend;
	uint8_t v = b->get_backlight ? b->get_backlight(b) : 75u;
	if (delta < 0) {
		const uint8_t d = (uint8_t)(-delta);
		v = (v >= d) ? v - d : 0;
	} else {
		const uint8_t d = (uint8_t)delta;
		v = (v <= 100u - d) ? v + d : 100u;
	}
	if (b->set_backlight) b->set_backlight(b, v);
	if (b->commit_backlight) b->commit_backlight(b, v);
}

/* Volume step via the machine backend (0..25, same range as the menu slider). */
static void vol_adjust(void *backend, int delta)
{
	machine_backend_t *b = (machine_backend_t *)backend;
	uint8_t v = b->get_volume ? b->get_volume(b) : 0u;
	if (delta < -100) {
		v = 0u; /* mute */
	} else if (delta < 0) {
		v = (v >= 1u) ? v - 1u : 0u;
	} else {
		v = (v <= 24u) ? v + 1u : 25u;
	}
	if (b->set_volume) b->set_volume(b, v);
}

void macplus_task(void *args)
{
	framebuffer_t *lcd = framebuffer_lookup("lcd");
	mach_s3_blit_worker_t *blit_worker = mach_s3_blit_worker_create(lcd);
	mach_s3_frame_source_t *frame_source = (mach_s3_frame_source_t *)calloc(1, sizeof(mach_s3_frame_source_t));
	assert(lcd != NULL);
	assert(blit_worker != NULL);
	assert(frame_source != NULL);
	frame_source->worker = blit_worker;

	ui_t *ui = ui_new((ui_config_t){
	        .blit_worker = blit_worker,
	        .lcd = lcd,
	});

	/* Flash-mode boot: hold a full-screen hint and skip the emulator.
	 * Exit paths: browser flash (esptool hard_reset reboots), or power
	 * cycle (the one-shot flag is already cleared → normal boot). */
	if (mach_s3_flash_mode_active()) {
		flash_mode_ui_show();
		while (true) {
			ui_run_frame(ui);
		}
	}

	/* MODE_UI: pause menu normally; with VTERM_MODE=1 it hosts the vterm
	 * terminal (enter from the Mac pause state, F10/F12 returns to the Mac). */
	/* Check upgrade BEFORE heavy MacPlus init (mac_get_instance allocates PSRAM) */
	if (upgrade_file_exists("/sdcard/upgrade.bin")) {
		upgrade_ui_show();
		esp_err_t upg_ret = upgrade_run_with_progress("/sdcard/upgrade.bin", upgrade_ui_update);
		if (upg_ret != ESP_OK) {
			ESP_LOGE(TAG, "Upgrade failed: %s", esp_err_to_name(upg_ret));
			/* Error already on screen via progress callback.
			 * Stay here until user power-cycles. */
			while (true) {
				ui_run_frame(ui);
			}
		}
		/* On success, upgrade_run_with_progress reboots before returning */
	}

	macplus_config_t boot_config = load_boot_config(frame_source);
	macplus_t *s = mac_get_instance(boot_config);
	frame_source->mac = s;
	/* Boot-restored floppy: the core opens and inserts it (single owner,
	 * no pre-created block — no fd leak). */
	{
		char restored_path[FLOPPY_MAX_PATH];
		if (mach_s3_settings_persist_get_restored_floppy_path(restored_path, sizeof(restored_path))) {
			if (!mac_floppy_insert(s, restored_path, true, false))
				ESP_LOGE(TAG, "restore floppy failed: %s", restored_path);
			else
				ESP_LOGI(TAG, "restore floppy inserted: %s", restored_path);
		} else {
			ESP_LOGI(TAG, "no restored floppy path");
		}
	}
	sensor_shm_init(s);
	/* Message dispatch context + PSRAM allocator for internal copies. */
	mac_msg_init(s, msg_alloc_psram, msg_free_psram);
	pram_persist_load(s);

	xTaskCreatePinnedToCore(&sound_output_task, "snd", k_task_stack_words, s->sound.out, k_sound_task_priority, NULL, 0);

	monitor_ctx_t *mon_ctx = calloc(1, sizeof(*mon_ctx));
	assert(mon_ctx != NULL);
	mon_ctx->mac = s;
	mon_ctx->frame_source = frame_source;
	xTaskCreatePinnedToCore(&monitor_task, "mon", 4096, mon_ctx, 1, NULL, 0);

	machine_backend_t *fb = machine_backend_create(s, lcd);
	assert(fb != NULL);

	/* Register floppy/sound/backlight backend with settings UI */
	mach_s3_settings_ui_bind_backend(fb);

	s->backlight_adjust = bl_adjust;
	s->backlight_adjust_ctx = fb;
	s->volume_adjust = vol_adjust;
	s->volume_adjust_ctx = fb;

	typedef enum {
		MODE_MAC = 0,
		MODE_UI,
	} mach_s3_mode_t;
#if VTERM_MODE && VTERM_BOOT
	mach_s3_mode_t mode = MODE_UI; /* debug: boot straight into vterm */
#else
	mach_s3_mode_t mode = MODE_MAC;
#endif

	/* WiFi remote: default ON (after the upgrade flow). Auto-off after
	 * 60s without activity — the page/pause-menu can re-enable it. */
	web_control_enable();

	/* M3: telnet server for the vterm mode (port 23) */
#if VTERM_MODE
	extern void vterm_telnet_start(void);
	vterm_telnet_start();
#endif

	while (true) {
		if (mode == MODE_MAC) {
			macplus_run_frame(s);
			if (mac_get_pause(s)) {
				uint16_t mac_x, mac_y;
				mac_get_mouse_pos(s, &mac_x, &mac_y);
				ui_set_mouse_pos(ui, (int32_t)mac_x, (int32_t)mac_y);
				ui_pause_enter(ui);
				mode = MODE_UI;
			}
		} else {
#if VTERM_MODE
			/* MODE_UI temporarily replaced by vterm (M3 prep) */
			extern bool vterm_esp32_enter(framebuffer_t *lcd, mach_s3_blit_worker_t *blit_worker);
			if (vterm_esp32_enter(lcd, blit_worker)) {
				mode = MODE_MAC;
				mac_set_pause(s, 0);
				VBUF_MARK_DIRTY(s);
			}
#else
			ui_run_frame(ui);
			if (ui_pause_take_exit_request(ui)) {
				int32_t lx, ly;
				ui_get_mouse_pos(ui, &lx, &ly);
				ui_pause_leave(ui);
				s->abs_mouse_ready = 1;
				mac_set_mouse_abs(s, (uint16_t)lx, (uint16_t)ly, 0);
				mac_set_pause(s, 0);
				VBUF_MARK_DIRTY(s);
				mode = MODE_MAC;
			}
#endif
		}
	}
}

/* Sync ESP32 system time from DS3231 once at boot. */
static void sync_system_time_from_ds3231(void)
{
	struct tm t;
	if (sensors_get_time(&t) == ESP_OK) {
		struct timeval tv = { .tv_sec = mktime(&t), .tv_usec = 0 };
		settimeofday(&tv, NULL);
	}
}

void app_main(void)
{
	/* Flash mode check runs first: physical key forced entry (GPIO, no
	 * dependency) + menu one-shot NVS flag. Early enough that USB Host
	 * init below can be skipped. */
	const bool flash_mode_boot = mach_s3_flash_mode_check();

	/* OTA rollback confirmation — must come before any critical init */
	upgrade_mark_app_valid();

	(void)platform_storage_mount_sdcard();

	input_init();

	(void)probe_device((const char *)_binary_mach_s3_json_start, (size_t)(_binary_mach_s3_json_end - _binary_mach_s3_json_start));

	sensors_init();
	sync_system_time_from_ds3231();

	/* Pre-allocate Mac's last 64KB RAM block in internal DRAM
	 * before BT init consumes that memory pool. */
	s_mac_ram_last_block = heap_caps_malloc(MEMMAP_ES, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
	assert(s_mac_ram_last_block != NULL);

	xTaskCreatePinnedToCore(&macplus_task, "macplus", 8 * 1024, NULL, k_emu_task_priority, NULL, 1);

	// ble_hid_host_init();
	if (!flash_mode_boot) {
		usb_hid_main();
	}
}
