#include "mac_trap_log.h"

#include <stdio.h>
#include <string.h>

#include "m68k.h"
#include "m68kconf.h"

#if MAC_TRAP_LOG

const char *mac_trap_name_lookup(uint16_t w)
{
	for (unsigned i = 0; i < mac_trap_count; i++) {
		if (mac_trap_words[i] == w)
			return mac_trap_names[i];
	}
	uint16_t c = (w & (1u << 11)) ? (uint16_t)(w & 0xF8FFu) : (uint16_t)(w & 0xFBFFu);
	if (c != w) {
		for (unsigned i = 0; i < mac_trap_count; i++) {
			if (mac_trap_words[i] == c)
				return mac_trap_names[i];
		}
	}
	return NULL;
}

static uint16_t stkw(uint32_t sp, unsigned off)
{
	return (uint16_t)m68k_read_memory_16((sp + off) & 0xffffffu);
}

static uint32_t stkl(uint32_t sp, unsigned off)
{
	return m68k_read_memory_32((sp + off) & 0xffffffu);
}

static void snprint_rect_at_ptr(char *buf, size_t buflen, uint32_t ptr24)
{
	uint32_t r = ptr24 & 0xffffffu;
	int16_t top = (int16_t)stkw(r, 0);
	int16_t left = (int16_t)stkw(r, 2);
	int16_t bottom = (int16_t)stkw(r, 4);
	int16_t right = (int16_t)stkw(r, 6);

	snprintf(buf, buflen, " R@(%06lX)=(%d,%d,%d,%d)", (unsigned long)r, (int)top, (int)left, (int)bottom, (int)right);
}

/** Pascal stack at trap: params pushed R→L, first param at SP (Inside Macintosh). */
static void mac_trap_snprint_params(uint16_t w, uint32_t sp, char *buf, size_t buflen)
{
	const char *n = mac_trap_name_lookup(w);

	if (!buflen)
		return;
	buf[0] = 0;
	if (sp & 1u) {
		snprintf(buf, buflen, " | sp=%06lX (odd)", (unsigned long)(sp & 0xffffffu));
		return;
	}

	if (!n)
		n = "";

	/* QuickDraw / common */
	if (!strcmp(n, "DrawText")) {
		/* DrawText(textPtr, firstByte, byteCount) → (SP)=byteCount, 2(SP)=firstByte, 4(SP)=textPtr */
		uint16_t byteCount = stkw(sp, 0);
		uint16_t firstByte = stkw(sp, 2);
		uint32_t textPtr = stkl(sp, 4) & 0xffffffu;
		char textStr[64];
		unsigned j;

		for (j = 0; j < byteCount && j < (sizeof(textStr) - 1); j++) {
			textStr[j] = (char)m68k_read_memory_8(textPtr + firstByte + j);
		}
		textStr[j] = 0;

		snprintf(buf, buflen, " | count=%u first=%u text@=%06lX \"%s\"", (unsigned)byteCount, (unsigned)firstByte, (unsigned long)textPtr, textStr);
		return;
	}
	if (!strcmp(n, "MoveTo")) {
		int16_t v = (int16_t)stkw(sp, 0); // Last param (rightmost)
		int16_t h = (int16_t)stkw(sp, 2); // First param (leftmost)

		snprintf(buf, buflen, " | h=%d v=%d", (int)h, (int)v);
		return;
	}
	if (!strcmp(n, "Move")) {
		int16_t dv = (int16_t)stkw(sp, 0);
		int16_t dh = (int16_t)stkw(sp, 2);
		snprintf(buf, buflen, " | dh=%d dv=%d", (int)dh, (int)dv);
		return;
	}
	if (!strcmp(n, "EraseRect") || !strcmp(n, "FrameRect")) {
		uint32_t rp = stkl(sp, 0) & 0xffffffu;
		char rbuf[80];

		rbuf[0] = 0;
		if (rp)
			snprint_rect_at_ptr(rbuf, sizeof rbuf, rp);
		snprintf(buf, buflen, " | r@=%06lX%s", (unsigned long)rp, rbuf);
		return;
	}
	if (!strcmp(n, "PenSize")) {
		int16_t ph = (int16_t)stkw(sp, 0);
		int16_t pw = (int16_t)stkw(sp, 2);
		snprintf(buf, buflen, " | w=%d h=%d", (int)pw, (int)ph);
		return;
	}
	if (!strcmp(n, "PlotIcon")) {
		uint32_t icon = stkl(sp, 0);
		uint32_t rp = stkl(sp, 4) & 0xffffffu;
		char rbuf[80];

		rbuf[0] = 0;
		if (rp)
			snprint_rect_at_ptr(rbuf, sizeof rbuf, rp);
		snprintf(buf, buflen, " | r@=%06lX icon=%08lX%s", (unsigned long)rp, (unsigned long)icon, rbuf);
		return;
	}
	if (!strcmp(n, "CopyBits")) {
		uint32_t maskRgn = stkl(sp, 0);
		uint16_t mode = stkw(sp, 4);
		uint32_t dstRect = stkl(sp, 6) & 0xffffffu;
		uint32_t srcRect = stkl(sp, 10) & 0xffffffu;
		uint32_t dstBits = stkl(sp, 14);
		uint32_t srcBits = stkl(sp, 18);

		(void)maskRgn;
		(void)dstBits;
		(void)srcBits;

		/* 
		 * If we moved the icon but it's drawn via CopyBits directly, we may need to 
		 * offset dstRect here too. But usually PlotIcon is enough.
		 */

		snprintf(buf, buflen, " | sRect@=%06lX dRect@=%06lX mode=%u", (unsigned long)srcRect, (unsigned long)dstRect, (unsigned)mode);
		return;
	}
	if (!strcmp(n, "InsetRect") || !strcmp(n, "OffsetRect")) {
		int16_t b = (int16_t)stkw(sp, 0);      // Last param (dv)
		int16_t a = (int16_t)stkw(sp, 2);      // Middle param (dh)
		uint32_t rp = stkl(sp, 4) & 0xffffffu; // First param (Rect*)
		char rbuf[80];

		rbuf[0] = 0;
		if (rp)
			snprint_rect_at_ptr(rbuf, sizeof rbuf, rp);
		snprintf(buf, buflen, " | r@=%06lX a=%d b=%d%s", (unsigned long)rp, (int)a, (int)b, rbuf);
		return;
	}
	if (!strcmp(n, "PenNormal")) {
		snprintf(buf, buflen, " | (no args)");
		return;
	}
	if (!strcmp(n, "InitGraf") || !strcmp(n, "InitPort")) {
		uint32_t p = stkl(sp, 0) & 0xffffffu;
		snprintf(buf, buflen, " | ptr@=%06lX", (unsigned long)p);
		return;
	}
	if (!strcmp(n, "BlockMove")) {
		/* BlockMove(srcPtr, destPtr, byteCount) → (SP)=byteCount, 4(SP)=destPtr, 8(SP)=srcPtr */
		uint32_t cnt = stkl(sp, 0);
		uint32_t dst = stkl(sp, 4) & 0xffffffu;
		uint32_t src = stkl(sp, 8) & 0xffffffu;
		snprintf(buf, buflen, " | src=%06lX dst=%06lX cnt=%lu", (unsigned long)src, (unsigned long)dst, (unsigned long)cnt);
		return;
	}
	if (!strcmp(n, "GetResource")) {
		/* GetResource(type, id) → (SP)=id, 2(SP)=type */
		int16_t id = (int16_t)stkw(sp, 0);
		uint32_t type = stkl(sp, 2);
		snprintf(buf, buflen, " | id=%d type=%08lX", (int)id, (unsigned long)type);
		return;
	}
	if (!strcmp(n, "NewHandle") || !strcmp(n, "NewHandleSysClear") || !strcmp(n, "NewHandleClear")) {
		uint32_t sz = stkl(sp, 0);
		snprintf(buf, buflen, " | size=%lu", (unsigned long)sz);
		return;
	}
	/* Read/Open vary by high-level vs PB; fall through to raw */

	/* default: raw stack slots */
	{
		uint32_t a = stkl(sp, 0);
		uint32_t b = stkl(sp, 4);
		uint32_t c = stkl(sp, 8);
		uint32_t d = stkl(sp, 12);

		snprintf(buf,
		         buflen,
		         " | stk@%06lX: %08lX %08lX %08lX %08lX",
		         (unsigned long)(sp & 0xffffffu),
		         (unsigned long)a,
		         (unsigned long)b,
		         (unsigned long)c,
		         (unsigned long)d);
	}
}

#if MAC_TRAP_LOG
static void mac_trap_log_full_word(uint16_t w, uint32_t fault_pc, uint32_t sp)
{
	const char *name;
	char pbuf[160];

	if ((w & 0xF000u) != 0xA000u)
		return;
	name = mac_trap_name_lookup(w);
	mac_trap_snprint_params(w, sp, pbuf, sizeof pbuf);
	printf("trap %04X @%06X %s%s\n", (unsigned)w, (unsigned)(fault_pc & 0xFFFFFFu), name ? name : "?", pbuf);
	fflush(stdout);
}
#endif

void mac_trap_on_1010_exception(void)
{
	uint16_t w = (uint16_t)m68k_get_reg(NULL, M68K_REG_IR);
	uint32_t fault_pc = (uint32_t)m68k_get_reg(NULL, M68K_REG_PPC) & 0xffffffu;
	uint32_t sp = (uint32_t)m68k_get_reg(NULL, M68K_REG_SP) & 0xffffffu;

	mac_trap_log_full_word(w, fault_pc, sp);
}

#else

void mac_trap_on_1010_exception(void)
{
}

#endif
