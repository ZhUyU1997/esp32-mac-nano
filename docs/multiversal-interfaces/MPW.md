# MPW Interfaces

Source: `multiversal/defs/MPW.yaml`

- Functions: **0**
- Typedefs: **0**
- Structs: **7**, Unions: **0**
- Enums: **0**
- Function pointers: **4**
- Common blocks: **0**
- Dispatchers: **0**
- Low-memory globals: **1**

## Structs

- **MPWFile** {  }
- **fsysTable** { magic: OSType, access: MPWAccessProcPtr, close: MPWFileProcPtr, read: MPWFileProcPtr, write: MPWFileProcPtr, ioctl: MPWIOCtlProcPtr }
- **MPWFile** { flags: INTEGER, err: INTEGER, functions: fsysTable*, cookie: LONGINT, count: LONGINT, buffer: void* }
- **MPWSeekParamBlock** { whence: LONGINT, offset: LONGINT }
- **devtable** { fsys: fsysTable, econ: fsysTable, syst: fsysTable }
- **PgmInfo2** { magic2: INTEGER, argc: LONGINT, argv: char**, envp: char**, exitCode: LONGINT, x: LONGINT, y: LONGINT, tableSize: INTEGER, ioptr: MPWFile*, devptr: devtable* }
- **PgmInfo1** { magic: OSType, pgmInfo2: PgmInfo2* }

## Function Pointers

- **MPWFileProcPtr** (file: MPWFile*) -> LONGINT
- **MPWQuitProcPtr** () -> void
- **MPWAccessProcPtr** (name: char*, op: LONGINT, file: MPWFile*) -> LONGINT
- **MPWIOCtlProcPtr** (file: MPWFile*, cmd: LONGINT, param: void*) -> LONGINT

## Low Memory Globals

- **MacPgm** @ 0x316 (PgmInfo1*)
