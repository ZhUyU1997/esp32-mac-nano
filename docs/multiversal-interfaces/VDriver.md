# VDriver Interfaces

Source: `multiversal/defs/VDriver.yaml`

- Functions: **0**
- Typedefs: **6**
- Structs: **6**, Unions: **0**
- Enums: **0**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **0**
- Low-memory globals: **0**

## Typedefs

- **VDParamBlockPtr** = VDParamBlock*
- **VDEntRecPtr** = VDEntryRecord*
- **VDGamRecPtr** = VDGammaRecord*
- **VDPgInfoPtr** = VDPgInfo*
- **VDFlagPtr** = VDFlagRec*
- **VDDefModePtr** = VDDefModeRec*

## Structs

- **VDParamBlock** { ?: ?, ioRefNum: INTEGER, csCode: INTEGER, csParam: Ptr }
- **VDEntryRecord** { csTable: Ptr, csStart: INTEGER, csCount: INTEGER }
- **VDGammaRecord** { csGTable: Ptr }
- **VDPgInfo** { csMode: INTEGER, csData: LONGINT, csPage: INTEGER, csBaseAddr: Ptr }
- **VDFlagRec** { flag: SignedByte }
- **VDDefModeRec** { spID: SignedByte }

