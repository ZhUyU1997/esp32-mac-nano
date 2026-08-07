/*
 * Mac Plus sound emulation (PCE arch/macplus/sound.c). Feeds sound_t only.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "snd.h"

/* Mini vMac SNDEMDEV.c */
static const uint16_t subtick_offset[MAC_SOUND_NUM_SUBTICKS] = {0, 25, 50, 90, 102, 115, 138, 161, 185, 208, 231, 254, 277, 300, 323, 346};

static const uint8_t subtick_n[MAC_SOUND_NUM_SUBTICKS] = {25, 25, 40, 12, 13, 23, 23, 24, 23, 23, 23, 23, 23, 23, 23, 24};

static const uint16_t vol_mult[7] = {8192, 9362, 10922, 13107, 16384, 21845, 32768};

static const uint16_t vol_offset[8] = {28672, 28087, 27307, 26215, 24576, 21846, 16384, 0};

static uint32_t sound_invert_phase;
static uint32_t sound_invert_state;

static unsigned mac_sound_invert_time_from_via(e6522_t *via)
{
	unsigned long sh = (unsigned long)via->addr_shift;
	unsigned acr = e6522_get_uint8(via, 0x0bUL << sh);

	if ((acr & 0xC0) != 0xC0) {
		return 0;
	}
	return (unsigned)e6522_get_uint8(via, 0x06UL << sh) | ((unsigned)e6522_get_uint8(via, 0x07UL << sh) << 8);
}

static uint16_t mac_sound_apply_volume(uint16_t samp, unsigned vol)
{
	if (vol >= 7u) {
		return samp;
	}
	return (uint16_t)(((uint32_t)samp * (uint32_t)vol_mult[vol]) >> 16) + vol_offset[vol];
}

static uint16_t mac_sound_apply_master_volume(uint16_t samp, unsigned master_vol)
{
	int32_t d = (int32_t)samp - 32768;
	d = (d * (int32_t)master_vol) / 100;
	return (uint16_t)(32768 + d);
}

static void mac_sound_do_subtick(mac_sound_t *ms, int subtick, e6522_t *via)
{
	unsigned n;
	unsigned start;
	unsigned invert_time;
	unsigned vol;
	unsigned i;
	uint16_t *p;

	if (ms->sbuf == NULL) {
		return;
	}
	if (ms->cnt >= (unsigned)MAC_SOUND_SAMPLES_PER_FRAME) {
		return;
	}

	start = subtick_offset[subtick];
	n = subtick_n[subtick];
	invert_time = mac_sound_invert_time_from_via(via);
	vol = ms->volume & 7u;

	if (ms->cnt + n > (unsigned)MAC_SOUND_SAMPLES_PER_FRAME) {
		n = (unsigned)MAC_SOUND_SAMPLES_PER_FRAME - ms->cnt;
	}

	p = &ms->buf[ms->cnt];

	if (!ms->enable && invert_time == 0) {
		for (i = 0; i < n; i++) {
			*p++ = 0x8000u;
			if ((uint16_t)0x8000u != ms->last_val) {
				ms->last_val = 0x8000u;
				ms->changed = 1;
			}
		}
		ms->cnt += n;
		return;
	}

	for (i = 0; i < n; i++) {
		unsigned off = start + i;
		uint32_t val = (uint32_t)ms->sbuf[2u * off];
		val = (val << 8) & 0xffffu;

		*p = (uint16_t)val;
		p++;
	}

	p -= n;

	if (invert_time != 0) {
		uint32_t phase_incr = (uint32_t)invert_time * 20u;

		for (i = 0; i < n; i++) {
			if (sound_invert_phase < 704u) {
				uint32_t on_portion = 0;
				uint32_t last_phase = 0;

				do {
					if (!sound_invert_state) {
						on_portion += (sound_invert_phase - last_phase);
					}
					sound_invert_state ^= 1u;
					last_phase = sound_invert_phase;
					sound_invert_phase += phase_incr;
				} while (sound_invert_phase < 704u);

				if (!sound_invert_state) {
					on_portion += 704u - last_phase;
				}
				*p = (uint16_t)(((uint32_t)*p * on_portion) / 704u);
			} else {
				if (sound_invert_state) {
					*p = 0;
				}
			}
			sound_invert_phase -= 704u;
			p++;
		}
		p -= n;
	}

	if (vol < 7u) {
		for (i = 0; i < n; i++) {
			p[i] = mac_sound_apply_volume(p[i], vol);
		}
	}
	if (ms->master_volume < 100u) {
		for (i = 0; i < n; i++) {
			p[i] = mac_sound_apply_master_volume(p[i], ms->master_volume);
		}
	}

	for (i = 0; i < n; i++) {
		if (p[i] != ms->last_val) {
			ms->last_val = p[i];
			ms->changed = 1;
		}
	}

	ms->cnt += n;
}

void mac_sound_begin_frame(mac_sound_t *ms)
{
	ms->cnt = 0;
	ms->subtick_next = 0;
	ms->clk = 0;
	sound_invert_phase = 0;
	sound_invert_state = 0;
}

void mac_sound_advance_subticks(mac_sound_t *ms, e6522_t *via, unsigned cycles_total)
{
	while (ms->subtick_next < 15u && cycles_total >= MAC_SOUND_CYCLES_PER_SUBTICK * (ms->subtick_next + 1u)) {
		mac_sound_do_subtick(ms, (int)ms->subtick_next, via);
		ms->subtick_next++;
	}
}

static void mac_sound_fill(mac_sound_t *ms, unsigned cnt)
{
	if ((cnt == 0) || (ms->cnt >= (unsigned)MAC_SOUND_SAMPLES_PER_FRAME)) {
		return;
	}

	if (ms->last_val != 0x8000u) {
		ms->changed = 1;
	}

	while ((cnt > 0) && (ms->cnt < (unsigned)MAC_SOUND_SAMPLES_PER_FRAME)) {
		ms->buf[ms->cnt] = 0x8000u;
		ms->cnt += 1;
		cnt -= 1;
	}

	ms->last_val = 0x8000u;
}

void mac_sound_set_sbuf(mac_sound_t *ms, const uint8_t *sbuf)
{
	ms->sbuf = sbuf;
}

void mac_sound_set_lowpass(mac_sound_t *ms, unsigned freq)
{
	ms->lowpass_freq = freq;
}

void mac_sound_set_volume(mac_sound_t *ms, unsigned vol)
{
	ms->volume = vol & 7u;
}

void mac_sound_set_master_volume(mac_sound_t *ms, unsigned vol)
{
	if (vol > 100u) {
		vol = 100u;
	}
	ms->master_volume = vol;
}

void mac_sound_set_enable(mac_sound_t *ms, int on)
{
	ms->enable = on ? 1 : 0;
}

void mac_sound_vbl(mac_sound_t *ms, e6522_t *via)
{
	if (ms->sbuf == NULL) {
		return;
	}
	if (ms->out == NULL || ms->out->write == NULL) {
		return;
	}

	while (ms->subtick_next < 16u) {
		mac_sound_do_subtick(ms, (int)ms->subtick_next, via);
		ms->subtick_next++;
	}

	if (ms->cnt < (unsigned)MAC_SOUND_SAMPLES_PER_FRAME) {
		if (!ms->enable && ((ms->silence_cnt < MAC_SOUND_SILENCE) || (ms->cnt > 0))) {
			mac_sound_fill(ms, (unsigned)MAC_SOUND_SAMPLES_PER_FRAME - ms->cnt);
		}
	}

	if (ms->changed) {
		if (ms->silence_cnt >= MAC_SOUND_SILENCE) {
			/* PCE mac_sound_speaker_on — N/A on I2S */
		}
		ms->silence_cnt = 0;
	} else {
		if (ms->silence_cnt < MAC_SOUND_SILENCE) {
			ms->silence_cnt += 1;
		}
	}

	if (ms->silence_cnt < MAC_SOUND_SILENCE) {
		if (ms->lowpass_freq > 0) {
			/* PCE snd_iir2_filter — optional */
		}
		if (ms->cnt > 0) {
			sound_write(ms->out, ms->buf, (unsigned)ms->cnt);
		}
	}

	ms->changed = 0;
	ms->idx = MAC_SOUND_IDX_START;
	ms->cnt = 0;
	ms->clk = 0;
	ms->subtick_next = 0;
}

void mac_sound_clock(mac_sound_t *ms, unsigned long n)
{
	(void)ms;
	(void)n;
}

int mac_sound_init(mac_sound_t *ms, sound_t *out)
{
	if (ms == NULL || out == NULL) {
		return -1;
	}
	memset(ms, 0, sizeof(*ms));
	ms->out = out;
	ms->last_val = 0x8000;
	ms->silence_cnt = MAC_SOUND_SILENCE;
	ms->lowpass_freq = 8000;
	ms->master_volume = 100u;
	return 0;
}

void mac_sound_free(mac_sound_t *ms)
{
	if (ms == NULL) {
		return;
	}
	sound_close(ms->out);
	ms->out = NULL;
}

mac_sound_t *mac_sound_new(sound_t *out)
{
	mac_sound_t *ms = calloc(1, sizeof(mac_sound_t));

	if (ms == NULL) {
		return NULL;
	}
	if (mac_sound_init(ms, out) != 0) {
		free(ms);
		return NULL;
	}
	return ms;
}

void mac_sound_del(mac_sound_t *ms)
{
	if (ms == NULL) {
		return;
	}
	mac_sound_free(ms);
	free(ms);
}
