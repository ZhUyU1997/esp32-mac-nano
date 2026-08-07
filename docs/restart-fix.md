# Mac Plus System 6 Restart Fix

Bisected to commits `2e544da..702a961`. Cold boot calls `m68k_pulse_reset()`. Warm restart via RESET instruction did not.

## Real Mac Plus RESET sequence

System 6 Restart:
  1. (cleanup)
  2. RESET instruction → RESET line asserted 124 cycles
     ├─ VIA 6522 reset → DDRA=0, port A bit 4 = input + pull-up → overlay ON
     ├─ SCC/IWM/SCSI reset
     └─ 68000 continues (PC unchanged)
  3. Next instruction: MOVEQ #2,D2 (fetch: prefetch queue, no bus access)
  4. BSR sequence → eventually branches to ROM entry 0x400000

## Broken code sequence (before fix)

System 6 Restart:
  1. (cleanup)
  2. RESET instruction → callback:
     ├─ mac_set_overlay(s, 1)        — overlay OFF→ON
     │   └─ memmap_rebuild_direct    — clears table, rebuilds
     │       0x000000-0x01FFFF → ROM (was RAM)
     ├─ e6522_reset                  — VIA reset
     │   └─ e6522_set_ora_out        — port A callback fires
     │       s->via_port_a ≠ via->ira → overlay/vbuf changed mid-instruction
     └─ other resets
  3. Next instruction fetch at 0x007958:
     ├─ overlay=ON, no prefetch → 0x007958 reads ROM, not RAM
     ├─ gets ROM garbage instead of MOVEQ #2
     └─ executes garbage → PC corrupts to 0x7506002 → crash

## Fixed code sequence

System 6 Restart:
  1. (cleanup)
  2. RESET instruction → callback:
     ├─ m68k_set_reg(SR, 0x2700)     — FLAG_INT_MASK = 7
     ├─ mac_irq_reset                — virq_state = 0 (7→1)
     ├─ e6522_reset                  — VIA timer stopped
     ├─ mac_set_overlay(s, 1)        — overlay ON, memmap rebuilt
     └─ mac_sony_reset, mac_scsi_reset, ... — peripherals
  3. Next instruction fetch at 0x007958:
     ├─ overlay=ON → would read ROM
     ├─ but M68K_EMULATE_PREFETCH has cached the 4-byte block from before rebuild
     └─ instruction served from prefetch cache → MOVEQ #2 (correct)

| # | File | Change |
|---|------|--------|
| 1 | `m68kconf.h` | `M68K_EMULATE_RESET = OPT_SPECIFY_HANDLER` |
| 2 | `interrupts.c` | callback: `m68k_set_reg(SR,0x2700)` + `mac_reset(macplus_instance())` |
| 3 | `interrupts.c` | `mac_irq_reset`: `m68k_set_virq(7→1,0)`, 7→1 avoids spurious NMI |
| 4 | `macplus.c` | keep `mac_set_overlay(s,1)` in `mac_reset` |
| 5 | `macplus.c` | move `mac_iwm_init`/`mac_sony_patch`/`mac_set_vbuf` to `macplus_boot` |
| 6 | `macplus.c` | `mac_reset` made non-static |
| 7 | `macplus.h` | declare `mac_reset` |
| 8 | — | `M68K_EMULATE_PREFETCH = OPT_ON` |
