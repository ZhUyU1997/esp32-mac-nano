# CommTool Interfaces

Source: `multiversal/defs/CommTool.yaml`

- Functions: **6**
- Typedefs: **3**
- Structs: **2**, Unions: **0**
- Enums: **6**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **0**
- Low-memory globals: **0**

## Functions

### CRMGetCRMVersion  

```c
INTEGER CRMGetCRMVersion()
```

Trap: — (executor 实现，无 trap)

### CRMGetHeader  

```c
QHdrPtr CRMGetHeader()
```

Trap: — (executor 实现，无 trap)

### CRMInstall  

```c
void CRMInstall(QElemPtr ?)
```

Trap: — (executor 实现，无 trap)

### CRMRemove  

```c
OSErr CRMRemove(QElemPtr ?)
```

Trap: — (executor 实现，无 trap)

### CRMSearch  

```c
QElemPtr CRMSearch(QElemPtr ?)
```

Trap: — (executor 实现，无 trap)

### InitCRM  

```c
CRMErr InitCRM()
```

Trap: — (executor 实现，无 trap)

## Typedefs

- **CRMRecPtr** = CRMRec*
- **CRMErr** = OSErr
- **CRMSerialPtr** = CRMSerialRecord*

## Enums

- **?**
- **?**
- **?**
- **?**
- **?**
- **?**

### Enum Values

**anonymous**:

- `crmGenericError` = -1
- `crmNoErr` = 0

**anonymous**:

- `curCRMVersion` = 1

**anonymous**:

- `crmType` = 9

**anonymous**:

- `crmRecVersion` = 1

**anonymous**:

- `curCRMSerRecVer` = 0

**anonymous**:

- `crmSerialDevice` = 1

## Structs

- **CRMRec** { qLink: QElemPtr, qType: INTEGER, crmVersion: INTEGER, crmPrivate: LONGINT, crmReserved: INTEGER, crmDeviceType: LONGINT, crmDeviceID: LONGINT, crmAttributes: LONGINT, crmStatus: LONGINT, crmRefCon: LONGINT }
- **CRMSerialRecord** { version: INTEGER, inputDriverName: StringHandle, outputDriverName: StringHandle, name: StringHandle, deviceIcon: Handle, ratedSpeed: LONGINT, maxSpeed: LONGINT, reserved: LONGINT }

