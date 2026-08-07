# PrintMgr Interfaces

From Technote 095 more stuff may be here

Source: `multiversal/defs/PrintMgr.yaml`

- Functions: **23**
- Typedefs: **5**
- Structs: **8**, Unions: **0**
- Enums: **8**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **1**
- Low-memory globals: **1**

## Functions

### PrClose  

```c
void PrClose()
```

Trap: — (executor 实现，无 trap) executor=C_

### PrCloseDoc  

```c
void PrCloseDoc(TPPrPort port)
```

Trap: — (executor 实现，无 trap) executor=C_

### PrClosePage  

```c
void PrClosePage(TPPrPort pPrPort)
```

Trap: — (executor 实现，无 trap) executor=C_

### PrCtlCall  

```c
void PrCtlCall(INTEGER iWhichCtl, LONGINT lParam1, LONGINT lParam2, LONGINT lParam3)
```

Trap: — (executor 实现，无 trap) executor=C_

### PrDlgMain  

```c
Boolean PrDlgMain(THPrint hPrint, ProcPtr initfptr)
```

Trap: — (executor 实现，无 trap) executor=C_

### PrDrvrClose  

```c
void PrDrvrClose()
```

Trap: — (executor 实现，无 trap) executor=C_

### PrDrvrDCE  

```c
Handle PrDrvrDCE()
```

Trap: — (executor 实现，无 trap) executor=C_

### PrDrvrOpen  

```c
void PrDrvrOpen()
```

Trap: — (executor 实现，无 trap) executor=C_

### PrDrvrVers  

```c
INTEGER PrDrvrVers()
```

Trap: — (executor 实现，无 trap) executor=C_

### PrError  

```c
INTEGER PrError()
```

Trap: — (executor 实现，无 trap) executor=C_

### PrGeneral  

```c
void PrGeneral(Ptr pData)
```

Trap: — (executor 实现，无 trap) executor=C_

### PrJobDialog  

```c
Boolean PrJobDialog(THPrint hPrint)
```

Trap: — (executor 实现，无 trap) executor=C_

### PrJobInit  

```c
TPPrDlg PrJobInit(THPrint hPrint)
```

Trap: — (executor 实现，无 trap) executor=C_

### PrJobMerge  

```c
void PrJobMerge(THPrint hPrintSrc, THPrint hPrintDst)
```

Trap: — (executor 实现，无 trap) executor=C_

### PrOpen  

```c
void PrOpen()
```

Trap: — (executor 实现，无 trap) executor=C_

### PrOpenDoc  

```c
TPPrPort PrOpenDoc(THPrint hPrint, TPPrPort port, Ptr pIOBuf)
```

Trap: — (executor 实现，无 trap) executor=C_

### PrOpenPage  

```c
void PrOpenPage(TPPrPort port, TPRect pPageFrame)
```

Trap: — (executor 实现，无 trap) executor=C_

### PrPicFile  

```c
void PrPicFile(THPrint hPrint, TPPrPort pPrPort, Ptr pIOBuf, Ptr pDevBuf, TPrStatus* prStatus)
```

Trap: — (executor 实现，无 trap) executor=C_

### PrSetError  

```c
void PrSetError(INTEGER iErr)
```

Trap: — (executor 实现，无 trap) executor=C_

### PrStlDialog  

```c
Boolean PrStlDialog(THPrint hPrint)
```

Trap: — (executor 实现，无 trap) executor=C_

### PrStlInit  

```c
TPPrDlg PrStlInit(THPrint hPrint)
```

Trap: — (executor 实现，无 trap) executor=C_

### PrValidate  

```c
Boolean PrValidate(THPrint hPrint)
```

Trap: — (executor 实现，无 trap) executor=C_

### PrintDefault  

```c
void PrintDefault(THPrint hPrint)
```

Trap: — (executor 实现，无 trap) executor=C_

## Typedefs

- **TPPrPort** = TPrPort*
- **TPPrint** = TPrint*
- **THPrint** = TPPrint*
- **TPRect** = Rect*
- **TPPrDlg** = TPrDlg* — From Technote 095 more stuff may be here

## Enums

- **?**
- **?**
- **?**
- **?**
- **?**
- **?**
- **TFeed**
- **TScan**

### Enum Values

**anonymous**:

- `bDraftLoop` = 0
- `bSpoolLoop` = 1

**anonymous**:

- `bDevCItoh` = 1
- `bDevLaser` = 3

**anonymous**:

- `iPFMaxPgs` = 128

**anonymous**:

- `iPrSavPFil` = -1
- `iIOAbort` = -27
- `MemFullErr` = -108
- `iPrAbort` = 128

**anonymous**:

- `iPrDevCtl` = 7
- `lPrReset` = 65536
- `lPrLineFeed` = 196608
- `lPrLFSixth` = 262143
- `lPrPageEnd` = 131072
- `iPrBitsCtl` = 4
- `lScreenBits` = 0
- `lPaintBits` = 1
- `iPrIOCtl` = 5

**anonymous**:

- `iPrDrvrRef` = -3

**TFeed**:

- `feedCut` = ?
- `feedFanFold` = ?
- `feedMechCut` = ?
- `feedOther` = ?

**TScan**:

- `scanTB` = ?
- `scanBL` = ?
- `scanLR` = ?
- `scanRL` = ?

## Structs

- **TPrPort** { gPort: GrafPort, saveprocs: QDProcs, spare: LONGINT[4], fOurPtr: Boolean, fOurBits: Boolean }
- **TPrInfo** { iDev: INTEGER, iVRes: INTEGER, iHRes: INTEGER, rPage: Rect }
- **TPrStl** { wDev: INTEGER, iPageV: INTEGER, iPageH: INTEGER, bPort: SignedByte, feed: char }
- **TPrXInfo** { iRowBytes: INTEGER, iBandV: INTEGER, iBandH: INTEGER, iDevBytes: INTEGER, iBands: INTEGER, bPatScale: SignedByte, bULThick: SignedByte, bULOffset: SignedByte, bULShadow: SignedByte, scan: char, bXInfoX: SignedByte }
- **TPrJob** { iFstPage: INTEGER, iLstPage: INTEGER, iCopies: INTEGER, bJDocLoop: SignedByte, fFromUsr: Boolean, pIdleProc: ProcPtr, pFileName: StringPtr, iFileVol: INTEGER, bFileVers: SignedByte, bJobX: SignedByte }
- **TPrint** { iPrVersion: INTEGER, prInfo: TPrInfo, rPaper: Rect, prStl: TPrStl, prInfoPT: TPrInfo, prXInfo: TPrXInfo, prJob: TPrJob, printX: INTEGER[19] }
- **TPrStatus** { iTotPages: INTEGER, iCurPage: INTEGER, iTotCopies: INTEGER, iCurCopy: INTEGER, iTotBands: INTEGER, iCurBand: INTEGER, fPgDirty: Boolean, fImaging: Boolean, hPrint: THPrint, pPRPort: TPPrPort, hPic: PicHandle }
- **TPrDlg** { dlg: DialogRecord, pFltrProc: ModalFilterUPP, pItemProc: UserItemUPP, hPrintUsr: THPrint, fDoIt: Boolean, fDone: Boolean, lUser1: LONGINT, lUser2: LONGINT, lUser3: LONGINT, lUser4: LONGINT, iNumFst: INTEGER, iNumLst: INTEGER }

## Dispatchers

- **PrGlue**—

## Low Memory Globals

- **PrintErr** @ 0x944 (INTEGER) — PrintMgr IMII-161 (true-b);
