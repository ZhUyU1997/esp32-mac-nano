# HelpMgr Interfaces

Source: `multiversal/defs/HelpMgr.yaml`

- Functions: **21**
- Typedefs: **1**
- Structs: **2**, Unions: **0**
- Enums: **1**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **1**
- Low-memory globals: **0**

## Functions

### HMBalloonPict  

```c
OSErr HMBalloonPict(HMMessageRecord* messp, PicHandle* pictp)
```

Trap: — (executor 实现，无 trap) executor=C_

### HMBalloonRect  

```c
OSErr HMBalloonRect(HMMessageRecord* messp, Rect* rectp)
```

Trap: — (executor 实现，无 trap) executor=C_

### HMExtractHelpMsg  

```c
OSErr HMExtractHelpMsg(ResType type, INTEGER resid, INTEGER msg, INTEGER state, HMMessageRecord* helpmsgp)
```

Trap: — (executor 实现，无 trap) executor=C_

### HMGetBalloonWindow  

```c
OSErr HMGetBalloonWindow(WindowPtr* windowpp)
```

Trap: — (executor 实现，无 trap) executor=C_

### HMGetBalloons  

```c
Boolean HMGetBalloons()
```

Trap: — (executor 实现，无 trap) executor=C_

### HMGetDialogResID  

```c
OSErr HMGetDialogResID(INTEGER* residp)
```

Trap: — (executor 实现，无 trap) executor=C_

### HMGetFont  

```c
OSErr HMGetFont(INTEGER* fontp)
```

Trap: — (executor 实现，无 trap) executor=C_

### HMGetFontSize  

```c
OSErr HMGetFontSize(INTEGER* sizep)
```

Trap: — (executor 实现，无 trap) executor=C_

### HMGetHelpMenuHandle  

```c
OSErr HMGetHelpMenuHandle(MenuHandle* mhp)
```

Trap: — (executor 实现，无 trap) executor=C_

### HMGetIndHelpMsg  

```c
OSErr HMGetIndHelpMsg(ResType type, INTEGER resid, INTEGER msg, INTEGER state, LONGINT* options, Point tip, Rect* altrectp, INTEGER* theprocp, INTEGER* variantp, HMMessageRecord* helpmsgp, INTEGER* count)
```

Trap: — (executor 实现，无 trap) executor=C_

### HMGetMenuResID  

```c
OSErr HMGetMenuResID(INTEGER* menuidp, INTEGER* residp)
```

Trap: — (executor 实现，无 trap) executor=C_

### HMIsBalloon  

```c
Boolean HMIsBalloon()
```

Trap: — (executor 实现，无 trap) executor=C_

### HMRemoveBalloon  

```c
OSErr HMRemoveBalloon()
```

Trap: — (executor 实现，无 trap) executor=C_

### HMScanTemplateItems  

```c
OSErr HMScanTemplateItems(INTEGER whichid, INTEGER whicresfile, ResType whictype)
```

Trap: — (executor 实现，无 trap) executor=C_

### HMSetBalloons  

```c
OSErr HMSetBalloons(Boolean flag)
```

Trap: — (executor 实现，无 trap) executor=C_

### HMSetDialogResID  

```c
OSErr HMSetDialogResID(INTEGER resid)
```

Trap: — (executor 实现，无 trap) executor=C_

### HMSetFont  

```c
OSErr HMSetFont(INTEGER font)
```

Trap: — (executor 实现，无 trap) executor=C_

### HMSetFontSize  

```c
OSErr HMSetFontSize(INTEGER size)
```

Trap: — (executor 实现，无 trap) executor=C_

### HMSetMenuResID  

```c
OSErr HMSetMenuResID(INTEGER menuid, INTEGER resid)
```

Trap: — (executor 实现，无 trap) executor=C_

### HMShowBalloon  

```c
OSErr HMShowBalloon(HMMessageRecord* msgp, Point tip, RectPtr alternaterectp, Ptr tipprocptr, INTEGER proc, INTEGER variant, INTEGER method)
```

Trap: — (executor 实现，无 trap) executor=C_

### HMShowMenuBalloon  

```c
OSErr HMShowMenuBalloon(INTEGER item, INTEGER menuid, LONGINT flags, LONGINT itemreserved, Point tip, RectPtr alternaterectp, Ptr tipproc, INTEGER proc, INTEGER variant)
```

Trap: — (executor 实现，无 trap) executor=C_

## Typedefs

- **HMMessageRecPtr** = HMMessageRecord*

## Enums

- **?**

### Enum Values

**anonymous**:

- `hmHelpDisabled` = -850
- `hmBalloonAborted` = -853
- `hmSameAsLastBalloon` = -854
- `hmHelpManagerNotInited` = -855
- `hmSkippedBalloon` = -857
- `hmWrongVersion` = -858
- `hmUnknownHelpType` = -859
- `hmOperationUnsupported` = -861
- `hmNoBalloonUp` = -862
- `hmCloseViewActive` = -863

## Structs

- **HMStringResType** { hmmResID: INTEGER, hmmIndex: INTEGER }
- **HMMessageRecord** { hmmHelpType: INTEGER, u: ? }

## Dispatchers

- **Pack14**— & 0xFF ? ### — & 0xFF ? ###

