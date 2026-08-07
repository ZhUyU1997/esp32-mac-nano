# SegmentLdr Interfaces

SegmentLdr SysEqu.a (true-b);

Source: `multiversal/defs/SegmentLdr.yaml`

- Functions: **6**
- Typedefs: **0**
- Structs: **1**, Unions: **0**
- Enums: **2**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **0**
- Low-memory globals: **7**

## Functions

### ClrAppFiles  

```c
void ClrAppFiles(INTEGER index)
```

Trap: — (executor 实现，无 trap)

### CountAppFiles  

```c
void CountAppFiles(INTEGER* messagep, INTEGER* countp)
```

Trap: — (executor 实现，无 trap)

### ExitToShell  

```c
void ExitToShell()
```

Trap: `0xA9F4` executor=C_

### GetAppFiles  

```c
void GetAppFiles(INTEGER index, AppFile* filep)
```

Trap: — (executor 实现，无 trap)

### GetAppParms  

```c
void GetAppParms(StringPtr namep, INTEGER* rnp, Handle* aphandp)
```

Trap: `0xA9F5` executor=C_

### UnloadSeg  

```c
void UnloadSeg(void* addr)
```

Trap: `0xA9F1` executor=C_

## Enums

- **?**
- **?**

### Enum Values

**anonymous**:

- `appOpen` = 0
- `appPrint` = 1

**anonymous**:

- `_LoadSeg` = 43504
- `_UnLoadSeg` = 43505
- `_Launch` = 43506
- `_Chain` = 43507

## Structs

- **AppFile** { vRefNum: INTEGER, fType: OSType, versNum: INTEGER, fName: Str255 }

## Low Memory Globals

- **loadtrap** @ 0x12D (Byte) — SegmentLdr SysEqu.a (true-b);
- **FinderName** @ 0x2E0 (Byte[16]) — SegmentLdr IMII-59 (true);
- **CurApRefNum** @ 0x900 (INTEGER) — SegmentLdr IMII-58 (true);
- **CurApName** @ 0x910 (Byte[34]) — * NOTE: IMIII says CurApName is 32 bytes long, but it looks to me like * it is really 34 bytes long. SegmentLdr IMII-58 (true);
- **CurJTOffset** @ 0x934 (INTEGER) — SegmentLdr IMII-62 (true-b);
- **CurPageOption** @ 0x936 (INTEGER) — SegmentLdr IMII-60 (true);
- **AppParmHandle** @ 0xAEC (Handle) — SegmentLdr IMII-57 (true);
