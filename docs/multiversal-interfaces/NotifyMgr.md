# NotifyMgr Interfaces

value of -1 means remove queue element automatically

Source: `multiversal/defs/NotifyMgr.yaml`

- Functions: **2**
- Typedefs: **1**
- Structs: **1**, Unions: **0**
- Enums: **0**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **0**
- Low-memory globals: **0**

## Functions

### NMInstall  

```c
OSErr NMInstall(NMRecPtr nmptr)
```

Trap: `0xA05E` executor=True

### NMRemove  

```c
OSErr NMRemove(NMRecPtr nmptr)
```

Trap: `0xA05F` executor=True

## Typedefs

- **NMRecPtr** = NMRec* — value of -1 means remove queue element automatically

## Structs

- **NMRec** { qLink: QElemPtr, qType: INTEGER, nmFlags: INTEGER, nmPrivate: LONGINT, nmReserved: INTEGER, nmMark: INTEGER, nmIcon: Handle, nmSound: Handle, nmStr: StringPtr, nmResp: ProcPtr, nmRefCon: LONGINT }

