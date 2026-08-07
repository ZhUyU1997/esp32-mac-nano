#ifndef __BLOCK_H__
#define __BLOCK_H__

#include <stdint.h>
#include <stddef.h>

#include "device.h"

struct dtnode_t;
struct driver_t;

class(block_t, device_t)
{
	uint64_t (*capacity)(struct block_t * blk);
	uint64_t (*read)(struct block_t * blk, uint8_t * dst, uint64_t offset, uint64_t count);
	uint64_t (*write)(struct block_t * blk, const uint8_t *src, uint64_t offset, uint64_t count);
	void (*sync)(struct block_t * blk);
	uint8_t readonly; /* WPROT (PCE dsk_get_readonly) */
};

device_t *register_block(block_t *blk, struct driver_t *drv, const struct dtnode_t *n);
void unregister_block(block_t *blk);
block_t *block_lookup(const char *name);

uint64_t block_capacity(block_t *blk);
uint64_t block_read(block_t *blk, uint8_t *dst, uint64_t offset, uint64_t count);
uint64_t block_write(block_t *blk, const uint8_t *src, uint64_t offset, uint64_t count);
void block_sync(block_t *blk);

#endif
