#define _POSIX_C_SOURCE 200809L

#include "macplus.h"

#include <stdint.h>
#include <time.h>
#include <unistd.h>

#include "m68k.h"
#include "mac_hid_bridge.h"
#include "msg.h"

static uint64_t host_monotonic_us(void)
{
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
		return 0;
	}
	return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
}

/* Consumes mac_set_mouse() deltas -> VIA/SCC (cf. PCE mac_check_mouse). */
static void mac_check_mouse(macplus_t *s)
{
	unsigned char old_pb = s->via_port_b;

	if (s->mouse_button & 1u) {
		s->via_port_b &= 0xf7;
	} else {
		s->via_port_b |= 0x08;
	}
	if (s->via_port_b != old_pb) {
		e6522_set_irb_inp(&s->via, s->via_port_b);
	}

	if ((s->mouse_delta_x <= -2) || (s->mouse_delta_x >= 2)) {
		if (s->dcd_a) {
			s->via_port_b &= ~0x10;
		} else {
			s->via_port_b |= 0x10;
		}

		if (s->mouse_delta_x > 0) {
			s->via_port_b ^= 0x10;
			s->mouse_delta_x -= 2;
		} else {
			s->mouse_delta_x += 2;
		}
		e6522_set_irb_inp(&s->via, s->via_port_b);
		e8530_set_dcd_a(&s->scc, s->dcd_a);
		s->dcd_a = !s->dcd_a;
	}

	if ((s->mouse_delta_y <= -2) || (s->mouse_delta_y >= 2)) {
		if (s->dcd_b) {
			s->via_port_b &= ~0x20;
		} else {
			s->via_port_b |= 0x20;
		}

		if (s->mouse_delta_y > 0) {
			s->mouse_delta_y -= 2;
		} else {
			s->via_port_b ^= 0x20;
			s->mouse_delta_y += 2;
		}
		e6522_set_irb_inp(&s->via, s->via_port_b);
		e8530_set_dcd_b(&s->scc, s->dcd_b);
		s->dcd_b = !s->dcd_b;
	}
}

void mac_emu_clock(macplus_t *s, unsigned n)
{
	unsigned long viaclk, clkdiv;

	if (s->speed_factor == 0) {
		clkdiv = 1;
	} else {
		clkdiv = s->speed_factor;
	}

	{
		unsigned long sum = s->clk_div[0] + (unsigned long)n;

		s->clk_div[1] += sum / clkdiv;
		s->clk_div[0] = sum % clkdiv;
	}

	if (s->clk_div[1] < 10) {
		return;
	}

	viaclk = s->clk_div[1] / 10;

	e6522_clock(&s->via, viaclk);

	s->clk_div[1] -= 10 * viaclk;
	s->clk_div[2] += 10 * viaclk;

	if (s->clk_div[2] < 256) {
		return;
	}

	s->scc_clk_phase += 15UL * s->clk_div[2];
	e8530_clock(&s->scc, s->scc_clk_phase / 32);
	s->scc_clk_phase &= 31UL;

	mac_kbd_clock(s->kbd, s->clk_div[2]);

	s->clk_div[3] += s->clk_div[2];
	s->clk_div[2] = 0;

	if (s->clk_div[3] < 8192) {
		return;
	}

	mac_check_mouse(s);
	mac_rtc_clock(&s->rtc, s->clk_div[3]);
	s->clk_div[3] = 0;
}

static void record_cpu(mac_clock_sched_t *sched, int cycles_per_sec)
{
	static uint64_t start_us;
	uint64_t end_us = host_monotonic_us();

	if (start_us != 0)
		sched->cpu_hz = (int)((int64_t)cycles_per_sec * 1000000LL / (int64_t)(end_us - start_us));
	start_us = end_us;
}

void mac_clock_sched_init(mac_clock_sched_t *sched)
{
	sched->ca1 = 0;
	sched->ca2 = 0;
	sched->frame = 0;
	sched->cycles_per_sec = 0;
	sched->cpu_hz = 0;
	sched->frame_group_start_us = 0;
}

void mac_clock_run_frame(macplus_t *s, mac_clock_sched_t *sched, unsigned cycles_per_frame, unsigned cpu_step)
{
	int x;

	mac_sound_begin_frame(&s->sound);
	for (x = 0; x < (int)cycles_per_frame;) {
		unsigned step = (unsigned)((int)cycles_per_frame - x);
		if (step > cpu_step) {
			step = cpu_step;
		}
		m68k_execute((int)step);
		x += (int)step;
		mac_sound_advance_subticks(&s->sound, &s->via, (unsigned)x);
		mac_emu_clock(s, step);
		mac_msg_dispatch();
		macplus_input_poll(s);
	}

	sched->cycles_per_sec += x;

	if (s->frame_callback != NULL) {
		s->frame_callback((s->via_port_a & 0x40) ? s->vbuf1 : s->vbuf2, s->frame_callback_ctx);
	}
	sched->ca1 ^= 1;
	e6522_set_ca1_inp(&s->via, 0);
	e6522_set_ca1_inp(&s->via, 1);
	mac_sound_vbl(&s->sound, &s->via);

	sched->frame++;
	if (sched->frame == 59) {
		sched->ca2 ^= 1;
		e6522_set_ca2_inp(&s->via, sched->ca2);
	}
	if (sched->frame >= 60) {
		sched->ca2 ^= 1;
		e6522_set_ca2_inp(&s->via, sched->ca2);
		sched->frame = 0;
		record_cpu(sched, sched->cycles_per_sec);
		sched->cycles_per_sec = 0;
	}

	/* Per-frame throttling: target 1/60 s = ~16667 µs per frame.
	 * Skip throttling while sound is playing so the I2S ring buffer
	 * stays primed and avoids underrun pops on silence transitions. */
	if (s->sound.silence_cnt >= MAC_SOUND_SILENCE) {
		unsigned sf = s->speed_factor;
		int64_t frame_target_us;

		if (sf == 0) {
			sf = 1;
		}
		frame_target_us = 1000000LL / (60LL * (int64_t)sf);
		if (sched->frame_group_start_us != 0) {
			int64_t elapsed = (int64_t)(host_monotonic_us() - sched->frame_group_start_us);
			int64_t to_sleep = frame_target_us - elapsed;
			if (to_sleep > 0) {
				usleep((useconds_t)to_sleep);
			}
		}
	}
	sched->frame_group_start_us = host_monotonic_us();
}

int mac_clock_cpu_hz(const macplus_t *s)
{
	return s->clock_sched.cpu_hz;
}
