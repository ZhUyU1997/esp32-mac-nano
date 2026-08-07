/* macplus ROM-patching code
 *
 * Copyright 2024 Matt Evans
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <inttypes.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "macplus_config.h"

/* Plus v3 (0x4d1f8172) load-time patches; defaults in main/CMakeLists.txt, or idf.py -DROM_PATCH_*=0/1. */
#ifndef ROM_PATCH_SKIP_CHECKSUM_LOOP
#define ROM_PATCH_SKIP_CHECKSUM_LOOP 0
#endif
#ifndef ROM_PATCH_SHORTEN_RAM_SELFTEST
#define ROM_PATCH_SHORTEN_RAM_SELFTEST 0
#endif
#ifndef ROM_PATCH_HLE_ERASE_SCRN
#define ROM_PATCH_HLE_ERASE_SCRN 0
#endif
#ifndef ROM_PATCH_HLE_BOOT_PART2_RAM
#define ROM_PATCH_HLE_BOOT_PART2_RAM 0
#endif
#ifndef ROM_PATCH_SKIP_MBOOT_BEEP
#define ROM_PATCH_SKIP_MBOOT_BEEP 0
#endif
#ifndef ROM_PATCH_CENTER_DS_ALERT_RECT
#define ROM_PATCH_CENTER_DS_ALERT_RECT 1
#endif

#ifdef DEBUG
#define RDBG(...) printf(__VA_ARGS__)
#else
#define RDBG(...)                                                                                                                                              \
	do {                                                                                                                                                   \
	} while (0)
#endif

#define RERR(...) fprintf(stderr, __VA_ARGS__)

#define ROM_PLUSv3_VERSION 0x4d1f8172
#define ROM_PLUSv3_SONYDRV 0x17d30

#define M68K_INST_NOP 0x4e71
#define M68K_INST_JMP_A6 0x4ed6u
/* Plus v3 ROM 0x4d1f8172 (plus-rom-listing.asm); each group only if that patch is on. */
#if ROM_PATCH_SKIP_CHECKSUM_LOOP
#define M68K_PLUSV3_BRA_SKIP_CHECKSUM 0x6022u /* BRA.S +0x22: 0xD7C+0x22 -> 0xD9E */
#endif
#if ROM_PATCH_SHORTEN_RAM_SELFTEST
#define ROM_PLUSV3_OFF_RAMTEST_BPL 0x0e90u /* was BPL.B L127 (read loop) */
#define ROM_PLUSV3_OFF_RAMTEST_BGT 0x0ea8u /* was BGT.B L128 (verify loop) */
#endif

////////////////////////////////////////////////////////////////////////////////
// Replacement drivers to thwack over the ROM

////////////////////////////////////////////////////////////////////////////////

static uint32_t rom_get_version(uint8_t *rom_base)
{
#if BYTE_ORDER == LITTLE_ENDIAN
	return __builtin_bswap32(*(uint32_t *)rom_base);
#else
	return *(uint32_t *)rom_base;
#endif
}

/* Not perf-critical, so open-coding to support BE _and_ unaligned access */
#define ROM_WR32(offset, data)                                                                                                                                 \
	do {                                                                                                                                                   \
		rom_base[(offset) + 0] = ((data) >> 24) & 0xff;                                                                                                \
		rom_base[(offset) + 1] = ((data) >> 16) & 0xff;                                                                                                \
		rom_base[(offset) + 2] = ((data) >> 8) & 0xff;                                                                                                 \
		rom_base[(offset) + 3] = (data)&0xff;                                                                                                          \
	} while (0)
#define ROM_WR16(offset, data)                                                                                                                                 \
	do {                                                                                                                                                   \
		rom_base[(offset) + 0] = ((data) >> 8) & 0xff;                                                                                                 \
		rom_base[(offset) + 1] = (data)&0xff;                                                                                                          \
	} while (0)
#define ROM_WR8(offset, data)                                                                                                                                  \
	do {                                                                                                                                                   \
		rom_base[(offset) + 0] = (data)&0xff;                                                                                                          \
	} while (0)

static void rom_patch_plusv3(uint8_t *rom_base)
{
	/* Inspired by BasiliskII / Mini vMac (fast boot); see ROM_PATCH_* at top of this file.
         */

#if ROM_PATCH_SKIP_CHECKSUM_LOOP
	/* Skip P_ChecksumRomAndTestMemory test 1 (ROM word sum): BRA.S over loop to 0xD9E. */
	ROM_WR16(0xd7a, M68K_PLUSV3_BRA_SKIP_CHECKSUM);
#else
	/* HEAD / BasiliskII: bodging compare (eor.l d3,d1) -> eor.l d1,d1 so checksum matches. */
	ROM_WR16(0xd92, 0xb381 /* eor.l d1, d1 */);
#endif
#if ROM_PATCH_SHORTEN_RAM_SELFTEST
	/* Shorten in-ROM RAM read/verify loops (Mini vMac DisableRamTest). */
	ROM_WR16(ROM_PLUSV3_OFF_RAMTEST_BPL, M68K_INST_NOP);
	ROM_WR16(ROM_PLUSV3_OFF_RAMTEST_BGT, M68K_INST_NOP);
#endif

	/* To do:
         *
         * - No IWM init
         * - new Sound?
         */
#if MACPLUS_MEMSIZE_KB > 128 && MACPLUS_MEMSIZE_KB < 512
	/* Hack to change memtop: try out a 256K Mac :) */
	for (int i = 0x376; i < 0x37e; i += 2)
		ROM_WR16(i, M68K_INST_NOP);
	ROM_WR16(0x376, 0x2a7c); // moveal #RAM_SIZE, A5
	ROM_WR16(0x378, RAM_SIZE >> 16);
	ROM_WR16(0x37a, RAM_SIZE & 0xffff);
	/* That overrides the probed memory size, but
         * P_ChecksumRomAndTestMemory returns a failure code for
         * things that aren't 128/512.  Skip that:
         */
	ROM_WR16(0x132, 0x6000); // Bra (was BEQ)
	                         /* FIXME: We should also remove the memory probe routine, by
         * allowing the ROM checksum to fail (it returns failure, then
         * we carry on).  This avoids wild RAM addrs being accessed.
         */
#endif

#if DISP_WIDTH != 512 || DISP_HEIGHT != 342
#if (SCREEN_DISTANCE_FROM_TOP >= 65536)
#error "rom.c: Screen res patching maths won't work for a screen this large"
#endif
#define SCREEN_BASE (0x400000 - SCREEN_DISTANCE_FROM_TOP)
#define SCREEN_BASE_L16 (SCREEN_BASE & 0xffff)
#define SBCOORD(x, y) (SCREEN_BASE + ((DISP_WIDTH / 8) * (y)) + ((x) / 8))

	/* Changing video res:
         *
         * Original 512*342 framebuffer is 0x5580 bytes; the screen
         * buffer lands underneath sound/other buffers at top of mem,
         * i,e, 0x3fa700 = 0x400000-0x5580-0x380.  So any new buffer
         * will be placed (and read out from for the GUI) at
         * MEM_TOP-0x380-SCREEN_SIZE.
         *
         * For VGA, size is 0x9600 bytes (0x2580 words)
         */

	/* We need some space, low down, to create jump-out-and-patch
         * routines where a patch is too large to put inline.  The
         * TestSoftware check at 0x42 isn't used:
         */
	ROM_WR16(0x42, 0x6000);      /* bra */
	ROM_WR16(0x44, 0x62 - 0x44); /* offset */
	/* Now 0x46-0x57 can be used */
	unsigned int patch_0 = 0x46;
	ROM_WR16(patch_0 + 0, 0x9bfc); /* suba.l #imm32, A5 */
	ROM_WR16(patch_0 + 2, 0);      /* (Could add more here) */
	ROM_WR16(patch_0 + 4, SCREEN_DISTANCE_FROM_TOP);
	ROM_WR16(patch_0 + 6, 0x6000);                /* bra */
	ROM_WR16(patch_0 + 8, 0x3a4 - (patch_0 + 8)); /* Return to 3a4 */

	/* Magic screen-related locations in Mac Plus ROM 4d1f8172:
         *
         * 8c : screen base addr (usually 3fa700, now 3f6680)
         * 148 : screen base addr again
         * 164 : u32 screen address of crash Mac/critErr hex numbers
         * 188 : u16 bytes per row (critErr)
         * 194 : u16 bytes per row (critErr)
         * 19c : u16 (bytes per row * 6)-1 (critErr)
         * 1a4 : u32 screen address of critErr twiddly pattern
         * 1ee : u16 screen sie in words minus one
         * 3a2 : u16 screen size in bytes (BUT can't patch immediate)
         * 474 : u16 bytes per row
         * 494 : u16 screen y
         * 498 : u16 screen x
         * a0e : y
         * a10 : x
         * ee2 : u16 bytes per row minus 4 (tPutIcon)
         * ef2 : u16 bytes per row (tPutIcon)
         * 7e0 : u32 screen address of disk icon (240, 145)
         * 7f2 : u32 screen address of disk icon's symbol (248, 160)
         * f0c : u32 screen address of Mac icon (240, 145)
         * f18 : u32 screen address of Mac icon's face (248, 151)
         * f36 : u16 bytes per row minus 2 (mPutSymbol)
         * 1cd1 : hidecursor's bytes per line
         * 1d48 : xres minus 32 (for cursor rect clipping)
         * 1d4e : xres minus 32
         * 1d74 : y
         * 1d93 : bytes per line (showcursor)
         * 1e68 : y
         * 1e6e : x
         * 1e82 : y
         */
	ROM_WR16(0x8c, SCREEN_BASE_L16);
	ROM_WR16(0x148, SCREEN_BASE_L16);
	ROM_WR32(0x164, SBCOORD(DISP_WIDTH / 2 - (48 / 2), DISP_HEIGHT / 2 + 8));
	ROM_WR16(0x188, DISP_WIDTH / 8);
	ROM_WR16(0x194, DISP_WIDTH / 8);
	ROM_WR16(0x19c, (6 * DISP_WIDTH / 8) - 1);
	ROM_WR32(0x1a4, SBCOORD(DISP_WIDTH / 2 - 8, DISP_HEIGHT / 2 + 8 + 8));
	ROM_WR16(0x1ee, (SCREEN_SIZE / 4) - 1);

	ROM_WR32(0xf0c, SBCOORD(DISP_WIDTH / 2 - 16, DISP_HEIGHT / 2 - 26));
	ROM_WR32(0xf18, SBCOORD(DISP_WIDTH / 2 - 8, DISP_HEIGHT / 2 - 20));
	ROM_WR32(0x7e0, SBCOORD(DISP_WIDTH / 2 - 16, DISP_HEIGHT / 2 - 26));
	ROM_WR32(0x7f2, SBCOORD(DISP_WIDTH / 2 - 8, DISP_HEIGHT / 2 - 11));

	/* Patch "SubA #$5900, A5" to subtract 0x9880.
         * However... can't just patch the int16 immediate, as that's
         * sign-extended (and we end up with a subtract-negative,
         * i.e. an add).  There isn't space here to turn it into sub.l
         * so add some rigamarole to branch to some bytes stolen at
         * patch_0 up above.
         */
	ROM_WR16(0x3a0, 0x6000);          /* bra */
	ROM_WR16(0x3a2, patch_0 - 0x3a2); /* ...to patch0, returns at 0x3a4 */

	ROM_WR16(0x474, DISP_WIDTH / 8);
	ROM_WR16(0x494, DISP_HEIGHT);
	ROM_WR16(0x498, DISP_WIDTH);
	ROM_WR16(0xa0e, DISP_HEIGHT); /* copybits? */
	ROM_WR16(0xa10, DISP_WIDTH);
	ROM_WR16(0xee2, (DISP_WIDTH / 8) - 4); /* tPutIcon bytes per row, minus 4 */
	ROM_WR16(0xef2, DISP_WIDTH / 8);       /* tPutIcon bytes per row */
	ROM_WR16(0xf36, (DISP_WIDTH / 8) - 2); /* tPutIcon bytes per row, minus 2 */
	ROM_WR8(0x1cd1, DISP_WIDTH / 8);       /* hidecursor */
	ROM_WR16(0x1d48, DISP_WIDTH - 32);     /* 1d46+2 was originally 512-32 rite? */
	ROM_WR16(0x1d4e, DISP_WIDTH - 32);     /* 1d4c+2 is 480, same */
	ROM_WR16(0x1d6e, DISP_HEIGHT - 16);    /* showcursor (YESS fixed Y crash bug!) */
	ROM_WR16(0x1d74, DISP_HEIGHT);         /* showcursor */
	ROM_WR8(0x1d93, DISP_WIDTH / 8);       /* showcursor */
	ROM_WR16(0x1e68, DISP_HEIGHT);         /* mScrnSize */
	ROM_WR16(0x1e6e, DISP_WIDTH);          /* mScrnSize */
	ROM_WR16(0x1e82, DISP_HEIGHT);         /* tScrnBitMap */
#endif

#if ROM_PATCH_CENTER_DS_ALERT_RECT
	/*
         * Patch QuickDraw `Rect` immediates for DS alert chrome when DISP_WIDTH != 512.
         * Low-memory `DSAlertRect` is at $3F8 (top/left at $3F8, bottom/right at $3FC).
         *
         * P19 (plus-rom-listing.asm ~L1803): after `Lea (DSAlertRect), A0`, the ROM fills the
         * rect for a 512-wide screen (448×126). We only recompute left/right from DISP_WIDTH;
         * top/bottom stay as in ROM.
         *   1356  Move.L  #$400020, (A0)+     ; imm @ $1358: top=$0040 (64), left=$0020 (32)
         *   135C  Move.L  #$BE01E0, (A0)      ; imm @ $135E: bottom=$00BE (190), right=$01E0 (480)
         *
         * P_mDSHook, label L437 (~L4051): absolute stores to DSAlertRect / .botRight for a
         * smaller box (285×72). Same rule: fixed vertical span, center horizontally.
         *   2AB6  Move.L  #$500078, (DSAlertRect)       ; imm @ $2AB8: top=$0050 (80), left=$0078 (120)
         *   2ABE  Move.L  #$980195, (DSAlertRect+4)     ; imm @ $2AC0: bottom=$0098 (152), right=$0195 (405)
         */
	{
		const int p19_w = 480 - 32;
		const int p19_top = 64;
		const int p19_bottom = 190;
		const int p19_left = (DISP_WIDTH - p19_w) / 2;
		const int p19_right = p19_left + p19_w;
		uint32_t p19_lo = ((uint32_t)(uint16_t)p19_top << 16) | (uint16_t)p19_left;
		uint32_t p19_hi = ((uint32_t)(uint16_t)p19_bottom << 16) | (uint16_t)p19_right;

		ROM_WR32(0x1358, p19_lo);
		ROM_WR32(0x135e, p19_hi);

		const int hk_w = 405 - 120;
		const int hk_top = 80;
		const int hk_bottom = 152;
		const int hk_left = (DISP_WIDTH - hk_w) / 2;
		const int hk_right = hk_left + hk_w;
		uint32_t hk_lo = ((uint32_t)(uint16_t)hk_top << 16) | (uint16_t)hk_left;
		uint32_t hk_hi = ((uint32_t)(uint16_t)hk_bottom << 16) | (uint16_t)hk_right;

		ROM_WR32(0x2ab8, hk_lo);
		ROM_WR32(0x2ac0, hk_hi);
	}
#endif
#if ROM_PATCH_HLE_ERASE_SCRN
	/*
         * P_EraseScrnBuff @ 0x1EA — apply last: when DISP != 512x342, ROM_WR16(0x1ee, …) above
         * overwrites the same words; doing this earlier left garbage and no MMIO / no logs.
         * Host: byte 21 @ MAC_MMIO_HOOK_ADDR (macplus.c). Absolute store avoids clobbering A0.
         *   move.b #21, ($00EFFD00).l | 13 fc 00 15 00 ef fd 00
         *   jmp    (%a6)              | 4e d6
         *   nop                       | 4e 71   (pad to same 12 bytes as old patch)
         */
	ROM_WR32(0x1ea, 0x13fc0015u);
	ROM_WR32(0x1ee, 0x00effd00u);
	ROM_WR16(0x1f2, 0x4ed6u);
	ROM_WR16(0x1f4, M68K_INST_NOP);
#endif
#if ROM_PATCH_HLE_BOOT_PART2_RAM
	/*
         * P_BootPart2 @ 0x352 (plus-rom-listing.asm): was longword fill $FF from A3 to D0 (end addr).
         * Host: byte 22 @ MAC_MMIO_HOOK_ADDR; uses A3=start, D0=end (exclusive), same as unpatched entry.
         *   move.b #22, ($00EFFD00).l | 13 fc 00 16 00 ef fd 00
         *   nop nop nop             | 4e 71 x3  (14 bytes; was move/a/sub/lsr/loop)
         */
	ROM_WR32(0x352, 0x13fc0016u);
	ROM_WR32(0x356, 0x00effd00u);
	ROM_WR16(0x35a, M68K_INST_NOP);
	ROM_WR16(0x35c, M68K_INST_NOP);
	ROM_WR16(0x35e, M68K_INST_NOP);
#endif
#if ROM_PATCH_SKIP_MBOOT_BEEP
	/* P_mBootBeep @ 0x28A — was Lea VIA;…; jmp (a6) @ 0x350. Single jmp (a6) skips beep. */
	ROM_WR16(0x28a, M68K_INST_JMP_A6);
#endif
}

int rom_patch(uint8_t *rom_base)
{
	uint32_t v = rom_get_version(rom_base);
	int r = -1;
	/* See https://docs.google.com/spreadsheets/d/1wB2HnysPp63fezUzfgpk0JX_b7bXvmAg6-Dk7QDyKPY/edit#gid=840977089
         */
	switch (v) {
	case ROM_PLUSv3_VERSION:
		rom_patch_plusv3(rom_base);
		r = 0;
		break;

	default:
		RERR("Unknown ROM version %08lx, no patching", v);
	}

	return r;
}
