# MemoryMgr Interfaces

temporary memory functions; see tempmem.c

Source: `multiversal/defs/MemoryMgr.yaml`

- Functions: **55**
- Typedefs: **1**
- Structs: **2**, Unions: **0**
- Enums: **3**
- Function pointers: **1**
- Common blocks: **0**
- Dispatchers: **1**
- Low-memory globals: **25**

## Functions

### ApplicationZone  

```c
THz ApplicationZone()
```

Trap: — (executor 实现，无 trap) executor=C_

### BlockMove  

```c
void BlockMove(const void* src, void* dst, Size cnt, bool flush_p)
```

Trap: `0xA02E` executor=_BlockMove_flags

### CompactMem  

```c
Size CompactMem(Size sizeneeded, bool sys_p)
```

Trap: `0xA04C` executor=_CompactMem_flags

### DisposeHandle  

```c
void DisposeHandle(Handle h)
```

Trap: `0xA023` executor=True

### DisposePtr  

```c
void DisposePtr(Ptr p)
```

Trap: `0xA01F` executor=True

### EmptyHandle  

```c
void EmptyHandle(Handle h)
```

Trap: `0xA02B` executor=True

### FreeMem  

```c
int32_t FreeMem(bool sys_p)
```

Trap: `0xA01C` executor=_FreeMem_flags

### GZSaveHnd  

```c
Handle GZSaveHnd()
```

Trap: — (executor 实现，无 trap) executor=C_

### GetApplLimit  

```c
Ptr GetApplLimit()
```

Trap: — (executor 实现，无 trap) executor=C_

### GetHandleSize  

```c
Size GetHandleSize(Handle h)
```

Trap: `0xA025` executor=True

### GetPtrSize  

```c
Size GetPtrSize(Ptr p)
```

Trap: `0xA021` executor=True

### GetZone  

```c
THz GetZone()
```

Trap: `0xA11A` executor=True

### HClrRBit  

```c
void HClrRBit(Handle h)
```

Trap: `0xA068` executor=True

### HGetState  

```c
SignedByte HGetState(Handle h)
```

Trap: `0xA069` executor=True

### HLock  

```c
void HLock(Handle h)
```

Trap: `0xA029` executor=True

### HLockHi  

```c
void HLockHi(Handle h)
```

Trap: — (executor 实现，无 trap) executor=C_

### HNoPurge  

```c
void HNoPurge(Handle h)
```

Trap: `0xA04A` executor=True

### HPurge  

```c
void HPurge(Handle h)
```

Trap: `0xA049` executor=True

### HSetRBit  

```c
void HSetRBit(Handle h)
```

Trap: `0xA067` executor=True

### HSetState  

```c
void HSetState(Handle h, SignedByte flags)
```

Trap: `0xA06A` executor=True

### HUnlock  

```c
void HUnlock(Handle h)
```

Trap: `0xA02A` executor=True

### HandleZone  

```c
THz HandleZone(Handle h)
```

Trap: `0xA126` executor=True

### InitApplZone  

```c
void InitApplZone()
```

Trap: `0xA02C` executor=True

### InitZone  

```c
void InitZone(GrowZoneUPP pGrowZone, int16_t cMoreMasters, Ptr limitPtr, THz startPtr)
```

Trap: — (executor 实现，无 trap) executor=True

### MaxApplZone  

```c
void MaxApplZone()
```

Trap: `0xA063` executor=True

### MaxBlock  

```c
Size MaxBlock(bool sys_p)
```

Trap: `0xA061` executor=_MaxBlock_flags

### MaxMem  

```c
Size MaxMem(Size* growp, bool sys_p)
```

Trap: `0xA11D` executor=_MaxMem_flags

### MemError  

```c
OSErr MemError()
```

Trap: — (executor 实现，无 trap) executor=C_

### MoreMasters  

```c
void MoreMasters()
```

Trap: `0xA036` executor=True

### MoveHHi  

```c
void MoveHHi(Handle h)
```

Trap: `0xA064` executor=True

### NewEmptyHandle  

```c
Handle NewEmptyHandle(bool sys_p)
```

Trap: `0xA166` executor=_NewEmptyHandle_flags

### NewHandle  

```c
Handle NewHandle(Size size, bool sys_p, bool clear_p)
```

Trap: `0xA122` executor=_NewHandle_flags

### NewPtr  

```c
Ptr NewPtr(Size size, bool sys_p, bool clear_p)
```

Trap: `0xA11E` executor=_NewPtr_flags

### PtrZone  

```c
THz PtrZone(Ptr p)
```

Trap: `0xA148` executor=True

### PurgeMem  

```c
void PurgeMem(Size needed, bool sys_p)
```

Trap: `0xA04D` executor=_PurgeMem_flags

### PurgeSpace  

```c
void PurgeSpace(Size* totalp, Size* contigp, bool sys_p)
```

Trap: `0xA062` executor=_PurgeSpace_flags

### ReallocateHandle  

```c
void ReallocateHandle(Handle h, Size size)
```

Trap: `0xA027` executor=True

### RecoverHandle  

```c
Handle RecoverHandle(Ptr p, bool sys_p)
```

Trap: `0xA128` executor=_RecoverHandle_flags

### ReserveMem  

```c
void ReserveMem(Size needed, bool sys_p)
```

Trap: `0xA040` executor=_ResrvMem_flags

### SetApplBase  

```c
void SetApplBase(Ptr newbase)
```

Trap: `0xA057` executor=True

### SetApplLimit  

```c
void SetApplLimit(Ptr newlimit)
```

Trap: `0xA02D` executor=True

### SetGrowZone  

```c
void SetGrowZone(GrowZoneUPP newgz)
```

Trap: `0xA04B` executor=True

### SetHandleSize  

```c
void SetHandleSize(Handle h, Size newsize)
```

Trap: `0xA024` executor=True

### SetPtrSize  

```c
void SetPtrSize(Ptr p, Size newsize)
```

Trap: `0xA020` executor=True

### SetZone  

```c
void SetZone(THz hz)
```

Trap: `0xA01B` executor=True

### StackSpace  

```c
Size StackSpace()
```

Trap: `0xA065` executor=True

### SystemZone  

```c
THz SystemZone()
```

Trap: — (executor 实现，无 trap) executor=C_

### TempDisposeHandle  

```c
void TempDisposeHandle(Handle h, OSErr* result_code)
```

Trap: — (executor 实现，无 trap) executor=C_

### TempFreeMem  

```c
int32_t TempFreeMem()
```

Trap: — (executor 实现，无 trap) executor=C_ — temporary memory functions; see tempmem.c

### TempHLock  

```c
void TempHLock(Handle h, OSErr* result_code)
```

Trap: — (executor 实现，无 trap) executor=C_

### TempHUnlock  

```c
void TempHUnlock(Handle h, OSErr* result_code)
```

Trap: — (executor 实现，无 trap) executor=C_

### TempMaxMem  

```c
Size TempMaxMem(Size* grow)
```

Trap: — (executor 实现，无 trap) executor=C_

### TempNewHandle  

```c
Handle TempNewHandle(Size logical_size, OSErr* result_code)
```

Trap: — (executor 实现，无 trap) executor=C_

### TempTopMem  

```c
Ptr TempTopMem()
```

Trap: — (executor 实现，无 trap) executor=C_

### TopMem  

```c
Ptr TopMem()
```

Trap: — (executor 实现，无 trap) executor=C_

## Typedefs

- **THz** = Zone*

## Enums

- **?**
- **?**
- **?**

### Enum Values

**anonymous**:

- `memFullErr` = -108
- `memLockedErr` = -117
- `memPurErr` = -112
- `memWZErr` = -111

**anonymous**:

- `nilHandleErr` = -109

**anonymous**:

- `memROZErr` = -99
- `memAdrErr` = -110
- `memAZErr` = -113
- `memPCErr` = -114
- `memBCErr` = -115
- `memSCErr` = -116

## Structs

- **block_header_t** {  }
- **Zone** { bkLim: Ptr, purgePtr: Ptr, hFstFree: Ptr, zcbFree: LONGINT, gzProc: GrowZoneUPP, moreMast: INTEGER, flags: INTEGER, cntRel: INTEGER, maxRel: INTEGER, cntNRel: INTEGER, maxNRel: INTEGER, cntEmpty: INTEGER, cntHandles: INTEGER, minCBFree: LONGINT, purgeProc: ProcPtr, sparePtr: Ptr, allocPtr: block_header_t*, heapData: INTEGER }

## Function Pointers

- **GrowZoneUPP** (?: Size) -> LONGINT

## Dispatchers

- **OSDispatch**—

## Low Memory Globals

- **MemTop** @ 0x108 (Ptr) — MemoryMgr IMII-19 (true);
- **BufPtr** @ 0x10C (Ptr) — MemoryMgr IMII-19 (true-b);
- **HeapEnd** @ 0x114 (Ptr) — MemoryMgr IMII-19 (true);
- **TheZone** @ 0x118 (THz) — MemoryMgr IMII-31 (true);
- **ApplLimit** @ 0x130 (Ptr) — MemoryMgr IMII-19 (true);
- **MemErr** @ 0x220 (INTEGER) — MemoryMgr IMIV-80 (true);
- **SysZone** @ 0x2A6 (THz) — MemoryMgr IMII-19 (true);
- **ApplZone** @ 0x2AA (THz) — MemoryMgr IMII-19 (true);
- **ROMBase** @ 0x2AE (Ptr) — MemoryMgr IMIV-236 (true-b);
- **RAMBase** @ 0x2B2 (Ptr) — MemoryMgr IMI-87 (false);
- **heapcheck** @ 0x316 (Ptr) — MemoryMgr SysEqu.a (true-b);
- **Lo3Bytes** @ 0x31A (LONGINT) — MemoryMgr IMI-85 (true);
- **MinStack** @ 0x31E (LONGINT) — MemoryMgr IMII-17 (true-b);
- **DefltStack** @ 0x322 (LONGINT) — MemoryMgr IMII-17 (true-b);
- **GZRootHnd** @ 0x328 (Handle) — MemoryMgr IMI-43 (true);
- **GZMoveHnd** @ 0x330 (Handle) — MemoryMgr LowMem.h (false);
- **IAZNotify** @ 0x33C (ProcPtr) — MemoryMgr ThinkC (true-b);
- **CurrentA5** @ 0x904 (Ptr) — MemoryMgr IMI-95 (true);
- **CurStackBase** @ 0x908 (Ptr) — MemoryMgr IMII-19 (true-b);
- **Scratch20** @ 0x1E4 (Byte[20]) — MemoryMgr IMI-85 (true);
- **ToolScratch** @ 0x9CE (Byte[8]) — MemoryMgr IMI-85 (true);
- **Scratch8** @ 0x9FA (Byte[8]) — MemoryMgr IMI-85 (true);
- **OneOne** @ 0xA02 (LONGINT) — MemoryMgr IMI-85 (true);
- **MinusOne** @ 0xA06 (LONGINT) — MemoryMgr IMI-85 (true);
- **ApplScratch** @ 0xA78 (Byte[12]) — MemoryMgr IMI-85 (true);
