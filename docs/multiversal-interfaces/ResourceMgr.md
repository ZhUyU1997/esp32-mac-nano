# ResourceMgr Interfaces

resource attribute masks

Source: `multiversal/defs/ResourceMgr.yaml`

- Functions: **48**
- Typedefs: **0**
- Structs: **0**, Unions: **0**
- Enums: **7**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **1**
- Low-memory globals: **11**

## Functions

### AddResource  

```c
void AddResource(Handle data, ResType typ, INTEGER id, ConstStringPtr name)
```

Trap: `0xA9AB` executor=C_

### ChangedResource  

```c
void ChangedResource(Handle res)
```

Trap: `0xA9AA` executor=C_

### CloseResFile  

```c
void CloseResFile(INTEGER rn)
```

Trap: `0xA99A` executor=C_

### Count1Resources  

```c
INTEGER Count1Resources(ResType typ)
```

Trap: `0xA80D` executor=C_

### Count1Types  

```c
INTEGER Count1Types()
```

Trap: `0xA81C` executor=C_

### CountResources  

```c
INTEGER CountResources(ResType typ)
```

Trap: `0xA99C` executor=C_

### CountTypes  

```c
INTEGER CountTypes()
```

Trap: `0xA99E` executor=C_

### CreateResFile  

```c
void CreateResFile(ConstStringPtr fn)
```

Trap: `0xA9B1` executor=C_

### CurResFile  

```c
INTEGER CurResFile()
```

Trap: `0xA994` executor=C_

### DetachResource  

```c
void DetachResource(Handle res)
```

Trap: `0xA992` executor=C_

### Get1IndResource  

```c
Handle Get1IndResource(ResType typ, INTEGER i)
```

Trap: `0xA80E` executor=C_

### Get1IndType  

```c
void Get1IndType(ResType* typ, INTEGER indx)
```

Trap: `0xA80F` executor=C_

### Get1NamedResource  

```c
Handle Get1NamedResource(ResType typ, ConstStringPtr s)
```

Trap: `0xA820` executor=C_

### Get1Resource  

```c
Handle Get1Resource(ResType typ, INTEGER id)
```

Trap: `0xA81F` executor=C_

### GetIndResource  

```c
Handle GetIndResource(ResType typ, INTEGER indx)
```

Trap: `0xA99D` executor=C_

### GetIndType  

```c
void GetIndType(ResType* typ, INTEGER indx)
```

Trap: `0xA99F` executor=C_

### GetMaxResourceSize  

```c
LONGINT GetMaxResourceSize(Handle h)
```

Trap: `0xA821` executor=C_

### GetNamedResource  

```c
Handle GetNamedResource(ResType typ, ConstStringPtr nam)
```

Trap: `0xA9A1` executor=C_

### GetNextFOND  

```c
Handle GetNextFOND(Handle fondHandle)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetResAttrs  

```c
INTEGER GetResAttrs(Handle res)
```

Trap: `0xA9A6` executor=C_

### GetResFileAttrs  

```c
INTEGER GetResFileAttrs(INTEGER rn)
```

Trap: `0xA9F6` executor=C_

### GetResInfo  

```c
void GetResInfo(Handle res, INTEGER* id1, ResType* typ, StringPtr name)
```

Trap: `0xA9A8` executor=C_

### GetResource  

```c
Handle GetResource(ResType typ, INTEGER id)
```

Trap: `0xA9A0` executor=C_

### GetResourceSizeOnDisk  

```c
LONGINT GetResourceSizeOnDisk(Handle res)
```

Trap: `0xA9A5` executor=C_

### HomeResFile  

```c
INTEGER HomeResFile(Handle res)
```

Trap: `0xA9A4` executor=C_

### InitResources  

```c
INTEGER InitResources()
```

Trap: `0xA995` executor=C_

### LoadResource  

```c
void LoadResource(Handle res)
```

Trap: `0xA9A2` executor=C_

### OpenRFPerm  

```c
INTEGER OpenRFPerm(ConstStringPtr fn, INTEGER vref, Byte perm)
```

Trap: `0xA9C4` executor=C_

### OpenResFile  

```c
INTEGER OpenResFile(ConstStringPtr fn)
```

Trap: `0xA997` executor=C_

### RGetResource  

```c
Handle RGetResource(ResType typ, INTEGER id)
```

Trap: `0xA80C` executor=C_

### ReadPartialResource  

```c
void ReadPartialResource(Handle resource, int32_t offset, Ptr buffer, int32_t count)
```

Trap: — (executor 实现，无 trap) executor=C_

### ReleaseResource  

```c
void ReleaseResource(Handle res)
```

Trap: `0xA9A3` executor=C_

### RemoveResource  

```c
void RemoveResource(Handle res)
```

Trap: `0xA9AD` executor=C_

### ResError  

```c
INTEGER ResError()
```

Trap: `0xA9AF` executor=C_

### RsrcMapEntry  

```c
LONGINT RsrcMapEntry(Handle h)
```

Trap: `0xA9C5` executor=C_

### RsrcZoneInit  

```c
void RsrcZoneInit()
```

Trap: `0xA996` executor=C_

### SetResAttrs  

```c
void SetResAttrs(Handle res, INTEGER attrs)
```

Trap: `0xA9A7` executor=C_

### SetResFileAttrs  

```c
void SetResFileAttrs(INTEGER rn, INTEGER attrs)
```

Trap: `0xA9F7` executor=C_

### SetResInfo  

```c
void SetResInfo(Handle res, INTEGER id, ConstStringPtr name)
```

Trap: `0xA9A9` executor=C_

### SetResLoad  

```c
void SetResLoad(Boolean load)
```

Trap: `0xA99B` executor=C_

### SetResPurge  

```c
void SetResPurge(Boolean install)
```

Trap: `0xA993` executor=C_

### SetResourceSize  

```c
void SetResourceSize(Handle resource, int32_t size)
```

Trap: — (executor 实现，无 trap) executor=C_

### Unique1ID  

```c
INTEGER Unique1ID(ResType typ)
```

Trap: `0xA810` executor=C_

### UniqueID  

```c
INTEGER UniqueID(ResType typ)
```

Trap: `0xA9C1` executor=C_

### UpdateResFile  

```c
void UpdateResFile(INTEGER rn)
```

Trap: `0xA999` executor=C_

### UseResFile  

```c
void UseResFile(INTEGER rn)
```

Trap: `0xA998` executor=C_

### WritePartialResource  

```c
void WritePartialResource(Handle resource, int32_t offset, Ptr buffer, int32_t count)
```

Trap: — (executor 实现，无 trap) executor=C_

### WriteResource  

```c
void WriteResource(Handle res)
```

Trap: `0xA9B0` executor=C_

## Enums

- **?** — resource attribute masks
- **?**
- **?** — resource manager return codes
- **?**
- **?** — IMIV
- **?** — IMVI
- **?** — resource file attribute masks

### Enum Values

**anonymous** — resource attribute masks:

- `resSysHeap` = 64
- `resPurgeable` = 32
- `resLocked` = 16
- `resProtected` = 8
- `resPreload` = 4
- `resChanged` = 2

**anonymous**:

- `resCompressed` = 1

**anonymous** — resource manager return codes:

- `CantDecompress` = -186

**anonymous**:

- `resNotFound` = -192
- `resFNotFound` = -193
- `addResFailed` = -194
- `rmvResFailed` = -196

**anonymous** — IMIV:

- `resAttrErr` = -198
- `mapReadErr` = -199

**anonymous** — IMVI:

- `resourceInMemory` = -188
- `inputOutOfBounds` = -190

**anonymous** — resource file attribute masks:

- `mapReadOnly` = 128
- `mapCompact` = 64
- `mapChanged` = 32

## Dispatchers

- **ResourceDispatch**—

## Low Memory Globals

- **TopMapHndl** @ 0xA50 (Handle) — ResourceMgr IMI-115 (true);
- **SysMapHndl** @ 0xA54 (Handle) — ResourceMgr IMI-114 (true);
- **SysMap** @ 0xA58 (INTEGER) — ResourceMgr IMI-114 (true);
- **CurMap** @ 0xA5A (INTEGER) — ResourceMgr IMI-117 (true);
- **resreadonly** @ 0xA5C (INTEGER) — ResourceMgr ToolEqu.a (false);
- **ResLoad** @ 0xA5E (Boolean) — ResourceMgr IMI-118 (true);
- **ResErr** @ 0xA60 (INTEGER) — ResourceMgr IMI-118 (true);
- **ResErrProc** @ 0xAF2 (ProcPtr) — ResourceMgr IMI-116 (true);
- **SysResName** @ 0xAD8 (Byte[20]) — ResourceMgr IMI-114 (true);
- **RomMapInsert** @ 0xB9E (Byte) — ResourceMgr IMIV-19 (false);
- **TmpResLoad** @ 0xB9F (Byte) — ResourceMgr IMIV-19 (false);
