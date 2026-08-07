/*
 * Sony floppy — block I/O path from PCE sony.c (raw LBA .img / .dsk).
 * PSI/GCR image paths omitted; tags zeroed for HFS.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "sony.h"
#include "m68k.h"

#define MAC_HOOK_SONY 16
#define MAC_HOOK_SONY_OPEN (MAC_HOOK_SONY + 0)
#define MAC_HOOK_SONY_PRIME (MAC_HOOK_SONY + 1)
#define MAC_HOOK_SONY_CTRL (MAC_HOOK_SONY + 2)
#define MAC_HOOK_SONY_STATUS (MAC_HOOK_SONY + 3)
#define MAC_HOOK_SONY_CLOSE (MAC_HOOK_SONY + 4)

#define SONY_TRACK 0
#define SONY_WPROT 2
#define SONY_DISKINPLACE 3
#define SONY_INSTALLED 4
#define SONY_SIDES 5
#define SONY_QLINK 6
#define SONY_QTYPE 10
#define SONY_QDRIVENO 12
#define SONY_QREFNUM 14
#define SONY_QFSID 16
#define SONY_TWOSIDEFMT 18
#define SONY_NEWIF 19
#define SONY_DRIVEERRS 20

#define qLink 0
#define qType 4
#define ioTrap 6
#define ioCmdAddr 8
#define ioCompletion 12
#define ioResult 16
#define ioNamePtr 18
#define ioVRefNum 22
#define ioRefNum 24
#define ioVersNum 26
#define ioPermssn 27
#define ioMisc 28
#define ioBuffer 32
#define ioReqCount 36
#define ioActCount 40
#define ioPosMode 44
#define ioPosOffset 46
#define csCode 26
#define csParam 28

#define dCtlDriver 0
#define dCtlFlags 4
#define dCtlQHdr 6
#define dCtlPosition 16
#define dCtlStorage 20
#define dCtlRefNum 24
#define dCtlCurTicks 26

#define fsAtMark 0
#define fsFromStart 1
#define fsFromMark 3

#define asyncTrpBit 0x0100
#define noQueueBit 0x0200

#define noErr 0
#define controlErr -17
#define statusErr -18
#define readErr -19
#define writeErr -20
#define abortErr -27
#define wPrErr -44
#define paramErr -50
#define nsDrvErr -56
#define noDriveErr -64
#define offLineErr -65

/* Mac Plus 68000: 24-bit physical address; mask off bogus high byte (e.g. A1=0x800017A2). */
static inline unsigned int mac_addr24(unsigned int a)
{
	return a & 0xFFFFFFu;
}

static unsigned int mem_get8(unsigned int a)
{
	return m68k_read_memory_8(mac_addr24(a));
}

/* Big-endian host view of Mac memory; any address (mac_sony_find steps by bytes). */
static unsigned int mem_get16(unsigned int a)
{
	return (m68k_read_memory_8(mac_addr24(a)) << 8) | m68k_read_memory_8(mac_addr24(a + 1));
}

static unsigned int mem_get32(unsigned int a)
{
	return ((unsigned)m68k_read_memory_8(mac_addr24(a)) << 24) | ((unsigned)m68k_read_memory_8(mac_addr24(a + 1)) << 16) |
	       ((unsigned)m68k_read_memory_8(mac_addr24(a + 2)) << 8) | (unsigned)m68k_read_memory_8(mac_addr24(a + 3));
}

static void mem_set8(unsigned int a, unsigned int v)
{
	m68k_write_memory_8(mac_addr24(a), v);
}

static void mem_set16(unsigned int a, unsigned int v)
{
	m68k_write_memory_16(mac_addr24(a), v);
}

static void mem_set32(unsigned int a, unsigned int v)
{
	m68k_write_memory_32(mac_addr24(a), v);
}

void mac_sony_init(mac_sony_t *sony, int enable)
{
	unsigned i;

	sony->open = 0;
	sony->patched = 0;
	sony->enable = (enable != 0);

	for (i = 0; i <= SONY_DRIVES; i++) {
		sony->disk[i] = NULL;
	}

	sony->check_addr = 0;
	sony->icon_addr[0] = 0;
	sony->icon_addr[1] = 0;

	sony->tag_buf = 0;

	sony->format_hd_as_dd = 0;
	sony->format_cnt = 0;

	for (i = 0; i < SONY_DRIVES; i++) {
		sony->delay_val[i] = 0;
		sony->delay_cnt[i] = 0;
	}
	sony->on_eject = NULL;
	sony->on_eject_ctx = NULL;
}

void mac_sony_set_disk(mac_sony_t *sony, unsigned drive, block_t *d)
{
	if (drive <= SONY_DRIVES) {
		sony->disk[drive] = d;
	}
}

static unsigned long mac_sony_get_vars(mac_sony_t *sony, unsigned drive)
{
	unsigned long ret;

	ret = mem_get32(0x0134);

	if ((drive >= 1) && (drive <= SONY_DRIVES)) {
		ret += 8 + 66 * drive;
	}

	(void)sony;
	return ret;
}

static unsigned long mac_sony_find_pcex_at(mac_sony_t *sony, unsigned long addr)
{
	unsigned long cnt;

	(void)sony;

	if (mem_get32(addr) != 0x50434558) {
		return 0;
	}

	cnt = mem_get32(addr + 8);

	if (cnt < 4) {
		return 0;
	}

	return addr;
}

static unsigned long mac_sony_find_pcex(mac_sony_t *sony, unsigned long addr)
{
	unsigned long base;

	base = mac_sony_find_pcex_at(sony, addr);
	if (base == 0) {
		return 0;
	}

	sony->check_addr = base + mem_get32(base + 16);
	sony->icon_addr[0] = base + mem_get32(base + 20);
	sony->icon_addr[1] = base + mem_get32(base + 24);

	addr = base + mem_get32(base + 12);

	return addr;
}

static unsigned long mac_sony_find(mac_sony_t *sony, unsigned long addr, unsigned long size)
{
	unsigned long sony_addr;

	(void)sony;

	while (size > 0) {
		addr += 1;
		size -= 1;

		if (mem_get16(addr - 1) != 0x052e) {
			continue;
		}

		if (mem_get32(addr + 1) != 0x536f6e79) {
			continue;
		}

		sony_addr = addr - 19;

		if (mem_get16(sony_addr) != 0x4f00) {
			continue;
		}

		return sony_addr;
	}

	return 0;
}

static void mac_sony_unpatch_rom(mac_sony_t *sony)
{
	unsigned i, j;
	unsigned sofs;
	unsigned long sadr;
	unsigned char *buf;

	if (sony->patched == 0) {
		return;
	}

	if ((sony->sony_addr == 0) || (sony->pcex_addr == 0)) {
		return;
	}

	buf = sony->patch_buf;

	for (i = 0; i < 5; i++) {
		sofs = mem_get16(sony->sony_addr + 8 + 2 * i);
		sadr = sony->sony_addr + sofs;

		for (j = 0; j < 6; j++) {
			mem_set8(sadr + j, *(buf++));
		}
	}

	sony->patched = 0;
}

static void mac_sony_patch_rom(mac_sony_t *sony)
{
	unsigned i, j;
	unsigned sofs, dofs;
	unsigned long sadr, dadr;
	unsigned char *buf;

	if ((sony->sony_addr == 0) || (sony->pcex_addr == 0)) {
		return;
	}

	buf = sony->patch_buf;

	for (i = 0; i < 5; i++) {
		sofs = mem_get16(sony->sony_addr + 8 + 2 * i);
		sadr = sony->sony_addr + sofs;

		dofs = mem_get16(sony->pcex_addr + 8 + 2 * i);
		dadr = sony->pcex_addr + dofs;

		for (j = 0; j < 6; j++) {
			*(buf++) = (unsigned char)mem_get8(sadr + j);
		}

		mem_set8(sadr + 0, 0x4e);
		mem_set8(sadr + 1, 0xf9);
		mem_set8(sadr + 2, (dadr >> 24) & 0xff);
		mem_set8(sadr + 3, (dadr >> 16) & 0xff);
		mem_set8(sadr + 4, (dadr >> 8) & 0xff);
		mem_set8(sadr + 5, dadr & 0xff);
	}
}

void mac_sony_patch(mac_sony_t *sony)
{
	unsigned long pcex;

	if (sony->enable == 0) {
		return;
	}

	if (sony->patched) {
		return;
	}

	sony->patched = 1;

	sony->pcex_addr = 0;
	sony->sony_addr = 0;

	pcex = mac_sony_find_pcex(sony, 0xf80000);
	if (pcex == 0) {
		pcex = mac_sony_find_pcex(sony, 0x400000);
	}

	if (pcex == 0) {
		printf("SONY: PCE ROM extension (PCEX) not found\n");
		sony->patched = 0;
		return;
	}

	sony->pcex_addr = pcex;
	printf("SONY: PCE ROM extension at 0x%06lx\n", (unsigned long)sony->pcex_addr);

	sony->sony_addr = mac_sony_find(sony, 0x400000, 1024UL * 1024UL);

	if (sony->sony_addr == 0) {
		printf("SONY: Sony driver signature not found in ROM\n");
		sony->patched = 0;
		return;
	}

	printf("SONY: Sony driver at 0x%06lx\n", (unsigned long)sony->sony_addr);

	mac_sony_patch_rom(sony);
}

void mac_sony_set_delay(mac_sony_t *sony, unsigned drive, unsigned delay)
{
	if (drive < SONY_DRIVES) {
		sony->delay_val[drive] = delay;
		sony->delay_cnt[drive] = delay;
	}
}

void mac_sony_insert(mac_sony_t *sony, unsigned drive)
{
	unsigned long vars;
	block_t *dsk;

	if (!sony->enable) {
		return;
	}

	if ((drive < 1) || (drive > SONY_DRIVES)) {
		return;
	}

	dsk = sony->disk[drive];

	if (dsk == NULL) {
		return;
	}

	vars = mac_sony_get_vars(sony, drive);

	if (mem_get8(vars + SONY_DISKINPLACE) == 0x00) {
		printf("SONY: insert drive %u\n", drive);

		mem_set8(vars + SONY_DISKINPLACE, 0x01);

		uint64_t cap = dsk->capacity ? dsk->capacity(dsk) : 0;
		unsigned long blocks = (unsigned long)(cap / 512ULL);
		if (blocks < 1600) {
			mem_set8(vars + SONY_TWOSIDEFMT, 0x00);
		} else {
			mem_set8(vars + SONY_TWOSIDEFMT, 0xff);
		}

		mem_set8(vars + SONY_NEWIF, 0xff);

		if (dsk->readonly) {
			mem_set8(vars + SONY_WPROT, 0xff);
		} else {
			mem_set8(vars + SONY_WPROT, 0x00);
		}
	}
}

void mac_sony_set_eject_callback(mac_sony_t *sony, void (*on_eject)(unsigned drive, void *ctx), void *ctx)
{
	if (sony == NULL) {
		return;
	}
	sony->on_eject = on_eject;
	sony->on_eject_ctx = ctx;
}

int mac_sony_disk_in_place(mac_sony_t *sony, unsigned drive)
{
	if (sony == NULL || !sony->enable) {
		return 0;
	}
	if ((drive < 1) || (drive > SONY_DRIVES)) {
		return 0;
	}
	if (sony->disk[drive] == NULL) {
		return 0;
	}
	unsigned long vars = mac_sony_get_vars(sony, drive);
	return (mem_get8(vars + SONY_DISKINPLACE) != 0) ? 1 : 0;
}

int mac_sony_check(mac_sony_t *sony)
{
	int check;
	unsigned i;
	unsigned long vars;
	block_t *dsk;

	if (sony->open == 0) {
		return 0;
	}

	check = 0;

	for (i = 0; i < SONY_DRIVES; i++) {
		if (sony->delay_cnt[i] > 0) {
			sony->delay_cnt[i] -= 1;

			if (sony->delay_cnt[i] == 0) {
				mac_sony_insert(sony, i + 1);
			}
		}

		dsk = sony->disk[i + 1];

		if (dsk != NULL) {
			vars = mac_sony_get_vars(sony, i + 1);

			if (mem_get8(vars + SONY_DISKINPLACE) == 0x01) {
				check = 1;
			}
		}
	}

	return check;
}

static void mac_sony_open(mac_sony_t *sony)
{
	sony->open = 1;

	if (mac_sony_check(sony)) {
		sony->pc = sony->check_addr;
	}
}

static unsigned long mac_sony_get_pblk(mac_sony_t *sony, unsigned ofs, unsigned size)
{
	if (size == 1) {
		return mem_get8(sony->a0 + ofs);
	} else if (size == 2) {
		return mem_get16(sony->a0 + ofs);
	} else if (size == 4) {
		return mem_get32(sony->a0 + ofs);
	}

	return 0;
}

static void mac_sony_set_pblk(mac_sony_t *sony, unsigned ofs, unsigned size, unsigned long val)
{
	if (size == 1) {
		mem_set8(sony->a0 + ofs, val);
	} else if (size == 2) {
		mem_set16(sony->a0 + ofs, val);
	} else if (size == 4) {
		mem_set32(sony->a0 + ofs, val);
	}
}

static unsigned long mac_sony_get_dctl(mac_sony_t *sony, unsigned ofs, unsigned size)
{
	if (size == 1) {
		return mem_get8(sony->a1 + ofs);
	} else if (size == 2) {
		return mem_get16(sony->a1 + ofs);
	} else if (size == 4) {
		return mem_get32(sony->a1 + ofs);
	}

	return 0;
}

static void mac_sony_set_dctl(mac_sony_t *sony, unsigned ofs, unsigned size, unsigned long val)
{
	if (size == 1) {
		mem_set8(sony->a1 + ofs, val);
	} else if (size == 2) {
		mem_set16(sony->a1 + ofs, val);
	} else if (size == 4) {
		mem_set32(sony->a1 + ofs, val);
	}
}

static void mac_sony_return(mac_sony_t *sony, unsigned res, int rts)
{
	unsigned trap;
	unsigned long val;

	sony->d0 = (res & 0x8000) ? (0xffff0000 | res) : 0;

	trap = (unsigned)mac_sony_get_pblk(sony, ioTrap, 2);

	mac_sony_set_pblk(sony, ioResult, 2, res);

	if ((rts == 0) && ((trap & noQueueBit) == 0)) {
		val = mem_get32(0x0134);
		val = mem_get32(val);

		sony->a1 = val;
		sony->pc = mem_get32(0x08fc);
	}
}

static unsigned long mac_sony_get_offset(mac_sony_t *sony)
{
	unsigned posmode;
	unsigned long ofs;

	posmode = (unsigned)mac_sony_get_pblk(sony, ioPosMode, 2);

	switch (posmode & 0x0f) {
	case fsAtMark:
		ofs = mac_sony_get_dctl(sony, dCtlPosition, 4);
		break;

	case fsFromStart:
		ofs = mac_sony_get_pblk(sony, ioPosOffset, 4);
		break;

	case fsFromMark:
		ofs = mac_sony_get_pblk(sony, ioPosOffset, 4);
		ofs += mac_sony_get_dctl(sony, dCtlPosition, 4);
		break;

	default:
		return 0;
	}

	return ofs;
}

static int mac_sony_read_block(block_t *dsk, void *buf, void *tag, unsigned long idx)
{
	memset(tag, 0, 12);

	if (dsk->read == NULL || dsk->read(dsk, (uint8_t *)buf, (uint64_t)idx * 512ULL, 512) != 512) {
		return 1;
	}

	return 0;
}

static int mac_sony_write_block(block_t *dsk, const void *buf, const void *tag, unsigned long idx)
{
	(void)tag;

	if (dsk->write == NULL) {
		return 1;
	}
	if (dsk->write(dsk, (const uint8_t *)buf, (uint64_t)idx * 512ULL, 512) != 512) {
		return 1;
	}
	if (dsk->sync != NULL) {
		dsk->sync(dsk);
	}

	return 0;
}

static void mac_sony_prime_read(mac_sony_t *sony, unsigned drive)
{
	unsigned long addr, vars;
	unsigned long ofs, cnt;
	unsigned long i, n;
	unsigned j;
	block_t *dsk;
	unsigned char buf[512];
	unsigned char tag[12];
	unsigned posmode;

	ofs = mac_sony_get_offset(sony);
	cnt = mac_sony_get_pblk(sony, ioReqCount, 4);
	addr = mac_sony_get_pblk(sony, ioBuffer, 4) & 0x00ffffff;
	posmode = (unsigned)mac_sony_get_pblk(sony, ioPosMode, 2);

	dsk = sony->disk[drive];

	if (dsk == NULL) {
		mac_sony_return(sony, offLineErr, 0);
		return;
	}

	if (posmode & 0x40) {
		mac_sony_return(sony, noErr, 0);
		return;
	}

	if ((ofs & 511) || (cnt & 511)) {
		mac_sony_return(sony, paramErr, 0);
		return;
	}

	n = cnt / 512;

	for (i = 0; i < n; i++) {
		if (mac_sony_read_block(dsk, buf, tag, (ofs / 512) + i)) {
			mac_sony_return(sony, 0xffff, 0);
			return;
		}

		for (j = 0; j < 512; j++) {
			mem_set8((unsigned int)(addr + 512 * i + j), buf[j]);
		}

		for (j = 0; j < 12; j++) {
			mem_set8(0x2fc + j, tag[j]);
		}

		if (sony->tag_buf != 0) {
			for (j = 0; j < 12; j++) {
				mem_set8((unsigned int)(sony->tag_buf + 12 * i + j), tag[j]);
			}
		}
	}

	vars = mac_sony_get_vars(sony, drive);
	mem_set8(vars + SONY_DISKINPLACE, 0x02);

	mac_sony_set_pblk(sony, ioActCount, 4, cnt);

	ofs = mac_sony_get_dctl(sony, dCtlPosition, 4);
	mac_sony_set_dctl(sony, dCtlPosition, 4, ofs + cnt);

	mac_sony_return(sony, noErr, 0);
}

static void mac_sony_prime_write(mac_sony_t *sony, unsigned drive)
{
	unsigned long addr;
	unsigned long ofs, cnt;
	unsigned relblk;
	unsigned long i, n;
	unsigned j;
	block_t *dsk;
	unsigned char buf[512];
	unsigned char tag[12];

	ofs = mac_sony_get_offset(sony);
	cnt = mac_sony_get_pblk(sony, ioReqCount, 4);
	addr = mac_sony_get_pblk(sony, ioBuffer, 4) & 0x00ffffff;

	dsk = sony->disk[drive];

	if (dsk == NULL) {
		mac_sony_return(sony, offLineErr, 0);
		return;
	}

	if (dsk->write == NULL) {
		mac_sony_return(sony, wPrErr, 0);
		return;
	}

	if ((cnt & 511) || (ofs & 511)) {
		mac_sony_return(sony, paramErr, 0);
		return;
	}

	memset(tag, 0, 12);

	relblk = (unsigned)mem_get16(0x302);

	n = cnt / 512;

	for (i = 0; i < n; i++) {
		for (j = 0; j < 512; j++) {
			buf[j] = (unsigned char)mem_get8((unsigned int)(addr + 512 * i + j));
		}

		if (sony->tag_buf != 0) {
			for (j = 0; j < 12; j++) {
				tag[j] = (unsigned char)mem_get8((unsigned int)(sony->tag_buf + 12 * i + j));
				mem_set8(0x2fc + j, tag[j]);
			}
		} else {
			mem_set16(0x302, relblk + (unsigned)i);

			for (j = 0; j < 12; j++) {
				tag[j] = (unsigned char)mem_get8(0x2fc + j);
			}
		}

		if (mac_sony_write_block(dsk, buf, tag, (ofs / 512) + i)) {
			mac_sony_return(sony, 0xffff, 0);
			return;
		}
	}

	mac_sony_set_pblk(sony, ioActCount, 4, cnt);

	ofs = mac_sony_get_dctl(sony, dCtlPosition, 4);
	mac_sony_set_dctl(sony, dCtlPosition, 4, ofs + cnt);

	mac_sony_return(sony, noErr, 0);
}

static void mac_sony_prime(mac_sony_t *sony)
{
	unsigned long vars;
	unsigned trap, vref;

	trap = (unsigned)mac_sony_get_pblk(sony, ioTrap, 2);
	vref = (unsigned)mac_sony_get_pblk(sony, ioVRefNum, 2);

	if ((vref < 1) || (vref > SONY_DRIVES)) {
		mac_sony_return(sony, nsDrvErr, 0);
		return;
	}

	vars = mac_sony_get_vars(sony, vref);

	if (mem_get8(vars + SONY_DISKINPLACE) == 0) {
		mac_sony_return(sony, offLineErr, 0);
		return;
	}

	mem_set8(vars + SONY_DISKINPLACE, 0x02);

	switch (trap & 0xff) {
	case 2:
		mac_sony_prime_read(sony, vref);
		break;

	case 3:
		mac_sony_prime_write(sony, vref);
		break;

	default:
		mac_sony_return(sony, 0xffef, 0);
		break;
	}
}

static int mac_sony_format(block_t *dsk, unsigned long blk)
{
	unsigned char buf[512];
	unsigned long i, n;

	uint64_t cap = dsk->capacity ? dsk->capacity(dsk) : 0;
	n = (unsigned long)(cap / 512ULL);

	if (n != blk) {
		return 1;
	}

	memset(buf, 0x00, 512);

	for (i = 0; i < n; i++) {
		if (dsk->write == NULL || dsk->write(dsk, buf, (uint64_t)i * 512ULL, 512) != 512) {
			return 1;
		}
	}

	return 0;
}

static void mac_sony_ctl_verify(mac_sony_t *sony)
{
	unsigned vref;
	block_t *dsk;

	vref = (unsigned)mac_sony_get_pblk(sony, ioVRefNum, 2);

	if ((vref < 1) || (vref > SONY_DRIVES)) {
		mac_sony_return(sony, nsDrvErr, 0);
		return;
	}

	dsk = sony->disk[vref];

	if (dsk == NULL) {
		mac_sony_return(sony, noDriveErr, 0);
		return;
	}

	mac_sony_return(sony, noErr, 0);
}

static void mac_sony_ctl_eject(mac_sony_t *sony)
{
	unsigned vref;
	unsigned long vars;

	vref = (unsigned)mac_sony_get_pblk(sony, ioVRefNum, 2);

	if ((vref < 1) || (vref > SONY_DRIVES)) {
		mac_sony_return(sony, nsDrvErr, 0);
		return;
	}

	vars = mac_sony_get_vars(sony, vref);

	printf("SONY: eject drive %u\n", vref);
	mem_set8(vars + SONY_DISKINPLACE, 0x00);
	mem_set8(vars + SONY_WPROT, 0x00);
	mem_set8(vars + SONY_TWOSIDEFMT, 0x00);
	if (sony->on_eject != NULL) {
		sony->on_eject(vref, sony->on_eject_ctx);
	}

	mac_sony_return(sony, noErr, 0);
}

static void mac_sony_ctl_set_tag_buf(mac_sony_t *sony)
{
	unsigned long tagbuf;

	tagbuf = mac_sony_get_pblk(sony, csParam, 4);

	sony->tag_buf = tagbuf & 0x00ffffff;

	mac_sony_return(sony, noErr, 0);
}

static void mac_sony_ctl_format(mac_sony_t *sony)
{
	unsigned vref, format;
	unsigned long blk;
	unsigned long vars;
	block_t *dsk;

	vref = (unsigned)mac_sony_get_pblk(sony, ioVRefNum, 2);
	format = (unsigned)mac_sony_get_pblk(sony, csParam, 2);

	if ((vref < 1) || (vref > SONY_DRIVES)) {
		mac_sony_return(sony, nsDrvErr, 0);
		return;
	}

	dsk = sony->disk[vref];

	if (dsk == NULL) {
		mac_sony_return(sony, noDriveErr, 0);
		return;
	}

	if (dsk->write == NULL) {
		mac_sony_return(sony, wPrErr, 0);
		return;
	}

	if ((format > 0) && (format <= sony->format_cnt)) {
		blk = sony->format_list[2 * (format - 1)];
	} else {
		uint64_t cap = dsk->capacity ? dsk->capacity(dsk) : 0;
		blk = (unsigned long)(cap / 512ULL);
	}

	if (mac_sony_format(dsk, blk)) {
		mac_sony_return(sony, paramErr, 0);
		return;
	}

	vars = mac_sony_get_vars(sony, vref);
	mem_set16(vars + 18, (unsigned)(blk & 0xffff));
	mem_set16(vars + 20, (unsigned)(blk >> 16));

	mac_sony_return(sony, noErr, 0);
}

static void mac_sony_ctl_get_icon(mac_sony_t *sony, int which)
{
	unsigned vref;
	unsigned long addr, addr1, addr2;
	block_t *dsk;

	(void)which;

	vref = (unsigned)mac_sony_get_pblk(sony, ioVRefNum, 2);

	if ((vref < 1) || (vref > SONY_DRIVES)) {
		mac_sony_return(sony, nsDrvErr, 0);
		return;
	}

	addr1 = sony->icon_addr[0];
	addr2 = sony->icon_addr[1];

	dsk = sony->disk[vref];

	if (dsk == NULL) {
		addr = addr1;
	} else {
		uint64_t cap = dsk->capacity ? dsk->capacity(dsk) : 0;
		unsigned long blocks = (unsigned long)(cap / 512ULL);
		switch (blocks) {
		case 800:
		case 1600:
		case 1440:
		case 2880:
			addr = addr1;
			break;

		default:
			addr = addr2;
			break;
		}
	}

	mac_sony_set_pblk(sony, csParam, 4, addr);

	mac_sony_return(sony, noErr, 0);
}

static void mac_sony_ctl_get_drive_info(mac_sony_t *sony)
{
	unsigned vref, val;

	vref = (unsigned)mac_sony_get_pblk(sony, ioVRefNum, 2);

	if ((vref < 1) || (vref > SONY_DRIVES)) {
		mac_sony_return(sony, nsDrvErr, 0);
		return;
	}

	val = vref - 1;
	val = ((val << 3) & 8) | ((val >> 1) & 1);

	mac_sony_set_pblk(sony, csParam, 4, (val << 8) | 0x04);

	mac_sony_return(sony, noErr, 0);
}

static void mac_sony_ctl_format_copy(mac_sony_t *sony)
{
	(void)sony;
	mac_sony_return(sony, paramErr, 0);
}

static void mac_sony_control(mac_sony_t *sony)
{
	unsigned cscode;

	cscode = (unsigned)mac_sony_get_pblk(sony, csCode, 2);

	switch (cscode) {
	case 1:
		mac_sony_return(sony, abortErr, 1);
		break;

	case 5:
		mac_sony_ctl_verify(sony);
		break;

	case 7:
		mac_sony_ctl_eject(sony);
		return;

	case 8:
		mac_sony_ctl_set_tag_buf(sony);
		break;

	case 9:
		mac_sony_return(sony, 0xffc8, 0);
		break;

	case 6:
		mac_sony_ctl_format(sony);
		return;

	case 21:
		mac_sony_ctl_get_icon(sony, 0);
		return;

	case 22:
		mac_sony_ctl_get_icon(sony, 1);
		return;

	case 23:
		mac_sony_ctl_get_drive_info(sony);
		return;

	case 21315:
		mac_sony_ctl_format_copy(sony);
		return;

	default:
		mac_sony_return(sony, controlErr, 0);
		return;
	}
}

static void mac_sony_status_format_list(mac_sony_t *sony)
{
	unsigned i;
	unsigned long ptr;
	unsigned vref, cnt;
	unsigned long blk;
	unsigned long *list;
	block_t *dsk;

	vref = (unsigned)mac_sony_get_pblk(sony, ioVRefNum, 2);
	cnt = (unsigned)mac_sony_get_pblk(sony, csParam, 2);
	ptr = mac_sony_get_pblk(sony, csParam + 2, 4);

	if ((vref < 1) || (vref > SONY_DRIVES)) {
		mac_sony_return(sony, nsDrvErr, 0);
		return;
	}

	dsk = sony->disk[vref];

	if (dsk == NULL) {
		mac_sony_return(sony, noDriveErr, 0);
		return;
	}

	uint64_t cap = dsk->capacity ? dsk->capacity(dsk) : 0;
	blk = (unsigned long)(cap / 512ULL);

	list = sony->format_list;

	sony->format_cnt = 1;
	list[0] = blk;
	list[1] = 0;

	if (cnt > sony->format_cnt) {
		cnt = sony->format_cnt;
	}

	for (i = 0; i < cnt; i++) {
		mem_set32((unsigned int)(ptr + 8 * i + 0), list[2 * i + 0]);
		mem_set32((unsigned int)(ptr + 8 * i + 4), list[2 * i + 1]);
	}

	mac_sony_set_pblk(sony, csParam, 2, cnt);

	mac_sony_return(sony, noErr, 0);
}

static void mac_sony_status_drive_status(mac_sony_t *sony)
{
	unsigned i;
	unsigned vref;
	unsigned long val;
	unsigned long src;
	block_t *dsk;

	vref = (unsigned)mac_sony_get_pblk(sony, ioVRefNum, 2);

	if ((vref < 1) || (vref > SONY_DRIVES)) {
		mac_sony_return(sony, nsDrvErr, 0);
		return;
	}

	dsk = sony->disk[vref];

	if (dsk == NULL) {
		mac_sony_return(sony, noDriveErr, 0);
		return;
	}

	src = mac_sony_get_vars(sony, vref);

	for (i = 0; i < 11; i++) {
		val = mem_get16((unsigned int)(src + 2 * i));
		mac_sony_set_pblk(sony, csParam + 2 * i, 2, val);
	}

	mac_sony_return(sony, noErr, 0);
}

static void mac_sony_status(mac_sony_t *sony)
{
	unsigned cscode;

	cscode = (unsigned)mac_sony_get_pblk(sony, csCode, 2);

	switch (cscode) {
	case 6:
		mac_sony_status_format_list(sony);
		break;

	case 8:
		mac_sony_status_drive_status(sony);
		break;

	default:
		mac_sony_return(sony, statusErr, 0);
		break;
	}
}

int mac_sony_hook(mac_sony_t *sony, unsigned val)
{
	switch (val) {
	case MAC_HOOK_SONY_OPEN:
		mac_sony_open(sony);
		return 0;

	case MAC_HOOK_SONY_PRIME:
		mac_sony_prime(sony);
		return 0;

	case MAC_HOOK_SONY_CTRL:
		mac_sony_control(sony);
		return 0;

	case MAC_HOOK_SONY_STATUS:
		mac_sony_status(sony);
		return 0;

	case MAC_HOOK_SONY_CLOSE:
		return 0;
	}

	return 1;
}

void mac_sony_reset(mac_sony_t *sony)
{
	unsigned i;

	mac_sony_unpatch_rom(sony);

	sony->open = 0;

	for (i = 0; i < SONY_DRIVES; i++) {
		sony->delay_cnt[i] = sony->delay_val[i];
	}
}
