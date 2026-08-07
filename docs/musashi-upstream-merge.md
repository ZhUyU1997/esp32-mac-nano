# Musashi 上游合并计划

目标：将官方 [kstenerud/Musashi](https://github.com/kstenerud/Musashi) 从当前基线（`83c920e` 附近）推进到 `5d9df94`（m68k_set_virq）。

项目仅支持 68000，不涉及 FPU/MMU/68040 等功能。

合并分支：`feat/musashi-upstream-merge`

## 结论先行

| commit | 日期 | 作者 | 说明 | 操作 |
|--------|------|------|------|------|
| \`78b3b13\` | 2016-04-27 | Martin Hellspong | Don't deduct instruction cycles during address errors | ❌ 跳过 |
| \`c19c871\` | 2016-04-27 | Martin Hellspong | Guard re-entry of exec-loop after address errors | ❌ 跳过 |
| \`843ce23\` | 2016-04-27 | Martin Hellspong | Undo instruction cycles in trap exception | ❌ 跳过 |
| \`c9b6972\` | 2016-04-27 | Karl Stenerud | Merge pull request #20 from marhel/address-error-cycles | ❌ 跳过 |
| \`2b8ef21\` | 2016-04-27 | Martin Hellspong | Undo instruction cycles in trap#n exception | ❌ 跳过 |
| \`c8faafc\` | 2016-04-28 | Karl Stenerud | Merge pull request #21 from marhel/trap-cycles | ❌ 跳过 |
| \`769ea6b\` | 2016-04-28 | Karl Stenerud | Merge pull request #22 from marhel/trap#n-cycles | ❌ 跳过 |
| \`15a51d1\` | 2016-04-28 | Karl Stenerud | Merge pull request #23 from marhel/unimplemented-instruction-cycles | ❌ 跳过 |
| \`a16bd2b\` | 2017-09-24 | SteveKwok | Fix size of ANDI, EORI, ORI | ❌ 跳过（已回滚，CCR 编码 16-bit 正确） |
| \`df0fb40\` | 2017-09-24 | SteveKwok | Fix size of ANDI, EORI, ORI | ❌ 跳过 |
| \`f24f2d2\` | 2017-11-05 | Karl Stenerud | Merge pull request #26 from SteveKwok/master | ❌ 跳过 |
| \`3368bed\` | 2018-06-09 | George Koehler | Remove extra `#include <stdio.h>` to fix `uint` | ✅ 已合入分支 |
| \`e0a7b5f\` | 2018-06-22 | Karl Stenerud | Merge pull request #31 from kernigh/kernigh-stdio | ❌ 跳过 |
| \`3ae92a7\` | 2018-07-13 | Derek Fawcus | Correct file modes | ❌ 跳过 |
| \`fe93f3a\` | 2018-07-13 | Karl Stenerud | Merge pull request #32 from dfawcus/file-modes | ❌ 跳过 |
| \`521059b\` | 2019-01-13 | tyfkda | Fix loop condition | ❌ 跳过 |
| \`c641acd\` | 2019-02-10 | Karl Stenerud | Merge pull request #33 from tyfkda/feature/warning | ❌ 跳过 |
| \`5bcffd7\` | 2019-09-04 | Emmanuel Anne | Revert "Fix size of ANDI, EORI, ORI" | ❌ 跳过 |
| \`a30886f\` | 2019-09-04 | Emmanuel Anne | Revert "Fix spelling, add note about unverified 020-counts" | ❌ 跳过 |
| \`d506471\` | 2019-09-04 | Emmanuel Anne | Revert "Fix MOVEM cycle-counts" | ❌ 跳过 |
| \`9667752\` | 2019-09-04 | Emmanuel Anne | from mame088 to mame097 | ❌ 跳过 |
| \`dad4a52\` | 2019-09-04 | Emmanuel Anne | basic Makefile to quickly test things | ❌ 跳过 |
| \`de46c03\` | 2019-09-04 | Emmanuel Anne | mame098: support for 68040 ! | ❌ 跳过 |
| \`a9a9bba\` | 2019-09-04 | Emmanuel Anne | fpu emulation mainly (from mame98 to mame106) | ❌ 跳过 |
| \`dcdff4d\` | 2019-09-04 | Emmanuel Anne | restored the example so that it can be compiled in linux | ❌ 跳过 |
| \`abc09c9\` | 2019-09-04 | Emmanuel Anne | cmpild, tas and rte callbacks (mame110) | ❌ 跳过 |
| \`bb4ca57\` | 2019-09-04 | Emmanuel Anne | mame115: u/sint32 definition update, movec for 040 | ❌ 跳过 |
| \`25e031e\` | 2019-09-04 | Emmanuel Anne | mame120: version update to 3.31 | ❌ 跳过 |
| \`b2ed988\` | 2019-09-04 | Emmanuel Anne | mame123: const, formating, and fix for reset instruction | ❌ 跳过 |
| \`438d1f4\` | 2019-09-05 | Emmanuel Anne | a few quirks with the new m68ki_instr_hook format | ❌ 跳过 |
| \`6ea3eaa\` | 2019-09-05 | Aaron Giles | Fixed 68000 prefetching operation. | ❌ 跳过 |
| **\`5d9df94\`** | **2019-09-05** | **Aaron Giles** | **Fix m68k irq line support.** | **✅ 已手动合并** |
| \`6fcf2d4\` | 2019-09-05 | Emmanuel Anne | const & dasm stuff | ❌ 跳过 |
| \`0a75e6e\` | 2019-09-05 | Aaron Giles | Changed 68000 interrupts to only trigger during execution. | ✅ 已合入分支 |
| \`f0d95c3\` | 2019-09-05 | Aaron Giles | Fixed handling of interrupts when the CPU was in the STOP state. | ✅ 已合入分支 |
| \`2c0f575\` | 2019-09-05 | Emmanuel Anne | from dgen fix M68K_REG_SR handling in m68k_set_reg() | ✅ 已合入分支 |
| \`b56ba66\` | 2019-09-06 | Karl Stenerud | Merge pull request #35 from zelurker/master | ❌ 跳过 |
| \`dbe9c95\` | 2019-09-06 | Emmanuel Anne | get rid of an unused variable | ✅ 无害 |
| \`241d1ef\` | 2019-09-06 | Emmanuel Anne | illegal instruction callback | ❌ 跳过 |
| \`7781886\` | 2019-09-06 | Emmanuel Anne | some more info in the readme | ❌ 跳过 |
| \`a8bf1d8\` | 2019-09-06 | Emmanuel Anne | more info in the readme | ❌ 跳过 |
| \`336331a\` | 2019-09-06 | Karl Stenerud | Merge pull request #37 from zelurker/doc | ❌ 跳过 |
| \`a9dffa0\` | 2019-09-06 | Karl Stenerud | Merge pull request #36 from zelurker/master | ❌ 跳过 |
| \`3c50276\` | 2019-09-13 | Brett Morgan | Hide generated output from git | ❌ 跳过 |
| \`474c267\` | 2019-09-12 | Karl Stenerud | Merge pull request #38 from domesticmouse/master | ❌ 跳过 |
| \`ea9dd8c\` | 2019-10-05 | arnaud | fixed disassembly (completely broken in !g_rawop mode ) | ❌ 跳过 |
| \`c1ebef6\` | 2019-10-06 | Karl Stenerud | Merge pull request #41 from arnaud-carre/master | ❌ 跳过 |
| \`890efa8\` | 2019-11-03 | Karl Stenerud | Add C++ guards to header files | ✅ 无害 |
| \`8266fbe\` | 2019-11-03 | Karl Stenerud | Removed INLINE and replaced it with static inline because it's part of the spec since c9x. Beefed up the warnings and cleared them up. m68kfpu.c is no longer included by m68kcpu.c. Removed a bunch of internal function forward declarations, except for those used by macros. Converted CRLF line endings to LF. | ❌ 跳过 |
| \`b1cbb3c\` | 2019-11-03 | Karl Stenerud | Small fixes for MacOS | ❌ 跳过 |
| \`cb1c759\` | 2019-11-04 | Karl Stenerud | Support MSVC noreturn | ❌ 跳过 |
| \`5a93a6d\` | 2019-11-05 | Karl Stenerud | Updated example m68kconf.h, and added fpu code to makefile | ❌ 跳过 |
| \`f3c3022\` | 2019-11-06 | Emmanuel Anne | allow to set the config file from a define | ❌ 跳过 |
| \`1673c69\` | 2019-11-27 | Philip Pemberton | fix memcpy on overlapping region | ❌ 跳过 |
| \`8143681\` | 2019-11-27 | Philip Pemberton | add bus error emulation | ❌ 跳过 |
| \`aa603f5\` | 2019-11-27 | Philip Pemberton | always include setjmp | ❌ 跳过 |
| \`3a2c230\` | 2019-11-27 | Philip Pemberton | add missing int i | ❌ 跳过 |
| \`27787fb\` | 2019-11-27 | Philip Pemberton | remove unused m68ki_bus_error_return_jmp_buf | ❌ 跳过 |
| \`0c6b08b\` | 2019-11-27 | Philip Pemberton | use CPU_RUN_MODE instead of BUS_ERROR_OCCURRED | ❌ 跳过 |
| \`4fc09e2\` | 2019-11-30 | Karl Stenerud | Merge pull request #47 from philpem/philpem/buserror | ❌ 跳过 |
| \`6be1436\` | 2019-11-30 | Karl Stenerud | Merge pull request #46 from philpem/philpem/fix_overlapping_memcpy | ❌ 跳过 |
| \`e5dcb59\` | 2019-11-30 | Emmanuel Anne | add some doc about MUSASHI_CNF | ❌ 跳过 |
| \`3e0547b\` | 2019-12-01 | Karl Stenerud | Merge pull request #48 from zelurker/conf | ❌ 跳过 |
| \`24366e2\` | 2019-12-11 | Emmanuel Anne | missed some more cycle usage in mame081 ! | ❌ 跳过 |
| \`6eb0d06\` | 2019-12-11 | Aaron Giles | CPUs actually take some time to reset. Changed the 68000/68010 to eat an appropriate number of cycles after a reset. | ✅ 已合入分支 |
| \`8e00fcd\` | 2019-12-11 | Angelo Salese | Added very basic SCC68070 implementation, currently is just a basic m68k with 32-bits of address lines. | ❌ 跳过 |
| \`093d69d\` | 2019-12-11 | R. Belmont | m68k: don't save signal contexts on *BSD and Mac OS X [scarlet, R. Belmont] | ❌ 跳过 |
| \`6155d71\` | 2019-12-11 | Ryan Holtz | Fleshed out SCC68070 definition in m68k core, for CD-i use in MESS [Harmony] | ❌ 跳过 |
| \`fd717a4\` | 2019-12-11 | Ryan Holtz | Don't mention in whatsnew - puts the BSD optimization back in. | ❌ 跳过 |
| \`af45689\` | 2019-12-11 | R. Belmont | m68k: disassemble PMOVE instruction (move to/from PMMU) | ❌ 跳过 |
| \`29825f5\` | 2019-12-12 | R. Belmont | M680x0 update | ❌ 跳过 |
| \`414590f\` | 2019-12-12 | R. Belmont | m68k: throw F-line trap correctly when PMMU instructions are hit on non-equipped CPUs. | ❌ 跳过 |
| \`0d60cea\` | 2019-12-12 | Karl Stenerud | Merge pull request #50 from zelurker/master | ❌ 跳过 |
| \`8126887\` | 2019-12-12 | Emmanuel Anne | move the reset_cycles to the cpu context | ❌ 跳过 |
| \`d5576b3\` | 2019-12-12 | R. Belmont | m680x0 update: | ❌ 跳过 |
| \`50baa65\` | 2019-12-12 | R. Belmont | 680x0 update: - Support PMOVE modes from PMMU - Allow the FPU to be used for both '030 and '040 - Add byte and word FPU loads/stores - Fixed buggy FPU 64-bit stores in the (An) addressing mode | ❌ 跳过 |
| \`556c574\` | 2019-12-13 | Karl Stenerud | Merge pull request #51 from zelurker/master | ❌ 跳过 |
| \`3639c27\` | 2019-12-13 | Emmanuel Anne | move the pmmu translation from ADDRESS_68K to the _fc functions | ❌ 跳过 |
| \`530f644\` | 2019-12-13 | R. Belmont | MC680x0 update | ❌ 跳过 |
| \`7efac18\` | 2019-12-13 | Emmanuel Anne | follow mame choice again | ❌ 跳过 |
| \`710f795\` | 2019-12-13 | Karl Stenerud | Merge pull request #52 from zelurker/master | ❌ 跳过 |
| \`8e1710d\` | 2019-12-14 | R. Belmont | Properly show 32-bit displacement for 020+ A reg relative [R. Belmont] | ❌ 跳过 |
| \`4cbdf6b\` | 2019-12-14 | R. Belmont | 680x0: Improve disassembly for various FMOVE forms [R. Belmont] | ❌ 跳过 |
| \`7a934a8\` | 2019-12-14 | R. Belmont | 680x0 FPU updates [R. Belmont] | ❌ 跳过 |
| \`3ec8f60\` | 2019-12-14 | R. Belmont | 680x0 FPU update: [R. Belmont] | ❌ 跳过 |
| \`3db368e\` | 2019-12-14 | R. Belmont | m680x0 FPU updates: [R. Belmont] | ❌ 跳过 |
| \`d1fd51f\` | 2019-12-14 | Emmanuel Anne | Makefile: add some m68kcpu.c dependancies | ❌ 跳过 |
| \`de395aa\` | 2019-12-14 | R. Belmont | M68k: Add more conditionals and FGETEXP instruction [R. Belmont] | ❌ 跳过 |
| \`6f04ba0\` | 2019-12-15 | Karl Stenerud | Merge pull request #53 from zelurker/master | ❌ 跳过 |
| \`8784597\` | 2020-05-27 | Karl Stenerud | Clean up GCC compiler warnings. Fixes #39 | ✅ 无害 |
| \`3b8e84f\` | 2020-06-06 | jotd | m68k_in.c: | ❌ 跳过 |
| \`8633295\` | 2020-06-07 | jotd | in m68k_in.c - fixed all CHK2/CMP2 instructions to support signed bounds. Previously it only worked with unsigned. Also sped up a bit   by changing | by || in Z evaluation formula - cptrapcc opcodes are still unsupported, but if no exception occurs, at least PC is properly updated - trapt/trapcc 16 & 32 instructions: PC is updated before exception is triggered (or if exception isn't triggered). This fixes the   return address value (just in case code returns from trap with RTE) and also the stackframe (XDAda 68040 compiler   read the data parameter to check exception type, wrong exception type is read if stackframe is incorrect) | ❌ 跳过 |
| \`8541cd6\` | 2020-06-09 | jotd | fscc instruction: added (disp,Ax) mode | ❌ 跳过 |
| \`8d0d7bb\` | 2020-06-10 | Karl Stenerud | Merge pull request #74 from jotd666/master | ❌ 跳过 |
| \`40e8cea\` | 2020-06-13 | Mateusz Kramarczyk | Fix compile errors. | ✅ 无害 |
| \`66d00ec\` | 2020-06-14 | Karl Stenerud | Merge pull request #75 from lleoha/fix-compile-errors | ❌ 跳过 |
| \`2b85f48\` | 2021-06-24 | Karl Stenerud | Add symlink to softfloat so that example app can compile | ❌ 跳过 |
| \`9494c3b\` | 2021-06-24 | Karl Stenerud | Update example readme since it was getting horribly out of date | ❌ 跳过 |
| \`df4d10b\` | 2021-07-22 | shadyjesse | allow bus error to occur when already exception handling a bus error (fixes page fault within a page fault in Freebee emulator) | ❌ 跳过 |
| \`d0ab9ac\` | 2021-07-29 | Karl Stenerud | Merge pull request #78 from agentbooth/master | ❌ 跳过 |
| \`5a98127\` | 2021-08-15 | Poul-Henning Kamp | Add a missing comma | ❌ 跳过 |
| \`0d5b364\` | 2021-08-15 | Poul-Henning Kamp | Update cpu-name table to match the enum in m68k.h | ✅ 无害 |
| \`91878a8\` | 2021-08-15 | Poul-Henning Kamp | Add void argument to make these proper prototypes | ✅ 无害 |
| \`5f08834\` | 2021-08-15 | Poul-Henning Kamp | Make diagnostic functions take const char * arguments. | ✅ 无害 |
| \`7bb99d7\` | 2021-08-15 | Poul-Henning Kamp | Add missing prototypes | ✅ 无害 |
| \`fc7a6fc\` | 2021-08-16 | Karl Stenerud | Merge pull request #80 from bsdphk/master | ❌ 跳过 |
| \`7ec87cd\` | 2024-02-03 | bebbo | fix cycles for shift and mul | ❌ 跳过（周期不影响功能） |
| \`369f2e5\` | 2024-02-03 | bebbo | reported cycle count is bogus | ❌ 跳过 |
| \`ca418a4\` | 2024-02-03 | bebbo | add  to the disassembler | ❌ 跳过 |
| \`ffdd51b\` | 2024-02-03 | bebbo | implement and fix some fpu instructions | ❌ 跳过 |
| \`5708b92\` | 2024-02-08 | Karl Stenerud | Merge pull request #102 from bebbo/master | ❌ 跳过 |
| \`dc4ac26\` | 2024-02-08 | Karl Stenerud | Merge pull request #103 from bebbo/p2 | ❌ 跳过 |
| \`0902af7\` | 2024-02-08 | Karl Stenerud | Merge pull request #104 from bebbo/p3 | ❌ 跳过 |
| \`2158f70\` | 2024-02-08 | Karl Stenerud | Merge pull request #105 from bebbo/p4 | ❌ 跳过 |
| \`b2425b5\` | 2024-02-25 | Paul-Arnold | Correct reported cycle count | ❌ 跳过 |
| \`b62dde3\` | 2024-06-23 | Karl Stenerud | Merge pull request #107 from Paul-Arnold/master | ❌ 跳过 |
| \`a933180\` | 2024-06-23 | Karl Stenerud | Fix some warnings and ensure enough space for sprintf | ✅ 无害 |
| \`9cd2167\` | 2025-09-29 | Ennio Barbaro | Add unit tests | ❌ 跳过 |
| \`dc5846a\` | 2025-10-01 | Ennio Barbaro | Saner memory layout | ❌ 跳过 |
| \`b5f1094\` | 2025-10-01 | Ennio Barbaro | Saner test makefile | ❌ 跳过 |
| \`30996cc\` | 2025-10-01 | Ennio Barbaro | Move tests in mc68000 | ❌ 跳过 |
| \`45fbfea\` | 2025-10-01 | Ennio Barbaro | Add bitfield tests | ❌ 跳过 |
| \`f17a970\` | 2025-10-01 | Ennio Barbaro | Fix bfchg, bfclr, bfins | ❌ 跳过（68020+ 指令，项目实现已有 CPU_TYPE_IS_EC020_PLUS 保护） |
| \`3b5356c\` | 2025-10-01 | Ennio Barbaro | Add test for long mul/div | ❌ 跳过 |
| \`a2c0a7f\` | 2025-10-01 | Ennio Barbaro | Add test for shift/trapcc | ❌ 跳过 |
| \`998400d\` | 2025-10-01 | Ennio Barbaro | Add few more tests | ❌ 跳过 |
| \`cc92676\` | 2025-10-01 | Ennio Barbaro | Add interrupt test | ❌ 跳过 |
| \`1fbe341\` | 2025-10-01 | Ennio Barbaro | Do not build UTs on all | ❌ 跳过 |
| \`15edd45\` | 2025-10-01 | Ennio Barbaro | Actually build tests | ❌ 跳过 |
| \`32cb323\` | 2025-10-01 | Ennio Barbaro | Remove commented out code | ❌ 跳过 |
| \`76ca887\` | 2025-10-07 | Ennio Barbaro | Add binary test files | ❌ 跳过 |
| \`a42faa1\` | 2025-10-07 | Ennio Barbaro | Do not build the tests with 'make test' | ❌ 跳过 |
| \`7078af3\` | 2025-10-07 | Ennio Barbaro | Add test/README.md | ❌ 跳过 |
| \`f96ade6\` | 2025-10-08 | Ennio Barbaro | test_driver: fix unnamed parameters | ❌ 跳过 |
| \`72c1d74\` | 2025-10-09 | Karl Stenerud | Merge pull request #114 from sbabbi/unit_tests | ❌ 跳过 |
| \`447d9db\` | 2026-03-04 | Chris Hanson | Prefix preprocessor macros with M68K_ | ❌ 跳过 |
| \`81f057c\` | 2026-03-04 | Chris Hanson | Wrap m68kconf.h option #defines in #ifdef/#endif | ❌ 跳过 |
| \`27e9b95\` | 2026-03-04 | Chris Hanson | Add a TRAP callback | ❌ 跳过 |
| \`4f90dcf\` | 2026-03-04 | Chris Hanson | Use ptrdiff_t for pointer arithmetic results | ✅ 无害 |
| \`b571910\` | 2026-03-08 | Karl Stenerud | Merge pull request #110 from eschaton/eschaton/symbol-prefixes | ❌ 跳过 |
| \`ae8fea1\` | 2026-03-08 | Karl Stenerud | Merge pull request #116 from eschaton/eschaton/trap-hook | ❌ 跳过 |
| \`313ebf1\` | 2026-03-08 | Karl Stenerud | Merge pull request #117 from eschaton/eschaton/ptrdiff_t | ❌ 跳过 |

### 操作步骤

```
1. ✅ 3368bed          — 已合入分支
2. ✅ 2c0f575          — 已合入分支
3. ✅ 0a75e6e          — 已合入分支
4. ✅ f0d95c3          — 已合入分支
5. ✅ 6eb0d06          — 已合入分支
6. ✅ 5d9df94          — 已手动完成
7. ❌ 7ec87cd          — 跳过（周期不影响功能）
8. ❌ f17a970          — 跳过（68020+ 位段指令，项目用 68000）
```
## 基线

当前项目 Musashi 源自 [Spritetm/minimacplus](https://github.com/Spritetm/minimacplus)，基线对应官方 repo commit [`83c920e`](https://github.com/kstenerud/Musashi/commit/83c920e893fabbb807580f8ad799e3473d613768)（2016-04-21）之后。
