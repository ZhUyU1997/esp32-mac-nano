# FileMgr Interfaces

TODO: Trap?

Source: `multiversal/defs/FileMgr.yaml`

- Functions: **122**
- Typedefs: **11**
- Structs: **20**, Unions: **3**
- Enums: **7**
- Function pointers: **0**
- Common blocks: **2**
- Dispatchers: **2**
- Low-memory globals: **11**

## Functions

### AllocContig  

```c
OSErr AllocContig(INTEGER rn, LONGINT* count)
```

Trap: — (executor 实现，无 trap) executor=True

### Allocate  

```c
OSErr Allocate(INTEGER rn, LONGINT* count)
```

Trap: — (executor 实现，无 trap) executor=True

### Create  

```c
OSErr Create(ConstStringPtr filen, INTEGER vrn, OSType creator, OSType filtyp)
```

Trap: — (executor 实现，无 trap) executor=True

### Eject  

```c
OSErr Eject(ConstStringPtr voln, INTEGER vrn)
```

Trap: — (executor 实现，无 trap) executor=True

### FInitQueue  

```c
void FInitQueue()
```

Trap: `0xA016` executor=C_

### FSClose  

```c
OSErr FSClose(INTEGER rn)
```

Trap: — (executor 实现，无 trap) executor=True

### FSDelete  

```c
OSErr FSDelete(ConstStringPtr filen, INTEGER vrn)
```

Trap: — (executor 实现，无 trap) executor=True

### FSMakeFSSpec  

```c
OSErr FSMakeFSSpec(int16_t vRefNum, int32_t dir_id, ConstStringPtr file_name, FSSpecPtr spec)
```

Trap: — (executor 实现，无 trap) executor=C_

### FSOpen  

```c
OSErr FSOpen(ConstStringPtr filen, INTEGER vrn, INTEGER* rn)
```

Trap: — (executor 实现，无 trap) executor=True

### FSRead  

```c
OSErr FSRead(INTEGER rn, LONGINT* count, void* buffp)
```

Trap: — (executor 实现，无 trap) executor=True

### FSWrite  

```c
OSErr FSWrite(INTEGER rn, LONGINT* count, const void* buffp)
```

Trap: — (executor 实现，无 trap) executor=True

### FSpCatMove  

```c
OSErr FSpCatMove(FSSpecPtr src, FSSpecPtr dst)
```

Trap: — (executor 实现，无 trap) executor=C_

### FSpCreate  

```c
OSErr FSpCreate(FSSpecPtr spec, OSType creator, OSType file_type, ScriptCode script)
```

Trap: — (executor 实现，无 trap) executor=C_

### FSpCreateResFile  

```c
void FSpCreateResFile(FSSpecPtr spec, OSType creator, OSType file_type, ScriptCode script)
```

Trap: — (executor 实现，无 trap) executor=C_

### FSpDelete  

```c
OSErr FSpDelete(FSSpecPtr spec)
```

Trap: — (executor 实现，无 trap) executor=C_

### FSpDirCreate  

```c
OSErr FSpDirCreate(FSSpecPtr spec, ScriptCode script, int32_t* created_dir_id)
```

Trap: — (executor 实现，无 trap) executor=C_

### FSpExchangeFiles  

```c
OSErr FSpExchangeFiles(FSSpecPtr src, FSSpecPtr dst)
```

Trap: — (executor 实现，无 trap) executor=C_

### FSpGetFInfo  

```c
OSErr FSpGetFInfo(FSSpecPtr spec, FInfo* fndr_info)
```

Trap: — (executor 实现，无 trap) executor=C_

### FSpOpenDF  

```c
OSErr FSpOpenDF(FSSpecPtr spec, SignedByte perms, int16_t* refNum_out)
```

Trap: — (executor 实现，无 trap) executor=C_

### FSpOpenRF  

```c
OSErr FSpOpenRF(FSSpecPtr spec, SignedByte perms, int16_t* refNum_out)
```

Trap: — (executor 实现，无 trap) executor=C_

### FSpOpenResFile  

```c
INTEGER FSpOpenResFile(FSSpecPtr spec, SignedByte perms)
```

Trap: — (executor 实现，无 trap) executor=C_

### FSpRename  

```c
OSErr FSpRename(FSSpecPtr spec, ConstStringPtr new_name)
```

Trap: — (executor 实现，无 trap) executor=C_

### FSpRstFLock  

```c
OSErr FSpRstFLock(FSSpecPtr spec)
```

Trap: — (executor 实现，无 trap) executor=C_

### FSpSetFInfo  

```c
OSErr FSpSetFInfo(FSSpecPtr spec, FInfo* fndr_info)
```

Trap: — (executor 实现，无 trap) executor=C_

### FSpSetFLock  

```c
OSErr FSpSetFLock(FSSpecPtr spec)
```

Trap: — (executor 实现，无 trap) executor=C_

### FlushVol  

```c
OSErr FlushVol(ConstStringPtr voln, INTEGER vrn)
```

Trap: — (executor 实现，无 trap) executor=True

### GetDrvQHdr  

```c
QHdrPtr GetDrvQHdr()
```

Trap: — (executor 实现，无 trap) executor=C_

### GetEOF  

```c
OSErr GetEOF(INTEGER rn, LONGINT* eof)
```

Trap: — (executor 实现，无 trap) executor=True

### GetFInfo  

```c
OSErr GetFInfo(ConstStringPtr filen, INTEGER vrn, FInfo* fndrinfo)
```

Trap: — (executor 实现，无 trap) executor=True

### GetFPos  

```c
OSErr GetFPos(INTEGER rn, LONGINT* filep)
```

Trap: — (executor 实现，无 trap) executor=True

### GetFSQHdr  

```c
QHdrPtr GetFSQHdr()
```

Trap: — (executor 实现，无 trap) executor=C_

### GetVCBQHdr  

```c
QHdrPtr GetVCBQHdr()
```

Trap: — (executor 实现，无 trap) executor=C_

### GetVInfo  

```c
OSErr GetVInfo(INTEGER drv, StringPtr voln, INTEGER* vrn, LONGINT* freeb)
```

Trap: — (executor 实现，无 trap)

### GetVRefNum  

```c
OSErr GetVRefNum(INTEGER prn, INTEGER* vrn)
```

Trap: — (executor 实现，无 trap)

### GetVol  

```c
OSErr GetVol(StringPtr voln, INTEGER* vrn)
```

Trap: — (executor 实现，无 trap) executor=True

### GetWDInfo  

```c
OSErr GetWDInfo(INTEGER wd, INTEGER* vrefp, LONGINT* dirp, LONGINT* procp)
```

Trap: — (executor 实现，无 trap) — NOTRAP_FUNCTION2(HDelete);

### HCreate  

```c
OSErr HCreate(INTEGER vref, LONGINT dirid, ConstStringPtr name, OSType creator, OSType type)
```

Trap: — (executor 实现，无 trap) executor=True

### HCreateResFile  

```c
void HCreateResFile(INTEGER vrefnum, LONGINT parid, ConstStringPtr name)
```

Trap: `0xA81B` executor=C_

### HDelete  

```c
OSErr HDelete(INTEGER vref, LONGINT dirid, ConstStringPtr name)
```

Trap: — (executor 实现，无 trap) — NOTRAP_FUNCTION2(HRename);

### HGetFInfo  

```c
OSErr HGetFInfo(INTEGER vref, LONGINT dirid, ConstStringPtr name, FInfo* fndrinfo)
```

Trap: — (executor 实现，无 trap) executor=True

### HOpen  

```c
OSErr HOpen(INTEGER vref, LONGINT dirid, ConstStringPtr name, SignedByte perm, INTEGER* refp)
```

Trap: — (executor 实现，无 trap) executor=True

### HOpenDF  

```c
OSErr HOpenDF(INTEGER vref, LONGINT dirid, ConstStringPtr name, SignedByte perm, INTEGER* refp)
```

Trap: — (executor 实现，无 trap) executor=True

### HOpenRF  

```c
OSErr HOpenRF(INTEGER vref, LONGINT dirid, ConstStringPtr name, SignedByte perm, INTEGER* refp)
```

Trap: — (executor 实现，无 trap) executor=True

### HOpenResFile  

```c
INTEGER HOpenResFile(INTEGER vref, LONGINT dirid, ConstStringPtr file_name, SignedByte perm)
```

Trap: `0xA81A` executor=C_

### HRename  

```c
OSErr HRename(INTEGER vref, LONGINT dirid, ConstStringPtr src, ConstStringPtr dst)
```

Trap: — (executor 实现，无 trap)

### OpenDF  

```c
OSErr OpenDF(ConstStringPtr filen, INTEGER vrn, INTEGER* rn)
```

Trap: — (executor 实现，无 trap) executor=True

### OpenRF  

```c
OSErr OpenRF(ConstStringPtr filen, INTEGER vrn, INTEGER* rn)
```

Trap: — (executor 实现，无 trap) executor=True

### PBAllocContig  

```c
OSErr PBAllocContig(ParmBlkPtr pb, Boolean async)
```

Trap: — (executor 实现，无 trap)

### PBAllocate  

```c
OSErr PBAllocate(ParmBlkPtr pb, Boolean async)
```

Trap: `0xA010` executor=True

### PBCatMove  

```c
OSErr PBCatMove(CMovePBPtr pb, Boolean async)
```

Trap: `0xA260` executor=True

### PBCatSearch  

```c
OSErr PBCatSearch(ParmBlkPtr pb, Boolean async)
```

Trap: `0xA260` executor=True

### PBClose  

```c
OSErr PBClose(ParmBlkPtr pb, Boolean async)
```

Trap: `0xA001` executor=True

### PBCloseWD  

```c
OSErr PBCloseWD(WDPBPtr pb, Boolean async)
```

Trap: `0xA260` executor=True

### PBCreate  

```c
OSErr PBCreate(ParmBlkPtr pb, Boolean async)
```

Trap: `0xA008` executor=True

### PBCreateFileIDRef  

```c
OSErr PBCreateFileIDRef(ParmBlkPtr pb, Boolean async)
```

Trap: `0xA260` executor=True

### PBDelete  

```c
OSErr PBDelete(ParmBlkPtr pb, Boolean async)
```

Trap: `0xA009` executor=True

### PBDeleteFileIDRef  

```c
OSErr PBDeleteFileIDRef(ParmBlkPtr pb, Boolean async)
```

Trap: `0xA260` executor=True

### PBDirCreate  

```c
OSErr PBDirCreate(HParmBlkPtr pb, Boolean async)
```

Trap: `0xA260` executor=True

### PBEject  

```c
OSErr PBEject(ParmBlkPtr pb)
```

Trap: `0xA017` executor=True

### PBExchangeFiles  

```c
OSErr PBExchangeFiles(ParmBlkPtr pb, Boolean async)
```

Trap: `0xA260` executor=True

### PBFlushFile  

```c
OSErr PBFlushFile(ParmBlkPtr pb, Boolean async)
```

Trap: `0xA045` executor=True

### PBFlushVol  

```c
OSErr PBFlushVol(ParmBlkPtr pb, Boolean async)
```

Trap: `0xA013` executor=True

### PBGetCatInfo  

```c
OSErr PBGetCatInfo(CInfoPBPtr pb, Boolean async)
```

Trap: `0xA260` executor=True

### PBGetEOF  

```c
OSErr PBGetEOF(ParmBlkPtr pb, Boolean async)
```

Trap: `0xA011` executor=True

### PBGetFCBInfo  

```c
OSErr PBGetFCBInfo(FCBPBPtr pb, Boolean async)
```

Trap: `0xA260` executor=True

### PBGetFInfo  

```c
OSErr PBGetFInfo(ParmBlkPtr pb, Boolean async)
```

Trap: `0xA00C` executor=True

### PBGetFPos  

```c
OSErr PBGetFPos(ParmBlkPtr pb, Boolean async)
```

Trap: `0xA018` executor=True

### PBGetVInfo  

```c
OSErr PBGetVInfo(ParmBlkPtr pb, Boolean async)
```

Trap: `0xA007` executor=True

### PBGetVol  

```c
OSErr PBGetVol(ParmBlkPtr pb, Boolean async)
```

Trap: `0xA014` executor=True

### PBGetWDInfo  

```c
OSErr PBGetWDInfo(WDPBPtr pb, Boolean async)
```

Trap: `0xA260` executor=True

### PBHCopyFile  

```c
OSErr PBHCopyFile(HParmBlkPtr pb, Boolean a)
```

Trap: `0xA260` executor=True

### PBHCreate  

```c
OSErr PBHCreate(HParmBlkPtr pb, Boolean async)
```

Trap: `0xA208` executor=True

### PBHDelete  

```c
OSErr PBHDelete(HParmBlkPtr pb, Boolean async)
```

Trap: `0xA209` executor=True

### PBHGetDirAccess  

```c
OSErr PBHGetDirAccess(HParmBlkPtr pb, Boolean a)
```

Trap: `0xA260` executor=True

### PBHGetFInfo  

```c
OSErr PBHGetFInfo(HParmBlkPtr pb, Boolean async)
```

Trap: `0xA20C` executor=True

### PBHGetLogInInfo  

```c
OSErr PBHGetLogInInfo(HParmBlkPtr pb, Boolean a)
```

Trap: `0xA260` executor=True

### PBHGetVInfo  

```c
OSErr PBHGetVInfo(HParmBlkPtr pb, Boolean async)
```

Trap: `0xA207` executor=True

### PBHGetVol  

```c
OSErr PBHGetVol(WDPBPtr pb, Boolean async)
```

Trap: `0xA214` executor=True

### PBHGetVolParms  

```c
OSErr PBHGetVolParms(HParmBlkPtr pb, Boolean async)
```

Trap: `0xA260` executor=True

### PBHMapID  

```c
OSErr PBHMapID(HParmBlkPtr pb, Boolean a)
```

Trap: `0xA260` executor=True

### PBHMapName  

```c
OSErr PBHMapName(HParmBlkPtr pb, Boolean a)
```

Trap: `0xA260` executor=True

### PBHMoveRename  

```c
OSErr PBHMoveRename(HParmBlkPtr pb, Boolean a)
```

Trap: `0xA260` executor=True

### PBHOpen  

```c
OSErr PBHOpen(HParmBlkPtr pb, Boolean async)
```

Trap: `0xA200` executor=True

### PBHOpenDF  

```c
OSErr PBHOpenDF(HParmBlkPtr pb, Boolean async)
```

Trap: `0xA260` executor=True

### PBHOpenDeny  

```c
OSErr PBHOpenDeny(HParmBlkPtr pb, Boolean a)
```

Trap: `0xA260` executor=True

### PBHOpenRF  

```c
OSErr PBHOpenRF(HParmBlkPtr pb, Boolean async)
```

Trap: `0xA20A` executor=True

### PBHRename  

```c
OSErr PBHRename(HParmBlkPtr pb, Boolean async)
```

Trap: `0xA20B` executor=True

### PBHRstFLock  

```c
OSErr PBHRstFLock(HParmBlkPtr pb, Boolean async)
```

Trap: `0xA242` executor=True

### PBHSetDirAccess  

```c
OSErr PBHSetDirAccess(HParmBlkPtr pb, Boolean a)
```

Trap: `0xA260` executor=True

### PBHSetFInfo  

```c
OSErr PBHSetFInfo(HParmBlkPtr pb, Boolean async)
```

Trap: `0xA20D` executor=True

### PBHSetFLock  

```c
OSErr PBHSetFLock(HParmBlkPtr pb, Boolean async)
```

Trap: `0xA241` executor=True

### PBHSetVol  

```c
OSErr PBHSetVol(WDPBPtr pb, Boolean async)
```

Trap: `0xA215` executor=True

### PBLockRange  

```c
OSErr PBLockRange(ParmBlkPtr pb, Boolean async)
```

Trap: `0xA260` executor=True

### PBMountVol  

```c
OSErr PBMountVol(ParmBlkPtr pb)
```

Trap: `0xA00F` executor=True

### PBOffLine  

```c
OSErr PBOffLine(ParmBlkPtr pb)
```

Trap: `0xA035` executor=True

### PBOpen  

```c
OSErr PBOpen(ParmBlkPtr pb, Boolean async)
```

Trap: `0xA000` executor=True — TODO: Trap?

### PBOpenDF  

```c
OSErr PBOpenDF(ParmBlkPtr pb, Boolean async)
```

Trap: `0xA060` executor=True

### PBOpenRF  

```c
OSErr PBOpenRF(ParmBlkPtr pb, Boolean async)
```

Trap: `0xA00A` executor=True

### PBOpenWD  

```c
OSErr PBOpenWD(WDPBPtr pb, Boolean async)
```

Trap: `0xA260` executor=True

### PBRead  

```c
OSErr PBRead(ParmBlkPtr pb, Boolean async)
```

Trap: `0xA002` executor=True

### PBRename  

```c
OSErr PBRename(ParmBlkPtr pb, Boolean async)
```

Trap: `0xA00B` executor=True

### PBResolveFileIDRef  

```c
OSErr PBResolveFileIDRef(ParmBlkPtr pb, Boolean async)
```

Trap: `0xA260` executor=True

### PBRstFLock  

```c
OSErr PBRstFLock(ParmBlkPtr pb, Boolean async)
```

Trap: `0xA042` executor=True

### PBSetCatInfo  

```c
OSErr PBSetCatInfo(CInfoPBPtr pb, Boolean async)
```

Trap: `0xA260` executor=True

### PBSetEOF  

```c
OSErr PBSetEOF(ParmBlkPtr pb, Boolean async)
```

Trap: `0xA012` executor=True

### PBSetFInfo  

```c
OSErr PBSetFInfo(ParmBlkPtr pb, Boolean async)
```

Trap: `0xA00D` executor=True

### PBSetFLock  

```c
OSErr PBSetFLock(ParmBlkPtr pb, Boolean async)
```

Trap: `0xA041` executor=True

### PBSetFPos  

```c
OSErr PBSetFPos(ParmBlkPtr pb, Boolean async)
```

Trap: `0xA044` executor=True

### PBSetFVers  

```c
OSErr PBSetFVers(ParmBlkPtr pb, Boolean async)
```

Trap: `0xA043` executor=True

### PBSetVInfo  

```c
OSErr PBSetVInfo(HParmBlkPtr pb, Boolean async)
```

Trap: `0xA260` executor=True

### PBSetVol  

```c
OSErr PBSetVol(ParmBlkPtr pb, Boolean async)
```

Trap: `0xA015` executor=True

### PBUnlockRange  

```c
OSErr PBUnlockRange(ParmBlkPtr pb, Boolean async)
```

Trap: `0xA260` executor=True

### PBUnmountVol  

```c
OSErr PBUnmountVol(ParmBlkPtr pb)
```

Trap: `0xA00E` executor=True

### PBWrite  

```c
OSErr PBWrite(ParmBlkPtr pb, Boolean async)
```

Trap: `0xA003` executor=True

### Rename  

```c
OSErr Rename(ConstStringPtr filen, INTEGER vrn, ConstStringPtr newf)
```

Trap: — (executor 实现，无 trap) executor=True

### RstFLock  

```c
OSErr RstFLock(ConstStringPtr filen, INTEGER vrn)
```

Trap: — (executor 实现，无 trap) executor=True

### SetEOF  

```c
OSErr SetEOF(INTEGER rn, LONGINT eof)
```

Trap: — (executor 实现，无 trap) executor=True

### SetFInfo  

```c
OSErr SetFInfo(ConstStringPtr filen, INTEGER vrn, FInfo* fndrinfo)
```

Trap: — (executor 实现，无 trap) executor=True

### SetFLock  

```c
OSErr SetFLock(ConstStringPtr filen, INTEGER vrn)
```

Trap: — (executor 实现，无 trap) executor=True

### SetFPos  

```c
OSErr SetFPos(INTEGER rn, INTEGER posmode, LONGINT possoff)
```

Trap: — (executor 实现，无 trap) executor=True

### SetVol  

```c
OSErr SetVol(ConstStringPtr voln, INTEGER vrn)
```

Trap: — (executor 实现，无 trap) executor=True

### UnmountVol  

```c
OSErr UnmountVol(ConstStringPtr voln, INTEGER vrn)
```

Trap: — (executor 实现，无 trap) executor=True

## Typedefs

- **IOMiscType** = uint32_t — Executor needs ioMisc to be uint32_t instead of the normal Ptr, so this is defined as a separate typedef here.
- **IOMiscType** = Ptr — Executor needs ioMisc to be uint32_t instead of the normal Ptr, so this is defined as a separate typedef here.
- **ParmBlkPtr** = ParamBlockRec*
- **HParmBlkPtr** = HParamBlockRec*
- **CInfoPBPtr** = CInfoPBRec*
- **CMovePBPtr** = CMovePBRec*
- **WDPBPtr** = WDPBRec*
- **FCBPBPtr** = FCBPBRec*
- **VCBPtr** = VCB*
- **FSSpecPtr** = FSSpec*
- **FSSpecArrayPtr** = FSSpecPtr

## Enums

- **?**
- **?**
- **?**
- **?**
- **?**
- **ParamBlkType**
- **CInfoType**

### Enum Values

**anonymous**:

- `fOnDesk` = 1
- `fHasBundle` = 8192
- `fInvisible` = 16384
- `fTrash` = -3
- `fDesktop` = -2
- `fDisk` = 0

**anonymous**:

- `fsCurPerm` = 0
- `fsRdPerm` = 1
- `fsWrPerm` = 2
- `fsRdWrPerm` = 3
- `fsRdWrShPerm` = 4
- `fsRdDenyPerm` = 16
- `fsWrDenyPerm` = 32

**anonymous**:

- `fsAtMark` = 0
- `fsFromStart` = 1
- `fsFromLEOF` = 2
- `fsFromMark` = 3
- `rdVerify` = 64

**anonymous**:

- `badMDBErr` = -60
- `badMovErr` = -122
- `bdNamErr` = -37
- `dirFulErr` = -33
- `dskFulErr` = -34
- `dupFNErr` = -48
- `eofErr` = -39
- `extFSErr` = -58
- `fBsyErr` = -47
- `fLckdErr` = -45
- `fnfErr` = -43
- `fnOpnErr` = -38
- `fsRnErr` = -59
- `gfpErr` = -52
- `ioErr` = -36
- `noMacDskErr` = -57
- `nsDrvErr` = -56
- `nsvErr` = -35
- `opWrErr` = -49
- `permErr` = -54
- `posErr` = -40
- `rfNumErr` = -51
- `tmfoErr` = -42
- `volOffLinErr` = -53
- `volOnLinErr` = -55
- `vLckdErr` = -46
- `wrgVolTypErr` = -123
- `wrPermErr` = -61
- `wPrErr` = -44
- `tmwdoErr` = -121
- `dirNFErr` = -120
- `fsDSIntErr` = -127

**anonymous**:

- `wrgVolTypeErr` = -123
- `diffVolErr` = -1303

**ParamBlkType**:

- `ioParamType` = ?
- `fileParamType` = ?
- `volumeParamType` = ?
- `cntrlParamType` = ?

**CInfoType**:

- `hfileInfo` = ?
- `dirInfo` = ?

## Structs

- **FInfo** { fdType: OSType, fdCreator: OSType, fdFlags: uint16_t, fdLocation: Point, fdFldr: uint16_t }
- **FXInfo** { fdIconID: uint16_t, fdUnused: uint16_t[4], fdComment: uint16_t, fdPutAway: LONGINT }
- **DInfo** { frRect: Rect, frFlags: uint16_t, frLocation: Point, frView: uint16_t }
- **DXInfo** { frScroll: Point, frOpenChain: LONGINT, frUnused: uint16_t, frComment: uint16_t, frPutAway: LONGINT }
- **IOParam** { ?: ?, ioRefNum: INTEGER, ioVersNum: SignedByte, ioPermssn: SignedByte, ioMisc: IOMiscType, ioBuffer: Ptr, ioReqCount: LONGINT, ioActCount: LONGINT, ioPosMode: INTEGER, ioPosOffset: LONGINT }
- **FileParam** { ?: ?, ioFRefNum: INTEGER, ioFVersNum: SignedByte, filler1: SignedByte, ioFDirIndex: INTEGER, ioFlAttrib: SignedByte, ioFlVersNum: SignedByte, ioFlFndrInfo: FInfo, ioFlNum: LONGINT, ioFlStBlk: INTEGER, ioFlLgLen: LONGINT, ioFlPyLen: LONGINT, ioFlRStBlk: INTEGER, ioFlRLgLen: LONGINT, ioFlRPyLen: LONGINT, ioFlCrDat: ULONGINT, ioFlMdDat: ULONGINT }
- **VolumeParam** { ?: ?, filler2: LONGINT, ioVolIndex: INTEGER, ioVCrDate: ULONGINT, ioVLsBkUp: ULONGINT, ioVAtrb: uint16_t, ioVNmFls: uint16_t, ioVDirSt: uint16_t, ioVBlLn: uint16_t, ioVNmAlBlks: uint16_t, ioVAlBlkSiz: LONGINT, ioVClpSiz: LONGINT, ioAlBlSt: uint16_t, ioVNxtFNum: LONGINT, ioVFrBlk: uint16_t }
- **CntrlParam** { ?: ?, ioCRefNum: INTEGER, csCode: INTEGER, csParam: INTEGER[11] }
- **HIoParam** { ?: ?, ioRefNum: INTEGER, ioVersNum: SignedByte, ioPermssn: SignedByte, ioMisc: IOMiscType, ioBuffer: Ptr, ioReqCount: LONGINT, ioActCount: LONGINT, ioPosMode: INTEGER, ioPosOffset: LONGINT }
- **HFileParam** { ?: ?, ioFRefNum: INTEGER, ioFVersNum: SignedByte, filler1: SignedByte, ioFDirIndex: INTEGER, ioFlAttrib: SignedByte, ioFlVersNum: SignedByte, ioFlFndrInfo: FInfo, ioDirID: LONGINT, ioFlStBlk: INTEGER, ioFlLgLen: LONGINT, ioFlPyLen: LONGINT, ioFlRStBlk: INTEGER, ioFlRLgLen: LONGINT, ioFlRPyLen: LONGINT, ioFlCrDat: ULONGINT, ioFlMdDat: ULONGINT }
- **HVolumeParam** { ?: ?, pfiller2: LONGINT, ioVolIndex: INTEGER, ioVCrDate: ULONGINT, ioVLsMod: ULONGINT, ioVAtrb: INTEGER, ioVNmFls: uint16_t, ioVBitMap: uint16_t, ioVAllocPtr: uint16_t, ioVNmAlBlks: uint16_t, ioVAlBlkSiz: LONGINT, ioVClpSiz: LONGINT, ioAlBlSt: uint16_t, ioVNxtCNID: LONGINT, ioVFrBlk: uint16_t, ioVSigWord: uint16_t, ioVDrvInfo: INTEGER, ioVDRefNum: INTEGER, ioVFSID: INTEGER, ioVBkUp: LONGINT, ioVSeqNum: uint16_t, ioVWrCnt: LONGINT, ioVFilCnt: LONGINT, ioVDirCnt: LONGINT, ioVFndrInfo: LONGINT[8] }
- **HFileInfo** { ?: ?, ioFlFndrInfo: FInfo, ioDirID: LONGINT, ioFlStBlk: INTEGER, ioFlLgLen: LONGINT, ioFlPyLen: LONGINT, ioFlRStBlk: INTEGER, ioFlRLgLen: LONGINT, ioFlRPyLen: LONGINT, ioFlCrDat: ULONGINT, ioFlMdDat: ULONGINT, ioFlBkDat: ULONGINT, ioFlXFndrInfo: FXInfo, ioFlParID: LONGINT, ioFlClpSiz: LONGINT }
- **DirInfo** { ?: ?, ioDrUsrWds: DInfo, ioDrDirID: LONGINT, ioDrNmFls: uint16_t, filler3: uint16_t[9], ioDrCrDat: ULONGINT, ioDrMdDat: ULONGINT, ioDrBkDat: ULONGINT, ioDrFndrInfo: DXInfo, ioDrParID: LONGINT }
- **CMovePBRec** { ?: ?, filler1: LONGINT, ioNewName: StringPtr, filler2: LONGINT, ioNewDirID: LONGINT, filler3: LONGINT[2], ioDirID: LONGINT }
- **WDPBRec** { ?: ?, filler1: uint16_t, ioWDIndex: INTEGER, ioWDProcID: LONGINT, ioWDVRefNum: INTEGER, filler2: INTEGER[7], ioWDDirID: LONGINT }
- **FCBPBRec** { ?: ?, ioRefNum: INTEGER, filler: uint16_t, ioFCBIndx: INTEGER, ioFCBobnoxiousfiller: INTEGER, ioFCBFlNm: LONGINT, ioFCBFlags: uint16_t, ioFCBStBlk: INTEGER, ioFCBEOF: LONGINT, ioFCBPLen: LONGINT, ioFCBCrPs: LONGINT, ioFCBVRefNum: INTEGER, ioFCBClpSiz: LONGINT, ioFCBParID: LONGINT }
- **VCB** { qLink: QElemPtr, qType: INTEGER, vcbFlags: uint16_t, vcbSigWord: uint16_t, vcbCrDate: ULONGINT, vcbLsMod: ULONGINT, vcbAtrb: uint16_t, vcbNmFls: uint16_t, vcbVBMSt: uint16_t, vcbAllocPtr: uint16_t, vcbNmAlBlks: uint16_t, vcbAlBlkSiz: LONGINT, vcbClpSiz: LONGINT, vcbAlBlSt: uint16_t, vcbNxtCNID: LONGINT, vcbFreeBks: uint16_t, vcbVN: Byte[28], vcbDrvNum: INTEGER, vcbDRefNum: INTEGER, vcbFSID: INTEGER, vcbVRefNum: INTEGER, vcbMAdr: Ptr, vcbBufAdr: Ptr, vcbMLen: uint16_t, vcbDirIndex: INTEGER, vcbDirBlk: uint16_t, vcbVolBkUp: LONGINT, vcbVSeqNum: uint16_t, vcbWrCnt: LONGINT, vcbXTClpSiz: LONGINT, vcbCTClpSiz: LONGINT, vcbNmRtDirs: uint16_t, vcbFilCnt: LONGINT, vcbDirCnt: LONGINT, vcbFndrInfo: LONGINT[8], vcbVCSize: uint16_t, vcbVBMCSiz: uint16_t, vcbCtlCSiz: uint16_t, vcbXTAlBlks: uint16_t, vcbCTAlBlks: uint16_t, vcbXTRef: INTEGER, vcbCTRef: INTEGER, vcbCtlBuf: Ptr, vcbDirIDM: LONGINT, vcbOffsM: uint16_t }
- **DrvQEl** { qLink: QElemPtr, qType: INTEGER, dQDrive: INTEGER, dQRefNum: INTEGER, dQFSID: INTEGER, dQDrvSz: uint16_t, dQDrvSz2: uint16_t }
- **FSSpec** { vRefNum: INTEGER, parID: LONGINT, name: Str63 } — data types introduced by the new high level file system dispatch traps
- **FSSpec** {  }

## Unions

- **ParamBlockRec** { ioParam: IOParam, fileParam: FileParam, volumeParam: VolumeParam, cntrlParam: CntrlParam }
- **HParamBlockRec** { ioParam: HIoParam, fileParam: HFileParam, volumeParam: HVolumeParam }
- **CInfoPBRec** { hFileInfo: HFileInfo, dirInfo: DirInfo }

## Common Blocks

- **COMMONFSQUEUEDEFS** { qLink: QElemPtr, qType: INTEGER, ioTrap: INTEGER, ioCmdAddr: Ptr, ioCompletion: ProcPtr, ioResult: OSErr, ioNamePtr: StringPtr, ioVRefNum: INTEGER }
- **COMMONCINFODEFS** { ?: ?, ioFRefNum: INTEGER, ioFVersNum: SignedByte, filler1: SignedByte, ioFDirIndex: INTEGER, ioFlAttrib: SignedByte, ioACUser: SignedByte }

## Dispatchers

- **FSDispatch**—
- **HighLevelFSDispatch**— prototypes for the high level filesystem dispatch traps — prototypes for the high level filesystem dispatch traps

## Low Memory Globals

- **BootDrive** @ 0x210 (INTEGER) — FileMgr IMIV-212 (true);
- **DrvQHdr** @ 0x308 (QHdr) — FileMgr IMIV-182 (true);
- **EjectNotify** @ 0x338 (ProcPtr) — FileMgr ThinkC (false);
- **FCBSPtr** @ 0x34E (Ptr) — FileMgr IMIV-179 (true);
- **DefVCBPtr** @ 0x352 (VCBPtr) — FileMgr IMIV-178 (true);
- **VCBQHdr** @ 0x356 (QHdr) — FileMgr IMIV-178 (true);
- **FSQHdr** @ 0x360 (QHdr) — FileMgr IMIV-176 (true);
- **WDCBsPtr** @ 0x372 (Ptr) — FileMgr idunno (true);
- **DefVRefNum** @ 0x384 (INTEGER) — FileMgr MPW (true);
- **ToExtFS** @ 0x3F2 (Ptr) — * Note: MacLinkPC+ loads 0x358 into a register (i.e. the address of the * pointer to the first element on the VCB queue) and then uses * 72 off of it (0x3A0) and 78 off of it (0x3A6). As long as * there are zeros there, that doesn't hurt us, but normally, * we'd have negative ones in there. Hence we describe them * here and set them to zero in executor. const LowMemGlobal<Ptr> FmtDefaults { 0x39E }; // FileMgr ThinkC (false); FileMgr IMIV-212 (false);
- **FSFCBLen** @ 0x3F6 (INTEGER) — FileMgr IMIV-97 (true);
