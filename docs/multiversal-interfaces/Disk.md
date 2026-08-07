# Disk Interfaces

Source: `multiversal/defs/Disk.yaml`

- Functions: **3**
- Typedefs: **0**
- Structs: **1**, Unions: **0**
- Enums: **1**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **0**
- Low-memory globals: **0**

## Functions

### DiskEject  

```c
OSErr DiskEject(INTEGER rn)
```

Trap: — (executor 实现，无 trap)

### DriveStatus  

```c
OSErr DriveStatus(INTEGER dn, DrvSts* statp)
```

Trap: — (executor 实现，无 trap)

### SetTagBuffer  

```c
OSErr SetTagBuffer(Ptr bp)
```

Trap: — (executor 实现，无 trap)

## Enums

- **?**

### Enum Values

**anonymous**:

- `firstDskErr` = -84
- `sectNFErr` = -81
- `seekErr` = -80
- `spdAdjErr` = -79
- `twoSideErr` = -78
- `initIWMErr` = -77
- `tk0BadErr` = -76
- `cantStepErr` = -75
- `wrUnderrun` = -74
- `badDBtSlp` = -73
- `badDCksum` = -72
- `noDtaMkErr` = -71
- `badBtSlpErr` = -70
- `badCksmErr` = -69
- `dataVerErr` = -68
- `noAdrMkErr` = -67
- `noNybErr` = -66
- `offLinErr` = -65
- `noDriveErr` = -64
- `lastDskErr` = -64

## Structs

- **DrvSts** { track: INTEGER, writeProt: SignedByte, diskInPlace: SignedByte, installed: SignedByte, sides: SignedByte, qLink: QElemPtr, qType: INTEGER, dQDrive: INTEGER, dQRefNum: INTEGER, dQFSID: INTEGER, twoSideFmt: SignedByte, needsFlush: SignedByte, diskErrs: INTEGER }

