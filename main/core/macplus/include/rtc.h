/*
 * Derived from PCE src/arch/macplus/rtc.h (GPL-2.0).
 * See upstream pce/src/arch/macplus/rtc.c
 */

#ifndef MACPLUS_MAC_RTC_H
#define MACPLUS_MAC_RTC_H

#include <stdint.h>

/* Emulated 68000 cycles per second — match main emu loop (8 MHz). */
#ifndef MAC_CPU_CLOCK
#define MAC_CPU_CLOCK 8000000u
#endif

typedef struct {
	unsigned char data[256];

	unsigned char reg_wp;
	unsigned char reg_test;

	unsigned long clock;
	unsigned long bias;

	int data_out;
	unsigned state;
	unsigned bitcnt;
	unsigned char cmd1;
	unsigned char cmd2;
	unsigned char shift;
	unsigned char sigval;

	int realtime;

	unsigned long clkcnt;

	void *set_data_ext;
	void (*set_data)(void *ext, unsigned char val);
	unsigned char set_data_val;

	void *set_osi_ext;
	void (*set_osi)(void *ext, unsigned char val);
	unsigned char set_osi_val;
} mac_rtc_t;

void mac_rtc_init(mac_rtc_t *rtc);
void mac_rtc_free(mac_rtc_t *rtc);

void mac_rtc_set_data_fct(mac_rtc_t *rtc, void *ext, void *fct);
void mac_rtc_set_osi_fct(mac_rtc_t *rtc, void *ext, void *fct);

void mac_rtc_set_realtime(mac_rtc_t *rtc, int realtime);

void mac_rtc_set_defaults(mac_rtc_t *rtc);

void mac_rtc_set_uint8(mac_rtc_t *rtc, unsigned char val);

void mac_rtc_clock(mac_rtc_t *rtc, unsigned long n);

unsigned char mac_rtc_get_byte(const mac_rtc_t *rtc, unsigned addr);
void         mac_rtc_set_byte(mac_rtc_t *rtc, unsigned addr, unsigned char val);

#endif
