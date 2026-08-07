# LowMem Interfaces

* NOTE: MacWrite starts writing longwords at location 0x80 for TRAPs rsys/misc WHOKNOWS (true-b);

Source: `multiversal/defs/LowMem.yaml`

- Functions: **0**
- Typedefs: **0**
- Structs: **0**, Unions: **0**
- Enums: **0**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **0**
- Low-memory globals: **21**

## Low Memory Globals

- **nilhandle** @ 0x0 (Ptr) — Declarations of low memory globals which haven't been put anywhere else yet follow. Whenever a low memory global clearly belongs to one manager/module, put it should live in the appropriate header. There is also a complete list of low memory globals in globals.cpp, which is not used but might be nice to have. rsys/misc MADEUP (true-b);
- **trapvectors** @ 0x80 (LONGINT[10]) — * NOTE: MacWrite starts writing longwords at location 0x80 for TRAPs rsys/misc WHOKNOWS (true-b);
- **dodusesit** @ 0xE4 (Ptr) — rsys/misc WHOKNOWS (true-b);
- **hyperlong** @ 0x1AA (LONGINT) — * Hypercard does a movel to this location. rsys/misc WHOKNOWS (true-b);
- **mathones** @ 0x282 (LONGINT) — * NOTE: mathones is a LONGINT that Mathematica looks at that contains -1 * on a Mac+ rsys/misc WHOKNOWS (true-b);
- **ROM85** @ 0x28E (INTEGER) — * NOTE: Theoretically ROM85 is mentioned in IMV, but I don't know where. * On a Mac+ the value 0x7FFF is stored there. * tim: It is at least on page IMV-328. MacTypes IMV-328 (true-b);
- **BufTgFNum** @ 0x2FC (LONGINT) — DiskDvr IMII-212 (false);
- **BufTgFFlg** @ 0x300 (INTEGER) — DiskDvr IMII-212 (false);
- **BufTgFBkNum** @ 0x302 (INTEGER) — DiskDvr IMII-212 (false);
- **BufTgDate** @ 0x304 (LONGINT) — DiskDvr IMII-212 (false);
- **MCLKPCmiss1** @ 0x3A0 (INTEGER) — MacLinkPC badaccess (true-b);
- **MCLKPCmiss2** @ 0x3A6 (INTEGER) — MacLinkPC badaccess (true-b);
- **JFLUSH** @ 0x6F4 (VoidUPP) — * JFLUSH is a guess from disassembling some of Excel 3.0 idunno guess (true-b);
- **JResUnknown1** @ 0x700 (VoidUPP) — idunno resedit (true-b);
- **JResUnknown2** @ 0x714 (VoidUPP) — idunno resedit (true-b);
- **graphlooksat** @ 0x952 (INTEGER) — * NOTE: The graphing program looks for a -1 in 0x952 rsys/misc WHOKNOWS (true-b);
- **macwritespace** @ 0x954 (LONGINT) — * NOTE: MacWrite stores a copy of the trap address for LoadSeg in 954 rsys/misc WHOKNOWS (true-b);
- **DSErrCode** @ 0xAF0 (INTEGER) — MacTypes IMII-362 (true);
- **SCSIFlags** @ 0xB22 (INTEGER) — uknown Private.a (true-b);
- **LastSPExtra** @ 0xB4C (LONGINT) — rsys/misc WHOKNOWS (true-b);
- **lastlowglobal** @ 0x2000 (LONGINT) — rsys/misc MadeUp (true-b);
