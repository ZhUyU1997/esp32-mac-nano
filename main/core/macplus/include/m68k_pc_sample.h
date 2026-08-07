#ifndef M68K_PC_SAMPLE_H
#define M68K_PC_SAMPLE_H

#include <stdint.h>

/*
 * Default 0. Set to 1 via CMake target_compile_definitions (see main/CMakeLists.txt):
 *   M68K_PC_SAMPLE_ENABLED=1
 * That enables Musashi M68K_INSTRUCTION_HOOK → m68k_instruction() every insn (slow).
 * Logs only when the PC hits an address in m68k_pc_sample.c:m68k_pc_watch[] (first hit each).
 * That table includes e.g. 0x400A30 (P_BootFromDisk _CopyBits site) plus boot checkpoints.
 */
#ifndef M68K_PC_SAMPLE_ENABLED
#define M68K_PC_SAMPLE_ENABLED 0
#endif

void m68k_pc_sample_reset(void);
void m68k_pc_sample_at_pc(unsigned int pc);

#endif
