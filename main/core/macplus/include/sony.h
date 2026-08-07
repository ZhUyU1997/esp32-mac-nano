/*
 * Sony floppy driver hook — adapted from PCE macplus/sony.c
 * Requires ROM with PCEX extension (see mac_sony_patch).
 */

#ifndef MACPLUS_SONY_H
#define MACPLUS_SONY_H

#include "block/block.h"

#define SONY_DRIVES 8

typedef struct mac_sony_s {
	char open;
	char patched;
	char enable;

	block_t *disk[SONY_DRIVES + 1];

	unsigned delay_val[SONY_DRIVES];
	unsigned delay_cnt[SONY_DRIVES];

	unsigned long check_addr;
	unsigned long icon_addr[2];

	unsigned long tag_buf;

	char format_hd_as_dd;

	unsigned format_cnt;
	unsigned long format_list[16];

	unsigned long pcex_addr;
	unsigned long sony_addr;
	unsigned char patch_buf[64];

	unsigned long d0;
	unsigned long a0;
	unsigned long a1;
	unsigned long pc;
	void (*on_eject)(unsigned drive, void *ctx);
	void *on_eject_ctx;
} mac_sony_t;

void mac_sony_init(mac_sony_t *sony, int enable);
void mac_sony_set_disk(mac_sony_t *sony, unsigned drive, block_t *d);
void mac_sony_patch(mac_sony_t *sony);
void mac_sony_set_delay(mac_sony_t *sony, unsigned drive, unsigned delay);
void mac_sony_insert(mac_sony_t *sony, unsigned drive);
void mac_sony_set_eject_callback(mac_sony_t *sony, void (*on_eject)(unsigned drive, void *ctx), void *ctx);
int mac_sony_disk_in_place(mac_sony_t *sony, unsigned drive);
int mac_sony_check(mac_sony_t *sony);
int mac_sony_hook(mac_sony_t *sony, unsigned val);
void mac_sony_reset(mac_sony_t *sony);

#endif
