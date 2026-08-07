PCEX ROM sources (fork for esp32-mac-nano)
==========================================

Upstream (read-only reference, PCE repo):
  pce/src/arch/macplus/pcex/pcex.S
  Makefile.inc copied as Makefile.inc.pce-upstream

Files:
  pcex.S          — PCE default: 0x4afc illegal + .word hook id (needs Musashi hook).
  pcex_mmio.S     — same logic, but traps via faulting_address + move.b hook id
                    (see Basilisk-style sonydrv.S MMIO). Requires emulator
                    support: patch faulting_address to a trapped MMIO byte, and
                    dispatch from m68k_write_memory_8 (MAC_MMIO_HOOK_ADDR) instead of
                    m68k_op_illegal.

Build (needs m68k-linux-gnu-gcc / m68k-elf- toolchain):
  make -C macintosh/pcex
  Outputs:
    pcex_illegal.rom — from pcex.S (illegal-instruction path)
    pcex_mmio.rom    — from pcex_mmio.S (MMIO @ faulting_address in source)

ESP-IDF embed (this repo — MMIO variant)
-----------------------------------------
  Build once:  make -C macintosh/pcex  →  macintosh/pcex/pcex_mmio.rom

  main/CMakeLists.txt embeds that path directly (no copy):
    ../macintosh/pcex/pcex_mmio.rom

  macplus.c expects embedded symbols from the filename (ESP-IDF idf.py embed):
    _binary_pcex_mmio_rom_start / _binary_pcex_mmio_rom_end

  MAC_MMIO_HOOK_ADDR in main/macplus/core/macplus.c must match faulting_address in pcex_mmio.S.

Link address: 0xF80000 (same as PCE macplus).

Using pcex_illegal.rom (PCE default / no MMIO in guest)
--------------------------------------------------------
If the guest calls the Sony hook via 0x4afc + hook word (not via MMIO), embed
the illegal build and wire Musashi + macplus.c as follows.

1) Binary on disk and CMake
   - Build:  make -C macintosh/pcex  →  macintosh/pcex/pcex_illegal.rom
   - In main/CMakeLists.txt EMBED_FILES, use:
       "../macintosh/pcex/pcex_illegal.rom"
     (not pcex_mmio.rom)

2) macplus.c — embedded blob symbols (name follows the .rom filename):
     extern const uint8_t pcex_illegal_rom_start[]
         asm("_binary_pcex_illegal_rom_start");
     extern const uint8_t pcex_illegal_rom_end[]
         asm("_binary_pcex_illegal_rom_end");
   Use those pointers in pcex_access_cb (and PCEX_ROM_SIZE) instead of
   pcex_mmio_rom_*.

3) macplus.c — export the Sony hook for the CPU illegal handler:
     Change mac_sony_try_hook from static to a global with external linkage, e.g.
       int mac_sony_try_hook(unsigned int hook_word);
     (same implementation as today; Musashi will call it.)

4) musashi/m68kopdm.c — after #include "m68kcpu.h", add:
     extern int mac_sony_try_hook(unsigned int hook_word);

   Replace m68k_op_illegal with a special case for 0x4afc:

     void m68k_op_illegal(void)
     {
         if (REG_IR == 0x4afc) {
             unsigned int ext = m68k_read_memory_16(REG_PC);
             unsigned int old_pc = m68k_get_reg(NULL, M68K_REG_PC);
             if (mac_sony_try_hook(ext) == 0) {
                 unsigned int new_pc = m68k_get_reg(NULL, M68K_REG_PC);
                 if (new_pc == old_pc)
                     REG_PC += 2;
                 return;
             }
         }
         m68ki_exception_illegal();
     }

You cannot use only the illegal ROM without these Musashi + linker changes;
otherwise 0x4afc will raise a real illegal instruction exception.
