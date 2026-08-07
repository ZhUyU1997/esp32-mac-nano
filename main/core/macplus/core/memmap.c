#include "memmap.h"

#include <stddef.h>

struct memmap_ent FAST_DATA_ATTR memmap[MEMMAP_MAX_ADDR / MEMMAP_ES];

static inline unsigned int memmap_addr_to_idx(unsigned int addr)
{
	return addr / MEMMAP_ES;
}

static void memmap_reset(perip_access_cb_t default_cb)
{
	for (int i = 0; i < MEMMAP_MAX_ADDR / MEMMAP_ES; i++) {
		memmap[i].mem_addr = 0;
		memmap[i].cb = default_cb;
	}
}

static void memmap_map_ram_range(unsigned int start_addr, unsigned int end_addr, unsigned char **ram_blocks, unsigned int ram_block_count)
{
	if (ram_blocks == NULL || ram_block_count == 0) {
		return;
	}
	unsigned int start = memmap_addr_to_idx(start_addr);
	unsigned int end = memmap_addr_to_idx(end_addr);
	for (unsigned int i = start; i < end; i++) {
		/* Keep baseline semantics: absolute memmap index modulo RAM blocks. */
		memmap[i].mem_addr = ram_blocks[i % ram_block_count];
		memmap[i].flags = 0;
	}
}

/* Map one contiguous host buffer across a range of memmap entries
 * (128KB ROM at 64KB granularity spans two entries).
 * Each entry sees its own slice: ram + (i - start) * MEMMAP_ES. */
static void memmap_map_ram_range_single(unsigned int start_addr, unsigned int end_addr, unsigned char *ram, int read_only)
{
	unsigned int start = memmap_addr_to_idx(start_addr);
	unsigned int end = memmap_addr_to_idx(end_addr);
	for (unsigned int i = start; i < end; i++) {
		memmap[i].mem_addr = ram + (i - start) * MEMMAP_ES;
		memmap[i].flags = read_only ? FLAG_RO : 0;
	}
}

static void memmap_map_cb_range(unsigned int start_addr, unsigned int end_addr, perip_access_cb_t cb)
{
	unsigned int start = memmap_addr_to_idx(start_addr);
	unsigned int end = memmap_addr_to_idx(end_addr);
	for (unsigned int i = start; i < end; i++) {
		memmap[i].mem_addr = NULL;
		memmap[i].cb = cb;
	}
}

void memmap_rebuild_direct(int remap_rom,
                           unsigned char *mac_rom,
                           unsigned char *ram_blocks[MACPLUS_RAMSIZE / MEMMAP_ES],
                           unsigned int ram_block_count,
                           const struct memmap_handlers *handlers)
{
	if (handlers == NULL) {
		return;
	}

	memmap_reset(handlers->unhandled_cb);

	if (remap_rom) {
		memmap_map_ram_range_single(0x000000, MACPLUS_ROMSIZE, mac_rom, 1);
		memmap_map_cb_range(0x020000, 0x400000, handlers->bogus_read_cb);
	} else {
		memmap_map_ram_range(0x000000, 0x400000, ram_blocks, ram_block_count);
	}

	memmap_map_ram_range_single(0x400000, 0x400000 + MACPLUS_ROMSIZE, mac_rom, 1);
	memmap_map_cb_range(0x420000, 0x500000, handlers->bogus_read_cb);
	memmap_map_ram_range(0x600000, 0x700000, ram_blocks, ram_block_count);

	for (unsigned int i = 0; i < handlers->cb_map_count; i++) {
		const struct memmap_handler_ent *ent = &handlers->cb_map[i];
		memmap_map_cb_range(ent->start_addr, ent->end_addr, ent->cb);
	}
}
