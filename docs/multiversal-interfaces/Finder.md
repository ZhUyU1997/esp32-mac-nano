# Finder Interfaces

Source: `multiversal/defs/Finder.yaml`

- Functions: **16**
- Typedefs: **2**
- Structs: **1**, Unions: **0**
- Enums: **0**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **0**
- Low-memory globals: **0**

## Functions

### PBDTAddAPPL  

```c
OSErr PBDTAddAPPL(DTPBPtr dtp, Boolean async)
```

Trap: `0xA260` executor=True

### PBDTAddIcon  

```c
OSErr PBDTAddIcon(DTPBPtr dtp, Boolean async)
```

Trap: `0xA260` executor=True

### PBDTCloseDown  

```c
OSErr PBDTCloseDown(DTPBPtr dtp)
```

Trap: — (executor 实现，无 trap) executor=True

### PBDTDelete  

```c
OSErr PBDTDelete(DTPBPtr dtp, Boolean async)
```

Trap: `0xA260` executor=True

### PBDTFlush  

```c
OSErr PBDTFlush(DTPBPtr dtp, Boolean async)
```

Trap: `0xA260` executor=True

### PBDTGetAPPL  

```c
OSErr PBDTGetAPPL(DTPBPtr dtp, Boolean async)
```

Trap: `0xA260` executor=True

### PBDTGetComment  

```c
OSErr PBDTGetComment(DTPBPtr dtp, Boolean async)
```

Trap: `0xA260` executor=True

### PBDTGetIcon  

```c
OSErr PBDTGetIcon(DTPBPtr dtp, Boolean async)
```

Trap: `0xA260` executor=True

### PBDTGetIconInfo  

```c
OSErr PBDTGetIconInfo(DTPBPtr dtp, Boolean async)
```

Trap: `0xA260` executor=True

### PBDTGetInfo  

```c
OSErr PBDTGetInfo(DTPBPtr dtp, Boolean async)
```

Trap: `0xA260` executor=True

### PBDTGetPath  

```c
OSErr PBDTGetPath(DTPBPtr dtp)
```

Trap: — (executor 实现，无 trap) executor=True

### PBDTOpenInform  

```c
OSErr PBDTOpenInform(DTPBPtr dtp)
```

Trap: — (executor 实现，无 trap) executor=True

### PBDTRemoveAPPL  

```c
OSErr PBDTRemoveAPPL(DTPBPtr dtp, Boolean async)
```

Trap: `0xA260` executor=True

### PBDTRemoveComment  

```c
OSErr PBDTRemoveComment(DTPBPtr dtp, Boolean async)
```

Trap: `0xA260` executor=True

### PBDTReset  

```c
OSErr PBDTReset(DTPBPtr dtp, Boolean async)
```

Trap: `0xA260` executor=True

### PBDTSetComment  

```c
OSErr PBDTSetComment(DTPBPtr dtp, Boolean async)
```

Trap: `0xA260` executor=True

## Typedefs

- **DTPBRecPtr** = DTPBRec*
- **DTPBPtr** = DTPBRec*

## Structs

- **DTPBRec** { qLink: QElemPtr, qType: INTEGER, ioTrap: INTEGER, ioCmdAddr: Ptr, ioCompletion: ProcPtr, ioResult: OSErr, ioNamePtr: StringPtr, ioVRefNum: INTEGER, ioDTRefNum: INTEGER, ioIndex: INTEGER, ioTagInfo: LONGINT, ioDTBuffer: Ptr, ioDTReqCount: LONGINT, ioDTActCount: LONGINT, filler1: SignedByte, ioIconType: SignedByte, filler2: INTEGER, ioDirID: LONGINT, ioFileCreator: OSType, ioFileType: OSType, ioFiller3: LONGINT, ioDTLgLen: LONGINT, ioDTPyLen: LONGINT, ioFiller4: INTEGER[14], ioAPPLParID: LONGINT }

