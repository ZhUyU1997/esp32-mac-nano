/*
 * Derived from PCE src/arch/macplus/rtc.c (GPL-2.0).
 * Hampa Hug — adapted for esp32-mac-nano (no ini/file, no PCE logging).
 */

#include "rtc.h"

#include <time.h>

void mac_rtc_init(mac_rtc_t *rtc)
{
	rtc->set_data_ext = NULL;
	rtc->set_data = NULL;
	rtc->set_data_val = 0;

	rtc->set_osi_ext = NULL;
	rtc->set_osi = NULL;
	rtc->set_osi_val = 0;

	rtc->data_out = 0;
	rtc->state = 0;
	rtc->bitcnt = 0;
	rtc->sigval = 0;

	rtc->realtime = 0;
	rtc->clkcnt = 0;

	rtc->reg_wp = 0;
	rtc->reg_test = 0;

	rtc->clock = 0;
	rtc->bias = 0;

	mac_rtc_set_defaults(rtc);
}

unsigned char mac_rtc_get_byte(const mac_rtc_t *rtc, unsigned addr)
{
	if (addr < 256) return rtc->data[addr];
	return 0;
}

void mac_rtc_set_byte(mac_rtc_t *rtc, unsigned addr, unsigned char val)
{
	if (addr < 256) rtc->data[addr] = val;
}

void mac_rtc_free(mac_rtc_t *rtc)
{
	(void)rtc;
}

void mac_rtc_set_data_fct(mac_rtc_t *rtc, void *ext, void *fct)
{
	rtc->set_data_ext = ext;
	rtc->set_data = fct;
}

void mac_rtc_set_osi_fct(mac_rtc_t *rtc, void *ext, void *fct)
{
	rtc->set_osi_ext = ext;
	rtc->set_osi = fct;
}

void mac_rtc_set_realtime(mac_rtc_t *rtc, int realtime)
{
	rtc->realtime = (realtime != 0);
}

void mac_rtc_set_defaults(mac_rtc_t *rtc)
{
	unsigned i;

	for (i = 0; i < 256; i++) {
		rtc->data[i] = 0;
	}

	rtc->data[0x10] = 0xa8;
	rtc->data[0x13] = 0x22;
	rtc->data[0x1e] = 0x64;
	rtc->data[0x08] = 0x18;
	rtc->data[0x09] = 0x88;
	rtc->data[0x0b] = 0x20;
}

static unsigned long mac_rtc_get_current_time(mac_rtc_t *rtc)
{
	time_t ut;
	unsigned long mt;

	(void)rtc;
	ut = time(NULL);
	mt = (unsigned long)ut;
	mt += 2082844800UL;
	return (mt);
}

static unsigned long mac_rtc_get_timezone(mac_rtc_t *rtc)
{
	unsigned long tz;

	tz = rtc->data[0xed];
	tz = (tz << 8) | rtc->data[0xee];
	tz = (tz << 8) | rtc->data[0xef];

	if (tz & 0x800000) {
		tz |= 0xff000000;
	}

	return (tz);
}

static void mac_rtc_set_data(mac_rtc_t *rtc, unsigned char val)
{
	rtc->set_data_val = (val != 0);

	if (rtc->set_data != NULL) {
		rtc->set_data(rtc->set_data_ext, rtc->set_data_val);
	}
}

static void mac_rtc_set_osi(mac_rtc_t *rtc, unsigned char val)
{
	val = (val != 0);

	if (rtc->set_osi_val == val) {
		return;
	}

	rtc->set_osi_val = val;

	if (rtc->set_osi != NULL) {
		rtc->set_osi(rtc->set_osi_ext, val);
	}
}

static void mac_rtc_cmd1_read(mac_rtc_t *rtc)
{
	unsigned char reg;

	reg = (rtc->cmd1 >> 2) & 0x1f;

	if ((rtc->cmd1 & 0xe3) == 0x81) {
		rtc->shift = (rtc->clock >> (8 * (reg & 3))) & 0xff;
	} else if ((rtc->cmd1 & 0xf3) == 0xa1) {
		rtc->shift = rtc->data[8 + ((rtc->cmd1 >> 2) & 3)];
	} else if ((rtc->cmd1 & 0xc3) == 0xc1) {
		rtc->shift = rtc->data[16 + ((rtc->cmd1 >> 2) & 15)];
	} else {
		rtc->shift = 0x00;
	}
}

static void mac_rtc_cmd1_write(mac_rtc_t *rtc)
{
	if (rtc->cmd1 == 0x35) {
		rtc->reg_wp = rtc->shift & 0x80;
		return;
	}

	if (rtc->reg_wp & 0x80) {
		return;
	}

	if ((rtc->cmd1 & 0xe3) == 0x01) {
		unsigned bit;
		unsigned long val;

		bit = 8 * ((rtc->cmd1 >> 2) & 3);
		val = rtc->shift & 0xff;

		rtc->clock &= ~(0x000000ffUL << bit);
		rtc->clock |= val << bit;
	} else if ((rtc->cmd1 & 0xf3) == 0x21) {
		rtc->data[8 + ((rtc->cmd1 >> 2) & 3)] = rtc->shift;
	} else if (rtc->cmd1 == 0x31) {
		rtc->reg_test = rtc->shift;
	} else if ((rtc->cmd1 & 0xc3) == 0x41) {
		rtc->data[16 + ((rtc->cmd1 >> 2) & 15)] = rtc->shift;
	}
}

static void mac_rtc_cmd2_read(mac_rtc_t *rtc)
{
	unsigned addr;

	addr = ((rtc->cmd1 & 7) << 5) | ((rtc->cmd2 >> 2) & 0x1f);

	if (addr < 256) {
		rtc->shift = rtc->data[addr];
	} else {
		rtc->shift = 0;
	}
}

static void mac_rtc_cmd2_write(mac_rtc_t *rtc)
{
	unsigned addr;

	if (rtc->reg_wp & 0x80) {
		return;
	}

	addr = ((rtc->cmd1 & 7) << 5) | ((rtc->cmd2 >> 2) & 0x1f);

	if (addr < 256) {
		rtc->data[addr] = rtc->shift;
	}
}

void mac_rtc_set_uint8(mac_rtc_t *rtc, unsigned char val)
{
	unsigned char dif;

	dif = rtc->sigval ^ val;
	rtc->sigval = val;

	if (val & 0x04) {
		rtc->state = 0;
		rtc->data_out = 0;
		rtc->bitcnt = 0;
		return;
	}

	if ((dif & ~val & 0x02) == 0) {
		return;
	}

	if (rtc->data_out) {
		mac_rtc_set_data(rtc, rtc->shift & 0x80);

		rtc->shift = (rtc->shift << 1) | ((rtc->shift >> 7) & 0x01);

		rtc->bitcnt += 1;

		if (rtc->bitcnt >= 8) {
			rtc->bitcnt = 0;
			rtc->data_out = 0;
			rtc->state = 0;
		}
	} else {
		rtc->shift = (rtc->shift << 1) | (val & 0x01);

		rtc->bitcnt += 1;

		if (rtc->bitcnt >= 8) {
			if (rtc->state == 0) {
				rtc->cmd1 = rtc->shift;

				if ((rtc->cmd1 & 0x78) == 0x38) {
					rtc->state = 2;
				} else if (rtc->cmd1 & 0x80) {
					mac_rtc_cmd1_read(rtc);
					rtc->state = 0;
					rtc->data_out = 1;
				} else {
					rtc->state = 1;
				}
			} else if (rtc->state == 1) {
				mac_rtc_cmd1_write(rtc);
				rtc->state = 0;
			} else if (rtc->state == 2) {
				rtc->cmd2 = rtc->shift;
				if (rtc->cmd1 & 0x80) {
					mac_rtc_cmd2_read(rtc);
					rtc->state = 0;
					rtc->data_out = 1;
				} else {
					rtc->state = 3;
				}
			} else if (rtc->state == 3) {
				mac_rtc_cmd2_write(rtc);
				rtc->state = 0;
			}

			rtc->bitcnt = 0;
		}
	}
}

void mac_rtc_clock(mac_rtc_t *rtc, unsigned long n)
{
	unsigned long old;

	old = rtc->clock;

	if (rtc->realtime) {
		rtc->clock = mac_rtc_get_current_time(rtc);
		rtc->clock += mac_rtc_get_timezone(rtc);
		rtc->clock += rtc->bias;
	} else {
		rtc->clkcnt += n;

		if (rtc->clkcnt > MAC_CPU_CLOCK) {
			rtc->clkcnt -= MAC_CPU_CLOCK;
			rtc->clock += 1;
		}

		rtc->clock += rtc->bias;
		rtc->bias = 0;
	}

	rtc->clock &= 0xffffffff;

	if (rtc->clock != old) {
		mac_rtc_set_osi(rtc, 1);
		mac_rtc_set_osi(rtc, 0);
	}
}
