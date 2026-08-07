/*
 * Mac Plus sound — SubTick timing from Mini vMac SNDEMDEV.c; frame = 130240 CPU
 * cycles @ 1/60 s (see macplus.c). VIA: mac_sound_set_sbuf / volume / enable.
 *
 * Feeds samples through abstract sound_t; board code supplies the device.
 */

#ifndef MACPLUS_MAC_SOUND_H
#define MACPLUS_MAC_SOUND_H

#include <stdint.h>
#include <stdbool.h>

#include "e6522.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAC_SOUND_CLK 352
#define MAC_SOUND_SILENCE 600

#define MAC_SOUND_SAMPLES_PER_FRAME 370
#define MAC_SOUND_IDX_START 16

#define MAC_SOUND_NUM_SUBTICKS 16
#define MAC_SOUND_CYCLES_PER_FRAME 130240
#define MAC_SOUND_CYCLES_PER_SUBTICK (MAC_SOUND_CYCLES_PER_FRAME / MAC_SOUND_NUM_SUBTICKS)

typedef struct {
	sound_t *out;

	const uint8_t *sbuf;
	unsigned idx;
	unsigned cnt;
	uint16_t buf[MAC_SOUND_SAMPLES_PER_FRAME];
	unsigned long clk;

	unsigned subtick_next;

	int enable;
	unsigned volume;
	unsigned master_volume;

	unsigned long lowpass_freq;

	uint16_t last_val;
	int changed;
	unsigned silence_cnt;
} mac_sound_t;

/* Returns 0 on success, non-zero on failure (invalid args). */
int mac_sound_init(mac_sound_t *ms, sound_t *out);
mac_sound_t *mac_sound_new(sound_t *out);

void mac_sound_free(mac_sound_t *ms);
void mac_sound_del(mac_sound_t *ms);

void mac_sound_set_sbuf(mac_sound_t *ms, const uint8_t *sbuf);
void mac_sound_set_lowpass(mac_sound_t *ms, unsigned freq);
void mac_sound_set_volume(mac_sound_t *ms, unsigned vol);
void mac_sound_set_master_volume(mac_sound_t *ms, unsigned vol);
void mac_sound_set_enable(mac_sound_t *ms, int on);

void mac_sound_begin_frame(mac_sound_t *ms);
void mac_sound_advance_subticks(mac_sound_t *ms, e6522_t *via, unsigned cycles_total);

void mac_sound_vbl(mac_sound_t *ms, e6522_t *via);
void mac_sound_clock(mac_sound_t *ms, unsigned long cnt);

#ifdef __cplusplus
}
#endif

#endif
