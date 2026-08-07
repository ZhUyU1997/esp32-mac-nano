/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include "macplus.h"
#include "m68k.h"
#include "mac_hid_bridge.h"
#include "msg.h"
#include <stdbool.h>
#include <byteswap.h>
#include "fast_attr.h"
#include "macplus_config.h"
#include "memmap.h"
#include "m68k_pc_sample.h"
#include "mac_trap_log.h"

#define M68K_CYCLES_PER_FRAME MAC_SOUND_CYCLES_PER_FRAME
#define M68K_CPU_STEP 814

/* Guest EA for move.b hooks; must match faulting_address in macintosh/pcex/pcex_mmio.S (not the PCEX blob marker). */
#define MAC_MMIO_HOOK_ADDR 0x00EFFD00u
/* ROM P_EraseScrnBuff HLE (rom.c ROM_PATCH_HLE_ERASE_SCRN). */
#define MAC_HOOK_ERASE_SCRN 21u
/* ROM P_BootPart2 RAM init HLE (rom.c ROM_PATCH_HLE_BOOT_PART2_RAM). */
#define MAC_HOOK_BOOT_PART2_RAM 22u
/* Sensor / RTC read — Mac app writes to 0xEFFD00 to trigger. */

/* printf tracing for MAC_MMIO_HOOK_ADDR (erase scrn, boot RAM, sony, …). */
#ifndef MAC_MMIO_HOOK_DEBUG
#define MAC_MMIO_HOOK_DEBUG 1
#endif
#if MAC_MMIO_HOOK_DEBUG
#define MAC_MMIO_HOOK_LOG(...)                                                                                                                                 \
	do {                                                                                                                                                   \
		printf(__VA_ARGS__);                                                                                                                           \
		fflush(stdout);                                                                                                                                \
	} while (0)
#else
#define MAC_MMIO_HOOK_LOG(...) ((void)0)
#endif

static FAST_DATA_ATTR macplus_t s_default_macplus = {};
static FAST_DATA_ATTR macplus_t *sim = &s_default_macplus;

int rom_patch(uint8_t *rom_base);
block_t *block_create_from_file(const char *path);
static int mac_msg_floppy_insert(macplus_t *sim, const char *msg, const char *val);
static int mac_msg_floppy_insert_ro(macplus_t *sim, const char *msg, const char *val);

void mac_init(macplus_t *instance)
{
	if (instance == NULL) {
		printf("mac_init: instance is NULL\n");
		abort();
	}
	memset(instance, 0, sizeof(*instance));
	instance->abs_mouse_ready = 0;
}

macplus_t *mac_new(void)
{
	macplus_t *instance = calloc(1, sizeof(*instance));
	if (instance == NULL) {
		return NULL;
	}
	mac_init(instance);
	return instance;
}

macplus_t *mac_get_instance(macplus_config_t config)
{
	if (!sim->booted) {
		mac_init(sim);
		macplus_boot(sim, &config);
	}
	return sim;
}

void mac_free(macplus_t *instance)
{
	if (instance == NULL) {
		return;
	}

	if (instance->kbd != NULL) {
		mac_kbd_del(instance->kbd);
		instance->kbd = NULL;
	}
	if (instance->booted) {
		mac_sound_free(&instance->sound);
		instance->booted = 0;
	}

	if (sim == instance) {
		sim = &s_default_macplus;
	}
	memset(instance, 0, sizeof(*instance));
}

macplus_t *macplus_instance(void)
{
	return sim;
}

#define MAC_MOUSE_ACCUM_MAX 30

void mac_set_mouse(macplus_t *sim, int dx, int dy, unsigned but)
{
	if (sim == NULL)
		return;

	sim->mouse_delta_x += dx;
	sim->mouse_delta_y += dy;

	if (sim->mouse_delta_x > MAC_MOUSE_ACCUM_MAX)
		sim->mouse_delta_x = MAC_MOUSE_ACCUM_MAX;
	if (sim->mouse_delta_y > MAC_MOUSE_ACCUM_MAX)
		sim->mouse_delta_y = MAC_MOUSE_ACCUM_MAX;
	if (sim->mouse_delta_x < -MAC_MOUSE_ACCUM_MAX)
		sim->mouse_delta_x = -MAC_MOUSE_ACCUM_MAX;
	if (sim->mouse_delta_y < -MAC_MOUSE_ACCUM_MAX)
		sim->mouse_delta_y = -MAC_MOUSE_ACCUM_MAX;

	sim->mouse_button = but;
}

static uint8_t *mem_ptr_at(unsigned int address)
{
	const struct memmap_ent *mm_ent = get_memmap_ent(address);
	if (mm_ent == NULL || mm_ent->mem_addr == NULL) {
		return NULL;
	}
	return &mm_ent->mem_addr[address & (MEMMAP_ES - 1)];
}

static uint16_t mem_read_u16_be(unsigned int address)
{
	uint8_t *p = mem_ptr_at(address);
	if (p == NULL) {
		return 0;
	}
	return (uint16_t)((uint16_t)p[0] << 8) | (uint16_t)p[1];
}

static void mem_write_u16_be(unsigned int address, uint16_t v)
{
	uint8_t *p = mem_ptr_at(address);
	if (p == NULL) {
		return;
	}
	p[0] = (uint8_t)(v >> 8);
	p[1] = (uint8_t)(v >> 0);
}

static void mem_write_u8(unsigned int address, uint8_t v)
{
	uint8_t *p = mem_ptr_at(address);
	if (p == NULL) {
		return;
	}
	p[0] = v;
}

void mac_set_mouse_abs(macplus_t *s, uint16_t x, uint16_t y, unsigned but)
{
	if (s == NULL) {
		return;
	}

	s->mouse_button = but;

	const unsigned int mtemp_y = 0x0828u;
	const unsigned int mtemp_x = 0x082Au;
	const unsigned int rawmouse_y = 0x082Cu;
	const unsigned int rawmouse_x = 0x082Eu;
	const unsigned int crsrnew = 0x08CEu;

	uint16_t old_x = mem_read_u16_be(rawmouse_x);
	uint16_t old_y = mem_read_u16_be(rawmouse_y);

	if (!s->abs_mouse_ready && (old_x != 15u || old_y != 15u)) {
		return;
	}
	s->abs_mouse_ready = 1;

	mem_write_u16_be(mtemp_x, x);
	mem_write_u16_be(mtemp_y, y);
	mem_write_u16_be(rawmouse_x, x);
	mem_write_u16_be(rawmouse_y, y);
	mem_write_u8(crsrnew, 1u);
}

void mac_get_mouse_pos(const macplus_t *s, uint16_t *x, uint16_t *y)
{
	if (s == NULL) {
		if (x) *x = 0;
		if (y) *y = 0;
		return;
	}
	const unsigned int rawmouse_y = 0x082Cu;
	const unsigned int rawmouse_x = 0x082Eu;
	if (x) *x = mem_read_u16_be(rawmouse_x);
	if (y) *y = mem_read_u16_be(rawmouse_y);
}

void mac_set_key(macplus_t *sim, unsigned event, pce_key_t key)
{
	if (sim == NULL || sim->kbd == NULL)
		return;
	mac_kbd_set_key(sim->kbd, event, key);
}

void mac_set_pause(macplus_t *s, int paused)
{
	if (s == NULL) {
		return;
	}
	s->paused = paused != 0;
}

int mac_get_pause(const macplus_t *s)
{
	if (s == NULL) {
		return 0;
	}
	return s->paused != 0;
}

/* ROM extension @ 0xF80000 — pointer/size from macplus_config_t (same pattern as main ROM). */
static uint8_t pcex_access_cb(unsigned int address, int data, int is_write)
{
	unsigned off = address - 0xF80000u;
	macplus_t *s = sim;

	(void)data;
	if (is_write) {
		return 0;
	}
	if (s->romex == NULL || off >= s->romex_size) {
		return 0xff;
	}
	return s->romex[off];
}

void m68k_instruction(void)
{
#if M68K_PC_SAMPLE_ENABLED
	unsigned pc = (unsigned)m68k_get_reg(NULL, M68K_REG_PC);
	static int hook_announced;

	if (!hook_announced) {
		hook_announced = 1;
		printf("m68k: instruction hook on (PC sample)\n");
		fflush(stdout);
	}
	m68k_pc_sample_at_pc(pc);
#endif
}

/*
 * Shared memory region at 0xF00000.
 * Written/read by 68k Mac apps through shm_region callbacks,
 * serviced by hook handlers registered via mac_register_hook().
 */
static uint8_t shm_region_cb(unsigned int address, int data, int is_write)
{
	macplus_t *s = sim;
	unsigned off = address - 0xF00000u;

	if (off >= sizeof(s->shm_region)) {
		return 0xff;
	}
	if (is_write) {
		s->shm_region[off] = (uint8_t)data;
		return 0;
	}
	return s->shm_region[off];
}

/* Set while mac_sony_hook runs (mem_get* → m68k_read_memory_* from host). */
static int mac_pce_in_sony_hook;

uint8_t unhandled_access_cb(unsigned int address, int data, int is_write)
{
	unsigned int pc = m68k_get_reg(NULL, M68K_REG_PC);
	const char *src = mac_pce_in_sony_hook ? " [from Sony hook mem]" : "";

	if (address >= 0xF00000u)
		return 0xffu;

	printf("Unhandled %s @ 0x%X! PC=0x%X%s\n", is_write ? "write" : "read", address, pc, src);
	fflush(stdout);
	return 0xff;
}

uint8_t bogus_read_cb(unsigned int address, int data, int is_write)
{
	if (is_write)
		return 0;
	return address ^ (address >> 8) ^ (address >> 16);
}

uint8_t scsi_access_cb(unsigned int address, int data, int is_write)
{
	unsigned dack = (address >> 9) & 1u;
	unsigned idx = (address >> 4) & 7u;
	unsigned long reg = (dack != 0 && idx == 0) ? 0x20ul : (unsigned long)idx;

	if (is_write) {
		mac_scsi_set_uint8(&sim->scsi, reg << 4, (unsigned char)data);
		return 0;
	}
	return mac_scsi_get_uint8(&sim->scsi, reg << 4);
}

static unsigned char mac_scc_get_uint8(unsigned long addr)
{
	unsigned char val = 0xff;
	unsigned int chn = (addr & (1 << 1)) ? 0 : 1;
	if (addr & (1 << 2)) {
		return e8530_get_data(&sim->scc, chn);
	} else {
		return e8530_get_ctl(&sim->scc, chn);
	}
	return val;
}

static void mac_scc_set_uint8(unsigned long addr, unsigned char val)
{
	unsigned int chn = (addr & (1 << 1)) ? 0 : 1;

	if (addr & (1 << 2)) {
		e8530_set_data(&sim->scc, chn, val);
	} else {
		e8530_set_ctl(&sim->scc, chn, val);
	}
}

uint8_t ssc_access_cb(unsigned int addr, int val, int is_write)
{
	if (is_write) {
		mac_scc_set_uint8(addr, val);
		return 0;
	} else {
		return mac_scc_get_uint8(addr);
	}
}

uint8_t iwm_access_cb(unsigned int address, int data, int is_write)
{
	if (is_write) {
		mac_iwm_set_uint8(&sim->iwm, (unsigned long)address, (unsigned char)data);
		return 0;
	}
	return mac_iwm_get_uint8(&sim->iwm, (unsigned long)address);
}

uint8_t via_access_cb(unsigned int address, int data, int is_write)
{
	if (is_write) {
		e6522_set_uint8(&sim->via, address & 0x1fff, data);
		return 0;
	}
	return e6522_get_uint8(&sim->via, address & 0x1fff);
}

static const struct memmap_handler_ent s_memmap_cb_map[] = {
        {.start_addr = 0x580000, .end_addr = 0x600000, .cb = scsi_access_cb},
        {.start_addr = 0x800000, .end_addr = 0xC00000, .cb = ssc_access_cb},
        {.start_addr = 0xC00000, .end_addr = 0xE00000, .cb = iwm_access_cb},
        {.start_addr = 0xE80000, .end_addr = 0xF00000, .cb = via_access_cb},
        {.start_addr = 0xF00000, .end_addr = 0xF20000, .cb = shm_region_cb},
        {.start_addr = 0xF80000, .end_addr = 0xFA0000, .cb = pcex_access_cb},
};

/* PCE mem.c mac_set_overlay — Mac Plus: VIA port A bit 4 toggles low memory map. */
void mac_set_overlay(macplus_t *s, int overlay)
{
	struct memmap_handlers handlers = {
	        .unhandled_cb = unhandled_access_cb,
	        .bogus_read_cb = bogus_read_cb,
	        .cb_map = s_memmap_cb_map,
	        .cb_map_count = sizeof(s_memmap_cb_map) / sizeof(s_memmap_cb_map[0]),
	};

	if (s->overlay == (overlay != 0)) {
		return;
	}
	s->overlay = (overlay != 0);
	memmap_rebuild_direct(s->overlay, s->rom, s->ram, MACPLUS_RAMSIZE / MEMMAP_ES, &handlers);
}

/* PCE macplus.c mac_set_vbuf — mac_video_set_vbuf; host sees framebuffer via frame_callback (clock.c). */
void mac_set_vbuf(macplus_t *s, uint8_t *vbuf)
{
	(void)s;
	(void)vbuf;
}

#define ENABLE_SKIP_MEMTEST_PATCH 1
#define MACPLUS_LOWMEM_MEMTOP_ADDR 0x000002AEu

#define MMAP_RAM_PTR(ent, addr) (&(ent)->mem_addr[(addr) & (MEMMAP_ES - 1)])
static void ram_init_from_config(macplus_t *s, const macplus_config_t *config)
{
	if (config == NULL) {
		printf("ram_init: config is NULL\n");
		abort();
	}
	for (int i = 0; i < MACPLUS_RAMSIZE / MEMMAP_ES; i++) {
		s->ram[i] = config->ram[i];
		if (s->ram[i] == NULL) {
			printf("ram_init: config->ram[%d] is NULL\n", i);
			abort();
		}
	}

	unsigned char *last_block = s->ram[MACPLUS_RAMSIZE / MEMMAP_ES - 1];
	(void)last_block;
}

/* PCE-style memtest skip: pre-seed low memory MemTop for Mac Plus ROM. */
static void mac_setup_mem(macplus_t *s)
{
#if ENABLE_SKIP_MEMTEST_PATCH
	const unsigned int addr = MACPLUS_LOWMEM_MEMTOP_ADDR;
	const unsigned int block = addr / MEMMAP_ES;
	const unsigned int off = addr & (MEMMAP_ES - 1);
	unsigned int memtop = MACPLUS_RAMSIZE;
	uint8_t *p;

	if (block >= (MACPLUS_RAMSIZE / MEMMAP_ES) || s->ram[block] == NULL) {
		return;
	}
	if (memtop > 0x00400000u) {
		memtop = 0x00400000u;
	}

	p = &s->ram[block][off];
	p[0] = (uint8_t)(memtop >> 24);
	p[1] = (uint8_t)(memtop >> 16);
	p[2] = (uint8_t)(memtop >> 8);
	p[3] = (uint8_t)(memtop >> 0);
#endif
}

/* P_EraseScrnBuff HLE: fill framebuffer with 0xFF. Return PC is still done by ROM jmp (a6). */
static int mac_erase_scrn_try_hook(void)
{
	unsigned a2 = m68k_get_reg(NULL, M68K_REG_A2) & 0xffffffu;
	unsigned a6 = m68k_get_reg(NULL, M68K_REG_A6) & 0xffffffu;
	unsigned n = (unsigned)SCREEN_SIZE;
	unsigned end = a2 + n;

	MAC_MMIO_HOOK_LOG("mac_erase_scrn_try_hook: A2=0x%06x A6=0x%06x n=%u\n", a2, a6, n);
	if (a2 >= end) {
		MAC_MMIO_HOOK_LOG("mac_erase_scrn_try_hook: skip (bad A2 range)\n");
		return 1;
	}
	for (unsigned p = a2; p < end; p++) {
		const struct memmap_ent *mm_ent = get_memmap_ent(p);

		if (!mm_ent->mem_addr) {
			MAC_MMIO_HOOK_LOG("mac_erase_scrn_try_hook: fail unmapped p=0x%06x\n", p);
			return 1;
		}
		*MMAP_RAM_PTR(mm_ent, p) = 0xffu;
	}
	MAC_MMIO_HOOK_LOG("mac_erase_scrn_try_hook: ok\n");
	VBUF_MARK_DIRTY(sim);
	return 0;
}

/* P_BootPart2 HLE: fill [A3, D0) with 0xFF; D0 is end address (listing: Sub.L A3,D0 before loop).
 * Within each MEMMAP_ES chunk host RAM is contiguous — memset per chunk, not per byte.
 */
static int mac_boot_part2_ram_try_hook(void)
{
	unsigned start = m68k_get_reg(NULL, M68K_REG_A3) & 0xffffffu;
	unsigned end = m68k_get_reg(NULL, M68K_REG_D0) & 0xffffffu;

	if (start >= end) {
		MAC_MMIO_HOOK_LOG("mac_boot_part2_ram_try_hook: skip empty range A3=0x%06x D0=0x%06x\n", start, end);
		return 0;
	}
	MAC_MMIO_HOOK_LOG("mac_boot_part2_ram_try_hook: A3=0x%06x D0=0x%06x (end)\n", start, end);
	while (start < end) {
		unsigned next = (start & ~(MEMMAP_ES - 1)) + MEMMAP_ES;
		unsigned chunk_end = next < end ? next : end;
		const struct memmap_ent *mm_ent = get_memmap_ent(start);

		if (!mm_ent->mem_addr) {
			MAC_MMIO_HOOK_LOG("mac_boot_part2_ram_try_hook: fail unmapped p=0x%06x\n", start);
			return 1;
		}
		memset(MMAP_RAM_PTR(mm_ent, start), 0xff, (size_t)(chunk_end - start));
		start = chunk_end;
	}
	MAC_MMIO_HOOK_LOG("mac_boot_part2_ram_try_hook: ok\n");
	return 0;
}

/* Guest MMIO byte write at MAC_MMIO_HOOK_ADDR (romex + ROM patches). Returns 0 if handled. */
static int mac_sony_try_hook(unsigned int hook_word)
{
	macplus_t *s = sim;
	if (!s->sony.enable) {
		return 1;
	}
	mac_sony_t *sony = &s->sony;
	int ok;

	sony->d0 = m68k_get_reg(NULL, M68K_REG_D0);
	sony->a0 = m68k_get_reg(NULL, M68K_REG_A0) & 0xFFFFFFu;
	sony->a1 = m68k_get_reg(NULL, M68K_REG_A1) & 0xFFFFFFu;
	sony->pc = m68k_get_reg(NULL, M68K_REG_PC);

	mac_pce_in_sony_hook = 1;
	ok = mac_sony_hook(sony, hook_word & 0xffffu);
	mac_pce_in_sony_hook = 0;
	if (ok == 0) {
		m68k_set_reg(M68K_REG_D0, sony->d0);
		m68k_set_reg(M68K_REG_A0, sony->a0);
		m68k_set_reg(M68K_REG_A1, sony->a1);
		m68k_set_reg(M68K_REG_PC, sony->pc);
		return 0;
	}
	return 1;
}

unsigned int FAST_FUNC_ATTR m68k_read_memory_8(unsigned int address)
{
	if ((address & 0xFFFFFFu) == (MAC_MMIO_HOOK_ADDR & 0xFFFFFFu))
		return 0xffu;
	const struct memmap_ent *mm_ent = get_memmap_ent(address);
	if (mm_ent->mem_addr) {
		uint8_t *p;
		p = (uint8_t *)MMAP_RAM_PTR(mm_ent, address);
		return *p;
	} else {
		return mm_ent->cb(address, 0, 0);
	}
}

unsigned int FAST_FUNC_ATTR m68k_read_memory_16(unsigned int address)
{
	const struct memmap_ent *mm_ent = get_memmap_ent(address);
	if ((address & 1) != 0)
		printf("%s: Unaligned access to %x!\n", __FUNCTION__, address);
	if (mm_ent->mem_addr) {
		uint16_t *p;
		p = (uint16_t *)MMAP_RAM_PTR(mm_ent, address);
		return __bswap_16(*p);
	} else {
		unsigned int ret;
		ret = mm_ent->cb(address, 0, 0) << 8;
		ret |= mm_ent->cb(address + 1, 0, 0);
		return ret;
	}
}

unsigned int FAST_FUNC_ATTR m68k_read_memory_32(unsigned int address)
{
	if ((address & 0x3) == 0) {
		const struct memmap_ent *mm_ent = get_memmap_ent(address);
		if (mm_ent->mem_addr) {
			uint16_t *p;
			p = (uint16_t *)MMAP_RAM_PTR(mm_ent, address);
			unsigned int hi = __bswap_16(*p++);
			unsigned int lo = __bswap_16(*p);
			return (hi << 16) | lo;
		} else {
			unsigned int ret;
			ret = mm_ent->cb(address, 0, 0) << 8;
			ret |= mm_ent->cb(address + 1, 0, 0);
			unsigned int ret2;
			ret2 = mm_ent->cb(address + 2, 0, 0) << 8;
			ret2 |= mm_ent->cb(address + 3, 0, 0);
			return (ret << 16) | ret2;
		}
	} else {
		uint16_t a = m68k_read_memory_16(address);
		uint16_t b = m68k_read_memory_16(address + 2);
		return (a << 16) | b;
	}
}

void mac_register_hook(macplus_t *sim, uint8_t value, mac_mmio_hook_fn_t handler)
{
	if (sim == NULL || handler == NULL) {
		return;
	}
	if (sim->mmio_hook_count >= 8) {
		printf("mac_register_hook: table full (max 8)\n");
		return;
	}
	sim->mmio_hooks[sim->mmio_hook_count].value   = value;
	sim->mmio_hooks[sim->mmio_hook_count].handler  = handler;
	sim->mmio_hook_count++;
}

void FAST_FUNC_ATTR m68k_write_memory_8(unsigned int address, unsigned int value)
{
	if ((address & 0xFFFFFFu) == (MAC_MMIO_HOOK_ADDR & 0xFFFFFFu)) {
		if ((value & 0xffu) == MAC_HOOK_ERASE_SCRN) {
			if (mac_erase_scrn_try_hook() == 0)
				return;
		}
		if ((value & 0xffu) == MAC_HOOK_BOOT_PART2_RAM) {
			if (mac_boot_part2_ram_try_hook() == 0)
				return;
		}
		if (mac_sony_try_hook(value & 0xffffu) == 0)
			return;

		/* Dispatch registered hooks */
		for (int i = 0; i < sim->mmio_hook_count; i++) {
			if ((value & 0xffu) == sim->mmio_hooks[i].value) {
				sim->mmio_hooks[i].handler(sim, value);
				return;
			}
		}
	}
	const struct memmap_ent *mm_ent = get_memmap_ent(address);
	if (mm_ent->mem_addr) {
		uint8_t *p;
		p = (uint8_t *)MMAP_RAM_PTR(mm_ent, address);
		*p = value;
		if (address >= MACPLUS_SCREENBUF &&
		    address < MACPLUS_SCREENBUF + SCREEN_SIZE)
			VBUF_MARK_DIRTY(sim);
	} else {
		mm_ent->cb(address, value, 1);
	}
}

void FAST_FUNC_ATTR m68k_write_memory_16(unsigned int address, unsigned int value)
{
	const struct memmap_ent *mm_ent = get_memmap_ent(address);
	if ((address & 1) != 0)
		printf("%s: Unaligned access to %x!\n", __FUNCTION__, address);
	if (mm_ent->mem_addr) {
		uint16_t *p;
		p = (uint16_t *)MMAP_RAM_PTR(mm_ent, address);
		*p = __bswap_16(value);
		if (address > MACPLUS_SCREENBUF - 2 &&
		    address < MACPLUS_SCREENBUF + SCREEN_SIZE)
			VBUF_MARK_DIRTY(sim);
	} else {
		mm_ent->cb(address, (value >> 8) & 0xff, 1);
		mm_ent->cb(address + 1, (value >> 0) & 0xff, 1);
	}
}

void FAST_FUNC_ATTR m68k_write_memory_32(unsigned int address, unsigned int value)
{

	if ((address & 0x3) == 0) {
		const struct memmap_ent *mm_ent = get_memmap_ent(address);
		if (mm_ent->mem_addr) {
			uint16_t *p = (uint16_t *)MMAP_RAM_PTR(mm_ent, address);
			*p++ = __bswap_16(value >> 16);
			*p = __bswap_16(value);
			if (address > MACPLUS_SCREENBUF - 4 &&
			    address < MACPLUS_SCREENBUF + SCREEN_SIZE)
				VBUF_MARK_DIRTY(sim);
		} else {
			mm_ent->cb(address, (value >> 24) & 0xff, 1);
			mm_ent->cb(address + 1, (value >> 16) & 0xff, 1);
			mm_ent->cb(address + 2, (value >> 8) & 0xff, 1);
			mm_ent->cb(address + 3, (value >> 0) & 0xff, 1);
		}
	} else {
		m68k_write_memory_16(address, value >> 16);
		m68k_write_memory_16(address + 2, value);
	}
}

unsigned char *m68k_pc_base = NULL;

void m68k_pc_changed_handler_function(unsigned int address)
{
	const struct memmap_ent *mm_ent = get_memmap_ent(address);
	if (mm_ent->mem_addr) {
		uint8_t *p;
		p = (uint8_t *)MMAP_RAM_PTR(mm_ent, address);
		m68k_pc_base = p - address;
	} else {
		printf("PC not in mem!\n");
		abort();
	}
}
static void mac_hw_init(macplus_t *s)
{
	int i;

	s->irq_bits = 0;
	s->dcd_a = 0;
	s->dcd_b = 0;
	s->speed_factor = 1;
	for (i = 0; i < 4; i++) {
		s->clk_div[i] = 0;
	}
	s->scc_clk_phase = 0;
}

void mac_reset(macplus_t *s)
{
	int i;

	mac_sony_reset(&s->sony);

	for (i = 0; i < 4; i++) {
		s->clk_div[i] = 0;
	}
	s->scc_clk_phase = 0;

	s->intr_scsi_via = 0;
	mac_irq_reset(s);
	mac_scsi_reset(&s->scsi);
	/* PCE macplus.c mac_reset: Mac Plus starts with overlay on. */
	mac_set_overlay(s, 1);
	e6522_reset(&s->via);
	s->via_port_a = 0xf7;
	s->via_port_b = 0xff;
	e6522_set_ira_inp(&s->via, s->via_port_a);
	e6522_set_irb_inp(&s->via, s->via_port_b);
}

static void mac_setup_kbd(macplus_t *s)
{
	s->kbd = mac_kbd_new();
	if (s->kbd == NULL) {
		printf("mac_kbd_new failed\n");
		abort();
	}
	mac_kbd_set_model(s->kbd, 3, 0);
	mac_kbd_set_keypad_mode(s->kbd, 0);
	mac_kbd_set_data_fct(s->kbd, &s->via, e6522_set_shift_inp);
	e6522_set_shift_out_fct(&s->via, s->kbd, mac_kbd_set_uint8);
	e6522_set_cb2_fct(&s->via, s->kbd, mac_kbd_set_data);
}

/* PCE: macplus.c mac_setup_rtc */
static void mac_setup_rtc(macplus_t *s)
{
	mac_rtc_init(&s->rtc);
	mac_rtc_set_data_fct(&s->rtc, s, mac_set_rtc_data);
	mac_rtc_set_osi_fct(&s->rtc, s, mac_interrupt_osi);
	mac_rtc_set_realtime(&s->rtc, 1);
}

static void mac_setup_via(macplus_t *s)
{
	e6522_init(&s->via, 9);
	e6522_set_irq_fct(&s->via, s, mac_interrupt_via);
	e6522_set_ora_fct(&s->via, s, mac_set_via_port_a);
	e6522_set_orb_fct(&s->via, s, mac_set_via_port_b);
}

static void mac_setup_scc(macplus_t *s)
{
	e8530_init(&s->scc);
	e8530_set_irq_fct(&s->scc, s, mac_interrupt_scc);
	e8530_set_clock(&s->scc, 3672000, 3672000, 3672000);
}

static void mac_setup_sony(macplus_t *s, const macplus_config_t *config)
{
	mac_sony_init(&s->sony, 1);
	mac_sony_set_eject_callback(&s->sony, config->floppy_eject_callback, config->floppy_eject_callback_ctx);
	mac_sony_set_disk(&s->sony, 1, config->fd);
	if (config->fd != NULL)
		mac_sony_set_delay(&s->sony, 0, 1);
}

static void mac_setup_iwm(macplus_t *s)
{
	mac_iwm_init(&s->iwm);
	mac_iwm_set_head_sel(&s->iwm, (unsigned char)(s->via_port_a & (1 << 5)));
	/* Restored floppy: IWM in-place signal matches SONY (present, not a
	 * disk-change event — switched untouched). */
	if (s->sony.disk[1] != NULL)
		mac_iwm_insert(&s->iwm, 0);
}

static void mac_setup_scsi(macplus_t *s, const macplus_config_t *config)
{
	mac_scsi_init(&s->scsi);
	/* Register all provided hard disk images on SCSI IDs 0..7 */
	printf("Creating HDs and registering them...\n");
	for (int scsi_id = 0; scsi_id < 8; scsi_id++) {
		if (config->hd[scsi_id] != NULL) {
			mac_scsi_register_device(&s->scsi, scsi_id, config->hd[scsi_id]);
			printf("  Registered SCSI ID %d\n", scsi_id);
		}
	}
	mac_scsi_set_int_fct(&s->scsi, s, mac_interrupt_scsi);
}

static void mac_setup_sound(macplus_t *s, const macplus_config_t *config)
{
	unsigned char *last_block = s->ram[MACPLUS_RAMSIZE / MEMMAP_ES - 1];
	s->sbuf1 = &last_block[MACPLUS_SNDBUF_OFFSET];
	s->sbuf2 = &last_block[MACPLUS_SNDBUF_ALT_OFFSET];

	if (mac_sound_init(&s->sound, config->sound) != 0) {
		printf("macplus: mac_sound_init failed\n");
		abort();
	}
	mac_sound_set_sbuf(&s->sound, (s->via_port_a & 0x08) ? s->sbuf1 : s->sbuf2);
	mac_sound_set_volume(&s->sound, (unsigned)(s->via_port_a & 7));
	mac_sound_set_enable(&s->sound, (s->via_port_b & 0x80) == 0);
	mac_sound_set_lowpass(&s->sound, 8000);
}

static void mac_setup_video(macplus_t *s)
{
	unsigned char *last_block = s->ram[MACPLUS_RAMSIZE / MEMMAP_ES - 1];
	s->vbuf1 = &last_block[MACPLUS_SCREENBUF_OFFSET];
	s->vbuf2 = &last_block[MACPLUS_SCREENBUF_ALT_OFFSET];
	mac_set_vbuf(s, (s->via_port_a & 0x40) ? s->vbuf1 : s->vbuf2);
}
void macplus_boot(macplus_t *s, const macplus_config_t *config)
{
	block_t *fd = NULL;
	void *rom = NULL;

	assert(s != NULL);
	assert(config != NULL);

	sim = s;
	sim->frame_callback = config->frame_callback;
	sim->frame_callback_ctx = config->frame_callback_ctx;
	fd = config->fd;
	rom = config->rom;

	assert(config->rom != NULL);
	assert(config->romex != NULL);
	assert(config->romex_size != 0);
	assert(config->sound != NULL);

	/* --- ROM / memory --- */
	s->romex = config->romex;
	s->romex_size = config->romex_size;
	rom_patch((uint8_t *)rom);
	s->rom = (uint8_t *)rom;
	ram_init_from_config(s, config);
	mac_setup_mem(s);
	mac_set_overlay(s, 1);

	/* --- Peripherals (PCE order: via→scc→rtc→kbd→iwm→scsi→sony→sound) --- */
	mac_setup_via(s);
	mac_setup_scc(s);
	mac_setup_rtc(s);
	mac_setup_kbd(s);
	mac_setup_iwm(s);
	mac_setup_scsi(s, config);
	mac_setup_sony(s, config);
	mac_setup_sound(s, config);
	mac_setup_video(s);

	/* --- Hardware state, reset, SONY ROM patch (patch after reset) --- */
	mac_hw_init(s);
	mac_reset(s);
	mac_sony_patch(&s->sony);

	/* Register core commands for cross-thread dispatch (PCE msg style). */
	mac_msg_register("floppy.insert", mac_msg_floppy_insert);
	mac_msg_register("floppy.insert.ro", mac_msg_floppy_insert_ro);

	/* --- CPU --- */
	printf("Initializing m68k...\n");
	m68k_pc_sample_reset();
	m68k_pc_changed_handler_function(0x0);
	m68k_init();
	printf("Setting CPU type and resetting...");
	m68k_set_cpu_type(M68K_CPU_TYPE_68000);
	m68k_pulse_reset();

	/* --- Clock, done --- */
	mac_clock_sched_init(&s->clock_sched);
	s->booted = 1;
	printf("Done! Boot completed.\n");
}

void macplus_run_frame(macplus_t *s)
{
	mac_clock_run_frame(s, &s->clock_sched, M68K_CYCLES_PER_FRAME, M68K_CPU_STEP);
}


int mac_floppy_insert(macplus_t *s, const char *path, bool delayed, bool ro)
{
	if (s == NULL || path == NULL || path[0] == '\0') {
		printf("floppy: no image path\n");
		return 0;
	}

	block_t *new_blk = block_create_from_file(path);
	if (new_blk == NULL) {
		printf("floppy: image open failed: %s\n", path);
		return 0;
	}

	/* Free the previous block (manual or boot-restored) — single owner. */
	if (s->floppy_block != NULL && s->floppy_block != new_blk)
		delete (s->floppy_block);

	new_blk->readonly = ro ? 1 : 0;

	s->sony.enable = 1;
	mac_sony_set_disk(&s->sony, 1, new_blk);
	mac_iwm_insert(&s->iwm, 0);
	if (delayed) {
		/* Boot-restored floppy: let the guest trigger the insert event on
		 * its next drive check (DISKINPLACE 0->1 edge), like the old
		 * config.fd restore did — a static 1 at boot is not mounted. */
		mac_sony_set_delay(&s->sony, 0, 1);
	} else {
		mac_sony_insert(&s->sony, 1);
	}
	s->floppy_block = new_blk;
	return 1;
}

block_t *mac_floppy_get(macplus_t *s)
{
	return (s != NULL) ? s->floppy_block : NULL;
}

/* floppy.insert: insert an uploaded floppy on the emulator thread
 * (dispatched from the emulator loop — no cross-thread writes). */
static int mac_msg_floppy_insert(macplus_t *sim, const char *msg, const char *val)
{
	(void)msg;
	if (!mac_floppy_insert(sim, val, false, false))
		printf("floppy: msg insert failed: %s\n", val);
	return 0;
}

/* floppy.insert.ro: insert an uploaded picture disk read-only. */
static int mac_msg_floppy_insert_ro(macplus_t *sim, const char *msg, const char *val)
{
	(void)msg;
	if (!mac_floppy_insert(sim, val, false, true))
		printf("floppy: msg insert.ro failed: %s\n", val);
	return 0;
}
