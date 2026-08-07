#ifndef MACPLUS_H
#define MACPLUS_H

#include <stddef.h>
#include <stdint.h>

#include "memmap.h"

#include "block/block.h"
#include "e6522.h"
#include "e8530.h"
#include "iwm.h"
#include "keyboard.h"
#include "rtc.h"
#include "scsi.h"
#include "snd.h"
#include "sony.h"
#include "sound.h"

typedef struct {
	void *rom;
	const uint8_t *romex;
	size_t romex_size;
	sound_t *sound;
	block_t *hd[8];
	block_t *fd;
	unsigned char *ram[MACPLUS_RAMSIZE / MEMMAP_ES];
	void (*frame_callback)(uint8_t *framebuffer, void *ctx);
	void *frame_callback_ctx;
	void (*floppy_eject_callback)(unsigned drive, void *ctx);
	void *floppy_eject_callback_ctx;
} macplus_config_t;

typedef struct {
	int ca1;
	int ca2;
	int frame;
	int cycles_per_sec;
	int cpu_hz;
	uint64_t frame_group_start_us;
} mac_clock_sched_t;

/**
 * @brief MMIO hook callback — called when 68k writes byte to 0xEFFD00.
 */
struct macplus;
typedef void (*mac_mmio_hook_fn_t)(struct macplus *sim, unsigned int value);

typedef struct macplus {
	e6522_t via;
	e8530_t scc;
	unsigned char irq_bits;
	unsigned char intr_scsi_via;
	unsigned char via_port_a;
	unsigned char via_port_b;
	unsigned char dcd_a;
	unsigned char dcd_b;
	long mouse_delta_x;
	long mouse_delta_y;
	unsigned mouse_button;
	mac_kbd_t *kbd;
	mac_rtc_t rtc;
	mac_sound_t sound;
	mac_scsi_t scsi;
	mac_iwm_t iwm;
	mac_sony_t sony;

	unsigned speed_factor;
	unsigned long clk_div[4];
	unsigned long scc_clk_phase;
	int overlay;
	uint8_t *rom;
	const uint8_t *romex;
	size_t romex_size;
	unsigned char *ram[MACPLUS_RAMSIZE / MEMMAP_ES];

	/* Core-owned floppy block (PCE dsks style): single owner of the
	 * inserted disk; replaced inserts free the old one (no fd leaks). */
	block_t *floppy_block;

	uint8_t *vbuf1;
	uint8_t *vbuf2;
	uint8_t vbuf_dirty;
	uint8_t *sbuf1;
	uint8_t *sbuf2;
	void (*frame_callback)(uint8_t *framebuffer, void *ctx);
	void *frame_callback_ctx;
	mac_clock_sched_t clock_sched;
	int booted;
	int abs_mouse_ready;
	int paused;

	/* Backlight step callback for GPIO A/C keys */
	void (*backlight_adjust)(void *backend, int delta);
	void *backlight_adjust_ctx;

	/* Volume step callback for volume keys (delta<-100 = mute) */
	void (*volume_adjust)(void *backend, int delta);
	void *volume_adjust_ctx;

	/* Generic MMIO hook table (register via mac_register_hook) */
	struct {
		uint8_t        value;
		mac_mmio_hook_fn_t handler;
	} mmio_hooks[8];
	int mmio_hook_count;

	/* Shared memory region at 0xF00000 (accessible from Mac) */
	uint8_t shm_region[512];
} macplus_t;

// atomic variants (zero-overhead on Xtensa, use if cross-core visibility issues suspected):
#define VBUF_MARK_DIRTY(s)  __atomic_store_n(&(s)->vbuf_dirty, 1, __ATOMIC_RELAXED)
#define VBUF_CLEAR_DIRTY(s) __atomic_store_n(&(s)->vbuf_dirty, 0, __ATOMIC_RELAXED)
#define VBUF_IS_DIRTY(s)    __atomic_load_n(&(s)->vbuf_dirty, __ATOMIC_RELAXED)

macplus_t *macplus_instance(void);

/**
 * @brief Register a handler for a given MMIO hook value.
 */
void mac_register_hook(macplus_t *sim, uint8_t value, mac_mmio_hook_fn_t handler);

void mac_init(macplus_t *sim);
macplus_t *mac_new(void);
macplus_t *mac_get_instance(macplus_config_t config);
/* Returns the created instance (never boots); safe from other threads
 * for reading volatile state such as DISKINPLACE. */
macplus_t *macplus_instance(void);
void mac_free(macplus_t *sim);
void macplus_boot(macplus_t *sim, const macplus_config_t *config);
void macplus_run_frame(macplus_t *sim);

void mac_set_pause(macplus_t *s, int paused);
int mac_get_pause(const macplus_t *s);

void mac_set_overlay(macplus_t *s, int overlay);
void mac_set_vbuf(macplus_t *s, uint8_t *vbuf);

void mac_interrupt_scsi(void *ext, unsigned char val);
void mac_interrupt_via(void *ext, unsigned char val);
void mac_set_via_port_a(void *ext, unsigned char val);
void mac_interrupt_osi(void *ext, unsigned char val);
void mac_set_rtc_data(void *ext, unsigned char v);
void mac_set_via_port_b(void *ext, unsigned char val);
void mac_interrupt_scc(void *ext, unsigned char val);
void mac_irq_reset(macplus_t *s);
void mac_reset(macplus_t *s);

void mac_emu_clock(macplus_t *s, unsigned n);
void mac_clock_sched_init(mac_clock_sched_t *sched);
void mac_clock_run_frame(macplus_t *s, mac_clock_sched_t *sched, unsigned cycles_per_frame, unsigned cpu_step);
int mac_clock_cpu_hz(const macplus_t *s);

void mac_set_mouse(macplus_t *s, int dx, int dy, unsigned but);
void mac_set_mouse_abs(macplus_t *s, uint16_t x, uint16_t y, unsigned but);
void mac_get_mouse_pos(const macplus_t *s, uint16_t *x, uint16_t *y);
void mac_set_key(macplus_t *sim, unsigned event, pce_key_t key);

/* Floppy lifecycle (PCE dsks style — the core owns the block):
 * - mac_floppy_insert: open the image, insert into drive 1, free the
 *   previously inserted block (restored or manual) — single owner,
 *   no fd leak.
 * - mac_floppy_get: current inserted block (for UI/status queries). */
int mac_floppy_insert(macplus_t *s, const char *path, bool delayed, bool ro);
block_t *mac_floppy_get(macplus_t *s);

#endif
