#define MACPLUS_ROMSIZE (128 * 1024)

// #define DISP_WIDTH 640
// #define DISP_HEIGHT 480
#define MACPLUS_MEMSIZE_KB RAM_SIZE

#define SCREEN_SIZE (DISP_WIDTH * DISP_HEIGHT / 8)
#define SCREEN_DISTANCE_FROM_TOP (SCREEN_SIZE + 0x380)

/* Memmap entry granularity (must match MEMMAP_ES in memmap.h).
 * 64KB: the last block (video + sound buffers) needs only 64KB of internal
 * DRAM; the rest lives in PSRAM. */
#define MACPLUS_BLOCK_SIZE (64 * 1024)

//Emulate a 128KiB MacPlus/Mac128K hybrid
#define MACPLUS_CACHESIZE 0
#define MACPLUS_RAMSIZE (MACPLUS_MEMSIZE_KB * 1024)
#define MACPLUS_SCREENBUF (MACPLUS_RAMSIZE - SCREEN_DISTANCE_FROM_TOP)
#define MACPLUS_SCREENBUF_ALT (MACPLUS_SCREENBUF - SCREEN_DISTANCE_FROM_TOP)
#define MACPLUS_SNDBUF (MACPLUS_RAMSIZE - 0x300)

/* Buffer offsets within the last block, derived from absolute addresses via
 * & (block size - 1). SCREENBUF_ALT (0x3ECD00) lies in the second-to-last
 * block (crosses the boundary); vbuf2 is not rendered, this offset is just a
 * valid in-block placeholder pointer. */
#define MACPLUS_SCREENBUF_OFFSET (MACPLUS_SCREENBUF & (MACPLUS_BLOCK_SIZE - 1))
#define MACPLUS_SCREENBUF_ALT_OFFSET (MACPLUS_SCREENBUF_ALT & (MACPLUS_BLOCK_SIZE - 1))
#define MACPLUS_SNDBUF_OFFSET (MACPLUS_SNDBUF & (MACPLUS_BLOCK_SIZE - 1))

//Source: Guide to the Macintosh family hardware
#define MACPLUS_SNDBUF_ALT (MACPLUS_SNDBUF - 0x5C00)
#define MACPLUS_SNDBUF_ALT_OFFSET (MACPLUS_SNDBUF_ALT & (MACPLUS_BLOCK_SIZE - 1))
