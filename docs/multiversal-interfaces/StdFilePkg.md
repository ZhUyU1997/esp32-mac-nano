# StdFilePkg Interfaces

StdFilePkg IMIV-72 (true);

Source: `multiversal/defs/StdFilePkg.yaml`

- Functions: **8**
- Typedefs: **1**
- Structs: **2**, Unions: **0**
- Enums: **8**
- Function pointers: **5**
- Common blocks: **0**
- Dispatchers: **1**
- Low-memory globals: **2**

## Functions

### CustomGetFile  

```c
void CustomGetFile(FileFilterYDUPP filefilter, INTEGER numtypes, SFTypeList typelist, StandardFileReply* replyp, INTEGER dlgid, Point where, DlgHookYDUPP dlghook, ModalFilterYDUPP filterproc, Ptr activeList, ActivateYDUPP activateproc, void* yourdatap)
```

Trap: — (executor 实现，无 trap) executor=C_

### CustomPutFile  

```c
void CustomPutFile(ConstStringPtr prompt, ConstStringPtr defaultName, StandardFileReply* replyp, INTEGER dlgid, Point where, DlgHookYDUPP dlghook, ModalFilterYDUPP filterproc, Ptr activeList, ActivateYDUPP activateproc, void* yourdatap)
```

Trap: — (executor 实现，无 trap) executor=C_

### SFGetFile  

```c
void SFGetFile(Point p, ConstStringPtr prompt, FileFilterUPP filef, INTEGER numt, SFTypeList tl, DlgHookUPP dh, SFReply* rep)
```

Trap: — (executor 实现，无 trap) executor=C_

### SFPGetFile  

```c
void SFPGetFile(Point p, ConstStringPtr prompt, FileFilterUPP filef, INTEGER numt, SFTypeList tl, DlgHookUPP dh, SFReply* rep, INTEGER dig, ModalFilterUPP fp)
```

Trap: — (executor 实现，无 trap) executor=C_

### SFPPutFile  

```c
void SFPPutFile(Point p, ConstStringPtr prompt, ConstStringPtr name, DlgHookUPP dh, SFReply* rep, INTEGER dig, ModalFilterUPP fp)
```

Trap: — (executor 实现，无 trap) executor=C_

### SFPutFile  

```c
void SFPutFile(Point p, ConstStringPtr prompt, ConstStringPtr name, DlgHookUPP dh, SFReply* rep)
```

Trap: — (executor 实现，无 trap) executor=C_

### StandardGetFile  

```c
void StandardGetFile(FileFilterUPP filef, INTEGER numt, SFTypeList tl, StandardFileReply* replyp)
```

Trap: — (executor 实现，无 trap) executor=C_

### StandardPutFile  

```c
void StandardPutFile(ConstStringPtr prompt, ConstStringPtr defaultname, StandardFileReply* replyp)
```

Trap: — (executor 实现，无 trap) executor=C_

## Typedefs

- **SFTypeList** = OSType[4]

## Enums

- **?**
- **?**
- **?**
- **?**
- **?**
- **?**
- **?**
- **?**

### Enum Values

**anonymous**:

- `putDlgID` = -3999

**anonymous**:

- `putSave` = 1
- `putCancel` = 2
- `putEject` = 5
- `putDrive` = 6
- `putName` = 7

**anonymous**:

- `getDlgID` = -4000

**anonymous**:

- `getOpen` = 1
- `getCancel` = 3
- `getEject` = 5
- `getDrive` = 6
- `getNmList` = 7
- `getScroll` = 8

**anonymous**:

- `sfItemOpenButton` = 1
- `sfItemCancelButton` = 2
- `sfItemBalloonHelp` = 3
- `sfItemVolumeUser` = 4
- `sfItemEjectButton` = 5
- `sfItemDesktopButton` = 6
- `sfItemFileListUser` = 7
- `sfItemPopUpMenuUser` = 8
- `sfItemDividerLinePict` = 9
- `sfItemFileNameTextEdit` = 10
- `sfItemPromptStaticText` = 11
- `sfItemNewFolderUser` = 12

**anonymous**:

- `sfHookFirstCall` = -1
- `sfHookLastCall` = -2
- `sfHookNullEvent` = 100
- `sfHookRebuildList` = 101
- `sfHookFolderPopUp` = 102
- `sfHookOpenFolder` = 103
- `sfHookOpenAlias` = 104
- `sfHookGoToDesktop` = 105
- `sfHookGoToAliasTarget` = 106
- `sfHookGoToParent` = 107
- `sfHookGoToNextDrive` = 108
- `sfHookGoToPrevDrive` = 109
- `sfHookChangeSelection` = 110

**anonymous**:

- `sfMainDialogRefCon` = 'stdf'

**anonymous**:

- `sfPutDialogID` = -6043
- `sfGetDialogID` = -6042

## Structs

- **SFReply** { good: Boolean, copy: Boolean, fType: OSType, vRefNum: INTEGER, version: INTEGER, fName: Str63 }
- **StandardFileReply** { sfGood: Boolean, sfReplacing: Boolean, sfType: OSType, sfFile: FSSpec, sfScript: ScriptCode, sfFlags: INTEGER, sfIsFolder: Boolean, sfIsVolume: Boolean, sfReserved1: LONGINT, sfReserved2: INTEGER }

## Function Pointers

- **DlgHookUPP** (item: INTEGER, theDialog: DialogPtr) -> INTEGER
- **FileFilterUPP** (pb: CInfoPBPtr) -> Boolean
- **DlgHookYDUPP** (item: INTEGER, theDialog: DialogPtr, yourDataPtr: void*) -> INTEGER
- **FileFilterYDUPP** (pb: CInfoPBPtr, yourDataPtr: void*) -> Boolean
- **ActivateYDUPP** (theDialog: DialogPtr, itemNo: INTEGER, activating: Boolean, yourDataPtr: void*) -> void

## Dispatchers

- **Pack3**—

## Low Memory Globals

- **SFSaveDisk** @ 0x214 (INTEGER) — StdFilePkg IMIV-72 (true);
- **CurDirStore** @ 0x398 (LONGINT) — StdFilePkg IMIV-72 (true);
