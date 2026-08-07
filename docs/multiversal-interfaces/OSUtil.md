# OSUtil Interfaces

extern LONGINT GetTrapAddress(INTEGER n); // 68K in emustubs, not supported on ppc extern void SetTrapAddress(LONGINT addr, INTEGER n);

Source: `multiversal/defs/OSUtil.yaml`

- Functions: **50**
- Typedefs: **3**
- Structs: **3**, Unions: **1**
- Enums: **10**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **0**
- Low-memory globals: **20**

## Functions

### CopyCStringToPascal  

```c
void CopyCStringToPascal(const char* src, Str255 dst)
```

Trap: — (executor 实现，无 trap)

### CopyPascalStringToC  

```c
void CopyPascalStringToC(ConstStr255Param src, char * dst)
```

Trap: — (executor 实现，无 trap)

### DateToSeconds  

```c
void DateToSeconds(DateTimeRec* d, ULONGINT* s)
```

Trap: `0xA9C7` executor=True

### DebugStr  

```c
void DebugStr(ConstStringPtr p)
```

Trap: `0xABFF` executor=C_

### Debugger  

```c
void Debugger()
```

Trap: `0xA9FF` executor=C_

### Delay  

```c
void Delay(LONGINT n, LONGINT* ftp)
```

Trap: `0xA03B` executor=True

### Dequeue  

```c
OSErr Dequeue(QElemPtr e, QHdrPtr h)
```

Trap: `0xA96E` executor=True

### Enqueue  

```c
void Enqueue(QElemPtr e, QHdrPtr h)
```

Trap: `0xA96F` executor=True

### Environs  

```c
void Environs(INTEGER* rom, INTEGER* machine)
```

Trap: — (executor 实现，无 trap)

### EqualString  

```c
Boolean EqualString(ConstStringPtr s1, ConstStringPtr s2, Boolean casesig, Boolean diacsig)
```

Trap: — (executor 实现，无 trap)

### FlushCodeCache  

```c
void FlushCodeCache()
```

Trap: `0xA0BD` executor=C_

### GetDateTime  

```c
void GetDateTime(ULONGINT* mactimepointer)
```

Trap: — (executor 实现，无 trap)

### GetMMUMode  

```c
Byte GetMMUMode()
```

Trap: — (executor 实现，无 trap) executor=True

### GetSysPPtr  

```c
SysPPtr GetSysPPtr()
```

Trap: — (executor 实现，无 trap)

### GetTime  

```c
void GetTime(DateTimeRec* d)
```

Trap: — (executor 实现，无 trap)

### GetToolboxTrapAddress  

```c
ProcPtr GetToolboxTrapAddress(uint16_t n)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetTrapAddress  

```c
ProcPtr GetTrapAddress(uint16_t n, bool newTraps, bool tool)
```

Trap: `0xA146` executor=_GetTrapAddress_flags

### HWPriv  

```c
void HWPriv(LONGINT d0, LONGINT a0)
```

Trap: `0xA198` executor=True

### HandAndHand  

```c
OSErr HandAndHand(Handle h1, Handle h2)
```

Trap: `0xA9E4` executor=True

### HandToHand  

```c
OSErr HandToHand(Handle* h)
```

Trap: `0xA9E1` executor=True

### InitUtil  

```c
OSErr InitUtil()
```

Trap: `0xA03F` executor=True

### MakeDataExecutable  

```c
void MakeDataExecutable(void* ptr, uint32_t sz)
```

Trap: — (executor 实现，无 trap) executor=C_

### NGetTrapAddress  

```c
ProcPtr NGetTrapAddress(uint16_t n, TrapType ttype)
```

Trap: — (executor 实现，无 trap) executor=C_ — extern LONGINT GetTrapAddress(INTEGER n); // 68K in emustubs, not supported on ppc extern void SetTrapAddress(LONGINT addr, INTEGER n);

### NSetTrapAddress  

```c
void NSetTrapAddress(ProcPtr addr, uint16_t n, TrapType ttype)
```

Trap: — (executor 实现，无 trap) executor=C_

### PtrAndHand  

```c
OSErr PtrAndHand(const void* p, Handle h, LONGINT s1)
```

Trap: `0xA9EF` executor=True

### PtrToHand  

```c
OSErr PtrToHand(const void* p, Handle* h, LONGINT s)
```

Trap: `0xA9E3` executor=True

### PtrToXHand  

```c
OSErr PtrToXHand(const void* p, Handle h, LONGINT s)
```

Trap: `0xA9E2` executor=True

### ROMlib_RelString  

```c
LONGINT ROMlib_RelString(const uint8_t* s1, const uint8_t* s2, Boolean casesig, Boolean diacsig, LONGINT d0)
```

Trap: — (executor 实现，无 trap)

### ROMlib_UprString  

```c
void ROMlib_UprString(StringPtr s, Boolean diac, INTEGER len)
```

Trap: — (executor 实现，无 trap)

### ReadDateTime  

```c
OSErr ReadDateTime(ULONGINT* secs)
```

Trap: `0xA039` executor=True

### RelString  

```c
INTEGER RelString(ConstStringPtr s1, ConstStringPtr s2, Boolean casesig, Boolean diacsig)
```

Trap: — (executor 实现，无 trap)

### Restart  

```c
void Restart()
```

Trap: — (executor 实现，无 trap)

### RestoreA5  

```c
void RestoreA5()
```

Trap: — (executor 实现，无 trap)

### SecondsToDate  

```c
void SecondsToDate(ULONGINT mactime, DateTimeRec* d)
```

Trap: `0xA9C6` executor=True

### SetA5  

```c
LONGINT SetA5(LONGINT a5)
```

Trap: — (executor 实现，无 trap)

### SetCurrentA5  

```c
LONGINT SetCurrentA5()
```

Trap: — (executor 实现，无 trap)

### SetDateTime  

```c
OSErr SetDateTime(ULONGINT mactime)
```

Trap: `0xA03A` executor=True

### SetTime  

```c
void SetTime(DateTimeRec* d)
```

Trap: — (executor 实现，无 trap)

### SetToolboxTrapAddress  

```c
void SetToolboxTrapAddress(ProcPtr addr, uint16_t n)
```

Trap: — (executor 实现，无 trap) executor=C_

### SetTrapAddress  

```c
void SetTrapAddress(ProcPtr addr, uint16_t n, bool newTraps, bool tool)
```

Trap: `0xA047` executor=_SetTrapAddress_flags

### SetUpA5  

```c
void SetUpA5()
```

Trap: — (executor 实现，无 trap)

### StripAddress  

```c
uint32_t StripAddress(uint32_t l)
```

Trap: `0xA055` executor=True

### StripAddress  

```c
Ptr StripAddress(Ptr p)
```

Trap: `0xA055`

### SwapMMUMode  

```c
void SwapMMUMode(Byte* bp)
```

Trap: `0xA05D` executor=True

### SysBeep  

```c
void SysBeep(INTEGER i)
```

Trap: `0xA9C8` executor=C_

### SysEnvirons  

```c
OSErr SysEnvirons(INTEGER vers, SysEnvRecPtr p)
```

Trap: `0xA090` executor=True

### UpperString  

```c
void UpperString(StringPtr s, Boolean diac)
```

Trap: — (executor 实现，无 trap)

### WriteParam  

```c
OSErr WriteParam()
```

Trap: `0xA038` executor=True

### c2pstrcpy  

```c
void c2pstrcpy(Str255 dst, const char* src)
```

Trap: — (executor 实现，无 trap)

### p2cstrcpy  

```c
void p2cstrcpy(char * dst, ConstStr255Param src)
```

Trap: — (executor 实现，无 trap)

## Typedefs

- **SysPPtr** = SysParmType*
- **SysEnvRecPtr** = SysEnvRec*
- **TrapType** = SignedByte

## Enums

- **?**
- **?**
- **QTypes**
- **?**
- **?** — sysEnv machine types
- **?**
- **?**
- **?**
- **?**
- **?**

### Enum Values

**anonymous**:

- `macXLMachine` = 0
- `macMachine` = 1
- `UNIXMachine` = 1127

**anonymous**:

- `clkRdErr` = -85
- `clkWrErr` = -86
- `prInitErr` = -88
- `prWrErr` = -87
- `hwParamErr` = -502

**QTypes**:

- `dummyType` = ?
- `vType` = ?
- `ioQType` = ?
- `drvQType` = ?
- `evType` = ?
- `fsQType` = ?

**anonymous**:

- `curSysEnvVers` = 2

**anonymous** — sysEnv machine types:

- `envMachUnknown` = 0
- `env512KE` = 1
- `envMacPlus` = 2
- `envSE` = 3
- `envMacII` = 4
- `envMac` = -1
- `envXL` = -2

**anonymous**:

- `envCPUUnknown` = 0
- `env68000` = 1
- `env68020` = 3
- `env68030` = 4
- `env68040` = 5

**anonymous**:

- `envUnknownKbd` = 0
- `envMacKbd` = 1
- `envMacAndPad` = 2
- `envMacPlusKbd` = 3
- `envAExtendKbd` = 4
- `envStandADBKbd` = 5

**anonymous**:

- `envBadVers` = -5501
- `envVersTooBig` = -5502

**anonymous**:

- `kOSTrapType` = ?
- `kToolboxTrapType` = ?

**anonymous**:

- `_Unimplemented` = 43167

## Structs

- **SysParmType** { valid: Byte, aTalkA: Byte, aTalkB: Byte, config: Byte, portA: INTEGER, portB: INTEGER, alarm: LONGINT, font: INTEGER, kbdPrint: INTEGER, volClik: INTEGER, misc: INTEGER }
- **DateTimeRec** { year: INTEGER, month: INTEGER, day: INTEGER, hour: INTEGER, minute: INTEGER, second: INTEGER, dayOfWeek: INTEGER }
- **SysEnvRec** { environsVersion: INTEGER, machineType: INTEGER, systemVersion: INTEGER, processor: INTEGER, hasFPU: Boolean, hasColorQD: Boolean, keyBoardType: INTEGER, atDrvrVersNum: INTEGER, sysVRefNum: INTEGER }

## Unions

- **QElem** { vblQElem: VBLTask, ioQElem: ParamBlockRec, drvQElem: DrvQEl, evQElem: EvQEl, vcbQElem: VCB }

## Low Memory Globals

- **SysVersion** @ 0x15A (INTEGER) — OSUtil ThinkC (true);
- **SPValid** @ 0x1F8 (Byte) — OSUtil IMII-392 (true);
- **SPATalkA** @ 0x1F9 (Byte) — OSUtil IMII-392 (true);
- **SPATalkB** @ 0x1FA (Byte) — OSUtil IMII-392 (true);
- **SPConfig** @ 0x1FB (Byte) — OSUtil IMII-392 (true);
- **SPPortA** @ 0x1FC (INTEGER) — OSUtil IMII-392 (true);
- **SPPortB** @ 0x1FE (INTEGER) — OSUtil IMII-392 (true);
- **SPAlarm** @ 0x200 (LONGINT) — OSUtil IMII-392 (true);
- **SPFont** @ 0x204 (INTEGER) — OSUtil IMII-392 (true);
- **SPKbd** @ 0x206 (Byte) — OSUtil IMII-369 (true);
- **SPPrint** @ 0x207 (Byte) — OSUtil IMII-392 (true);
- **SPVolCtl** @ 0x208 (Byte) — OSUtil IMII-392 (true);
- **SPClikCaret** @ 0x209 (Byte) — OSUtil IMII-392 (true);
- **SPMisc2** @ 0x20B (Byte) — OSUtil IMII-392 (true);
- **Time** @ 0x20C (ULONGINT) — OSUtil IMI-260 (true);
- **CrsrThresh** @ 0x8EC (INTEGER) — OSUtil IMII-372 (false);
- **MMUType** @ 0xCB1 (Byte) — OSUtil MPW (false);
- **MMU32Bit** @ 0xCB2 (Byte) — OSUtil IMV-592 (true-b);
- **DTQueue** @ 0xD92 (QHdr) — OSUtil IMV-466 (false);
- **JDTInstall** @ 0xD9C (ProcPtr) — OSUtil IMV (false);
