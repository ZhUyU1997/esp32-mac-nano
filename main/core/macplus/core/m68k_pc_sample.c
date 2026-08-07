/*
 * Optional PC watchpoints for boot / profiling (Musashi).
 */
#include "m68k_pc_sample.h"

#if M68K_PC_SAMPLE_ENABLED

#include <inttypes.h>
#include <stdio.h>
#include "esp_timer.h"

/* Physical 24-bit PCs as seen by the CPU (ROM at 0x400000 + offset). */
static const uint32_t m68k_pc_watch[] = {
        0x00400a30u, /* P_BootFromDisk: _CopyBits (listing ~A30) */
        0x00400d9eu, /* P_ChecksumRomAndTestMemory: after checksum, test 2 */
        0x00400e36u, /* L125: tests done, jmp (A6) */
};

#define M68K_PC_WATCH_COUNT (sizeof m68k_pc_watch / sizeof m68k_pc_watch[0])

static uint8_t m68k_pc_seen[M68K_PC_WATCH_COUNT];

void m68k_pc_sample_reset(void)
{
	for (unsigned i = 0; i < M68K_PC_WATCH_COUNT; i++)
		m68k_pc_seen[i] = 0;
}

void m68k_pc_sample_at_pc(unsigned int pc)
{
	unsigned int pcm = pc & 0xffffffu;

	for (unsigned i = 0; i < M68K_PC_WATCH_COUNT; i++) {
		uint32_t w = (uint32_t)m68k_pc_watch[i] & 0xffffffu;
		if (pcm != w || m68k_pc_seen[i])
			continue;
		m68k_pc_seen[i] = 1;
		uint64_t t = (uint64_t)esp_timer_get_time();
		printf("m68k_pc_sample: watch[%u] PC=0x%06" PRIx32 " t=%" PRIu64 " us\n", i, (uint32_t)pcm, t);
		fflush(stdout);
	}
}

#else /* !M68K_PC_SAMPLE_ENABLED */

void m68k_pc_sample_reset(void)
{
}
void m68k_pc_sample_at_pc(unsigned int pc)
{
	(void)pc;
}

#endif
