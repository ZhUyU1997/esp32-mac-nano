#ifndef MACPLUS_MEMMAP_H
#define MACPLUS_MEMMAP_H

#include <stdint.h>

#include "fast_attr.h"
#include "macplus_config.h"

/* 64KB granularity: the last block (video + sound buffers) needs only
 * 64KB of internal DRAM. Must match MACPLUS_BLOCK_SIZE in macplus_config.h. */
#define MEMMAP_ES MACPLUS_BLOCK_SIZE
#define MEMMAP_MAX_ADDR 0x1000000
#define FLAG_RO (1 << 0)

typedef uint8_t (*perip_access_cb_t)(unsigned int address, int data, int is_write);

struct memmap_ent {
	uint8_t *mem_addr;
	union {
		perip_access_cb_t cb;
		int flags;
	};
};

struct memmap_handler_ent {
	unsigned int start_addr;
	unsigned int end_addr;
	perip_access_cb_t cb;
};

struct memmap_handlers {
	perip_access_cb_t unhandled_cb;
	perip_access_cb_t bogus_read_cb;
	const struct memmap_handler_ent *cb_map;
	unsigned int cb_map_count;
};

extern struct memmap_ent memmap[MEMMAP_MAX_ADDR / MEMMAP_ES];

void memmap_rebuild_direct(int remap_rom,
                           unsigned char *mac_rom,
                           unsigned char *ram_blocks[MACPLUS_RAMSIZE / MEMMAP_ES],
                           unsigned int ram_block_count,
                           const struct memmap_handlers *handlers);

static inline const struct memmap_ent *FAST_FUNC_ATTR get_memmap_ent(unsigned int address)
{
	if (address >= MEMMAP_MAX_ADDR) {
		return &memmap[(MEMMAP_MAX_ADDR / MEMMAP_ES) - 1];
	}
	return &memmap[address / MEMMAP_ES];
}

#endif
