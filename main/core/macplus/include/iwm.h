/*
 * IWM stub — full PCE types/API: upstream pce/src/arch/macplus/iwm.h
 * One instance: macplus_t.iwm in macplus.c (same idea as PCE macplus_t.iwm).
 */

#ifndef IWM_H
#define IWM_H

#define MAC_IWM_DRIVES 3

typedef struct {
	unsigned char disk_inserted;
	unsigned char disk_switched; /* disk-changed event (snow SWITCHED): set on
	                              * insert/eject, cleared by CLRSWITCHED */
	unsigned char motor_on;
	unsigned char cur_cyl;
} mac_iwm_drive_t;

typedef struct {
	unsigned char lines;
	unsigned char head_sel;
	unsigned char mode;
	unsigned char handshake;
	mac_iwm_drive_t drv[MAC_IWM_DRIVES];
	mac_iwm_drive_t *curdrv;
} mac_iwm_t;

void mac_iwm_init(mac_iwm_t *iwm);
unsigned char mac_iwm_get_uint8(mac_iwm_t *iwm, unsigned long addr);
void mac_iwm_set_uint8(mac_iwm_t *iwm, unsigned long addr, unsigned char val);
void mac_iwm_set_head_sel(mac_iwm_t *iwm, unsigned char val);

/* Disk signal helpers (PCE mac_iwm_insert style — callers never poke
 * drv fields directly). */
/* Insert: disk present + disk-changed event (snow SWITCHED semantics).
 * Idempotent on disk_inserted. */
void mac_iwm_insert(mac_iwm_t *iwm, unsigned drive);

#endif
