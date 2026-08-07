# VRetraceMgr Interfaces

VRetraceMgr IMII-352 (true);

Source: `multiversal/defs/VRetraceMgr.yaml`

- Functions: **5**
- Typedefs: **1**
- Structs: **1**, Unions: **0**
- Enums: **1**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **0**
- Low-memory globals: **2**

## Functions

### GetVBLQHdr  

```c
QHdrPtr GetVBLQHdr()
```

Trap: — (executor 实现，无 trap)

### SlotVInstall  

```c
OSErr SlotVInstall(VBLTaskPtr vtaskp, INTEGER slot)
```

Trap: `0xA06F` executor=True

### SlotVRemove  

```c
OSErr SlotVRemove(VBLTaskPtr vtaskp, INTEGER slot)
```

Trap: `0xA070` executor=True

### VInstall  

```c
OSErr VInstall(VBLTaskPtr vtaskp)
```

Trap: `0xA033` executor=True

### VRemove  

```c
OSErr VRemove(VBLTaskPtr vtaskp)
```

Trap: `0xA034` executor=True

## Typedefs

- **VBLTaskPtr** = VBLTask*

## Enums

- **?**

### Enum Values

**anonymous**:

- `qErr` = -1
- `vTypErr` = -2

## Structs

- **VBLTask** { qLink: QElemPtr, qType: INTEGER, vblAddr: ProcPtr, vblCount: INTEGER, vblPhase: INTEGER }

## Low Memory Globals

- **VBLQueue** @ 0x160 (QHdr) — VRetraceMgr IMII-352 (true);
- **JVBLTask** @ 0xD28 (ProcPtr) — VRetraceMgr IMV (false);
