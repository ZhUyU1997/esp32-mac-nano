#ifndef SCSI_H
#define SCSI_H

#include <stdint.h>

#include "block/block.h"

typedef struct mac_scsi_s mac_scsi_t;

typedef struct {
	int valid;
	block_t *disk;
	unsigned char vendor[8];
	unsigned char product[16];
} mac_scsi_dev_t;

typedef enum {
	SCSI_XFER_NONE = 0,
	SCSI_XFER_RESPONSE,
	SCSI_XFER_DISK_READ,
	SCSI_XFER_DISK_WRITE,
} scsi_xfer_kind_t;

struct mac_scsi_s {
	unsigned phase;
	unsigned char odr;
	unsigned char csd;
	unsigned char icr;
	unsigned char mr2;
	unsigned char tcr;
	unsigned char csb;
	unsigned char ser;
	unsigned char bsr;
	unsigned char status;
	unsigned char message;
	unsigned cmd_i;
	unsigned cmd_n;
	unsigned char cmd[16];
	unsigned long buf_i;
	unsigned long buf_n;
	unsigned char scratch[512];
	scsi_xfer_kind_t xfer_kind;
	unsigned long disk_read_lba0;
	unsigned long disk_read_cache_lba;
	unsigned char disk_read_cache[512];
	unsigned long disk_write_lba0;
	unsigned char disk_write_sec[512];
	unsigned sel_drv;
	void (*cmd_start)(mac_scsi_t *scsi);
	void (*cmd_finish)(mac_scsi_t *scsi);
	unsigned char set_int_val;
	void *set_int_ext;
	void (*set_int)(void *ext, unsigned char val);
	mac_scsi_dev_t dev[8];
};

void mac_scsi_init(mac_scsi_t *scsi);
void mac_scsi_free(mac_scsi_t *scsi);
void mac_scsi_set_int_fct(mac_scsi_t *scsi, void *ext, void (*fct)(void *ext, unsigned char val));
void mac_scsi_register_device(mac_scsi_t *scsi, unsigned id, block_t *disk);
unsigned char mac_scsi_get_uint8(void *ext, unsigned long addr);
unsigned short mac_scsi_get_uint16(void *ext, unsigned long addr);
void mac_scsi_set_uint8(void *ext, unsigned long addr, unsigned char val);
void mac_scsi_set_uint16(void *ext, unsigned long addr, unsigned short val);
void mac_scsi_reset(mac_scsi_t *scsi);

#endif
