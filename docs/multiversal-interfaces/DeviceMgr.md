# DeviceMgr Interfaces

DeviceMgr IMII-192 (false);

Source: `multiversal/defs/DeviceMgr.yaml`

- Functions: **9**
- Typedefs: **6**
- Structs: **4**, Unions: **0**
- Enums: **7**
- Function pointers: **1**
- Common blocks: **0**
- Dispatchers: **0**
- Low-memory globals: **12**

## Functions

### CloseDriver  

```c
OSErr CloseDriver(INTEGER rn)
```

Trap: — (executor 实现，无 trap) executor=True

### Control  

```c
OSErr Control(INTEGER rn, INTEGER code, const void* param)
```

Trap: — (executor 实现，无 trap) executor=True

### GetDCtlEntry  

```c
DCtlHandle GetDCtlEntry(INTEGER rn)
```

Trap: — (executor 实现，无 trap) executor=True

### KillIO  

```c
OSErr KillIO(INTEGER rn)
```

Trap: — (executor 实现，无 trap) executor=True

### OpenDriver  

```c
OSErr OpenDriver(ConstStringPtr name, INTEGER* rnp)
```

Trap: — (executor 实现，无 trap) executor=True

### PBControl  

```c
OSErr PBControl(ParmBlkPtr pbp, Boolean a)
```

Trap: `0xA004` executor=True

### PBKillIO  

```c
OSErr PBKillIO(ParmBlkPtr pbp, Boolean a)
```

Trap: `0xA006` executor=True

### PBStatus  

```c
OSErr PBStatus(ParmBlkPtr pbp, Boolean a)
```

Trap: `0xA005` executor=True

### Status  

```c
OSErr Status(INTEGER rn, INTEGER code, Ptr param)
```

Trap: — (executor 实现，无 trap) executor=True

## Typedefs

- **DCtlPtr** = DCtlEntry*
- **DCtlHandle** = DCtlPtr*
- **DCtlHandlePtr** = DCtlHandle*
- **umacdriverptr** = umacdriver*
- **ramdriverptr** = ramdriver*
- **ramdriverhand** = ramdriverptr*

## Enums

- **DriverRoutineType**
- **?**
- **?**
- **?**
- **?**
- **?**
- **?**

### Enum Values

**DriverRoutineType**:

- `Open` = ?
- `Prime` = ?
- `Ctl` = ?
- `Stat` = ?
- `Close` = ?

**anonymous**:

- `asyncTrpBit` = 1 << 10
- `noQueueBit` = 1 << 9

**anonymous**:

- `NEEDTIMEBIT` = 1 << 13

**anonymous**:

- `aRdCmd` = 2
- `aWrCmd` = 3

**anonymous**:

- `killCode` = 1

**anonymous**:

- `NDEVICES` = 48

**anonymous**:

- `abortErr` = -27
- `badUnitErr` = -21
- `controlErr` = -17
- `dInstErr` = -26
- `dRemovErr` = -25
- `notOpenErr` = -28
- `openErr` = -23
- `readErr` = -19
- `statusErr` = -18
- `unitEmptyErr` = -22
- `writErr` = -20

## Structs

- **DCtlEntry** {  }
- **umacdriver** { udrvrOpen: DriverUPP, udrvrPrime: DriverUPP, udrvrCtl: DriverUPP, udrvrStatus: DriverUPP, udrvrClose: DriverUPP, udrvrName: Str255 }
- **ramdriver** { drvrFlags: INTEGER, drvrDelay: INTEGER, drvrEMask: INTEGER, drvrMenu: INTEGER, drvrOpen: INTEGER, drvrPrime: INTEGER, drvrCtl: INTEGER, drvrStatus: INTEGER, drvrClose: INTEGER, drvrName: char }
- **DCtlEntry** { dCtlDriver: umacdriverptr, dCtlFlags: INTEGER, dCtlQHdr: QHdr, dCtlPosition: LONGINT, dCtlStorage: Handle, dCtlRefNum: INTEGER, dCtlCurTicks: LONGINT, dCtlWindow: WindowPtr, dCtlDelay: INTEGER, dCtlEMask: INTEGER, dCtlMenu: INTEGER }

## Function Pointers

- **DriverUPP** (?: ParmBlkPtr, ?: DCtlPtr) -> OSErr

## Low Memory Globals

- **UTableBase** @ 0x11C (DCtlHandlePtr) — DeviceMgr IMII-192 (false);
- **Lvl1DT** @ 0x192 (ProcPtr[8]) — DeviceMgr IMII-197 (false);
- **Lvl2DT** @ 0x1B2 (ProcPtr[8]) — DeviceMgr IMII-198 (false);
- **UnitNtryCnt** @ 0x1D2 (INTEGER) — DeviceMgr ThinkC (true-b);
- **VIA** @ 0x1D4 (Ptr) — DeviceMgr IMIII-39 (true-b);
- **SCCRd** @ 0x1D8 (Ptr) — DeviceMgr IMII-199 (false);
- **SCCWr** @ 0x1DC (Ptr) — DeviceMgr IMII-199 (false);
- **IWM** @ 0x1E0 (Ptr) — DeviceMgr ThinkC (false);
- **ExtStsDT** @ 0x2BE (ProcPtr[4]) — DeviceMgr IMII-199 (false);
- **JFetch** @ 0x8F4 (Ptr) — DeviceMgr IMII-194 (false);
- **JStash** @ 0x8F8 (Ptr) — DeviceMgr IMII-195 (false);
- **JIODone** @ 0x8FC (Ptr) — DeviceMgr IMII-195 (false);
