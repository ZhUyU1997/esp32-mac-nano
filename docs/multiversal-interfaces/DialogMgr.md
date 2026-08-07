# DialogMgr Interfaces

DialogMgr IMI-411 (true);

Source: `multiversal/defs/DialogMgr.yaml`

- Functions: **43**
- Typedefs: **12**
- Structs: **3**, Unions: **0**
- Enums: **7**
- Function pointers: **4**
- Common blocks: **0**
- Dispatchers: **1**
- Low-memory globals: **6**

## Functions

### Alert  

```c
INTEGER Alert(INTEGER id, ModalFilterUPP fp)
```

Trap: `0xA985` executor=C_

### AppendDITL  

```c
void AppendDITL(DialogPtr ?, Handle ?, DITLMethod ?)
```

Trap: — (executor 实现，无 trap)

### CautionAlert  

```c
INTEGER CautionAlert(INTEGER id, ModalFilterUPP fp)
```

Trap: `0xA988` executor=C_

### CloseDialog  

```c
void CloseDialog(DialogPtr dp)
```

Trap: `0xA982` executor=C_

### CouldAlert  

```c
void CouldAlert(INTEGER id)
```

Trap: `0xA989` executor=C_

### CouldDialog  

```c
void CouldDialog(INTEGER id)
```

Trap: `0xA979` executor=C_

### CountDITL  

```c
int16_t CountDITL(DialogPtr ?)
```

Trap: — (executor 实现，无 trap)

### DialogCopy  

```c
void DialogCopy(DialogPtr dp)
```

Trap: — (executor 实现，无 trap)

### DialogCut  

```c
void DialogCut(DialogPtr dp)
```

Trap: — (executor 实现，无 trap)

### DialogDelete  

```c
void DialogDelete(DialogPtr dp)
```

Trap: — (executor 实现，无 trap)

### DialogPaste  

```c
void DialogPaste(DialogPtr dp)
```

Trap: — (executor 实现，无 trap)

### DialogSelect  

```c
Boolean DialogSelect(EventRecord* evt, DialogPtr* dpp, INTEGER* item)
```

Trap: `0xA980` executor=C_

### DisposeDialog  

```c
void DisposeDialog(DialogPtr dp)
```

Trap: `0xA983` executor=C_

### DrawDialog  

```c
void DrawDialog(DialogPtr dp)
```

Trap: `0xA981` executor=C_

### ErrorSound  

```c
void ErrorSound(SoundUPP sp)
```

Trap: `0xA98C` executor=C_

### FindDialogItem  

```c
INTEGER FindDialogItem(DialogPtr dp, Point pt)
```

Trap: `0xA984` executor=C_

### FreeAlert  

```c
void FreeAlert(INTEGER id)
```

Trap: `0xA98A` executor=C_

### FreeDialog  

```c
void FreeDialog(INTEGER id)
```

Trap: `0xA97A` executor=C_

### GetAlertStage  

```c
INTEGER GetAlertStage()
```

Trap: — (executor 实现，无 trap)

### GetDialogItem  

```c
void GetDialogItem(DialogPtr dp, INTEGER itemno, INTEGER* itype, Handle* item, Rect* r)
```

Trap: `0xA98D` executor=C_

### GetDialogItemText  

```c
void GetDialogItemText(Handle item, StringPtr text)
```

Trap: `0xA990` executor=C_

### GetNewDialog  

```c
DialogPtr GetNewDialog(INTEGER id, void* dst, WindowPtr behind)
```

Trap: `0xA97C` executor=C_

### GetStdFilterProc  

```c
OSErr GetStdFilterProc(ProcPtr* proc)
```

Trap: — (executor 实现，无 trap) executor=C_

### HideDialogItem  

```c
void HideDialogItem(DialogPtr dp, INTEGER item)
```

Trap: `0xA827` executor=C_

### InitDialogs  

```c
void InitDialogs(ProcPtr rp)
```

Trap: `0xA97B` executor=C_

### IsDialogEvent  

```c
Boolean IsDialogEvent(EventRecord* evt)
```

Trap: `0xA97F` executor=C_

### ModalDialog  

```c
void ModalDialog(ModalFilterUPP fp, INTEGER* item)
```

Trap: `0xA991` executor=C_

### NewColorDialog  

```c
DialogPtr NewColorDialog(void* ?, const Rect* ?, ConstStringPtr ?, Boolean ?, INTEGER ?, WindowPtr ?, Boolean ?, LONGINT ?, Handle ?)
```

Trap: `0xAA4B` executor=C_

### NewDialog  

```c
DialogPtr NewDialog(void* dst, const Rect* r, ConstStringPtr tit, Boolean vis, INTEGER procid, WindowPtr behind, Boolean gaflag, LONGINT rc, Handle items)
```

Trap: `0xA97D` executor=C_

### NoteAlert  

```c
INTEGER NoteAlert(INTEGER id, ModalFilterUPP fp)
```

Trap: `0xA987` executor=C_

### ParamText  

```c
void ParamText(ConstStringPtr p0, ConstStringPtr p1, ConstStringPtr p2, ConstStringPtr p3)
```

Trap: `0xA98B` executor=C_

### ResetAlertStage  

```c
void ResetAlertStage()
```

Trap: — (executor 实现，无 trap)

### SelectDialogItemText  

```c
void SelectDialogItemText(DialogPtr dp, INTEGER itemno, INTEGER start, INTEGER stop)
```

Trap: `0xA97E` executor=C_

### SetDialogCancelItem  

```c
OSErr SetDialogCancelItem(DialogPtr dialog, int16_t new_item)
```

Trap: — (executor 实现，无 trap) executor=C_

### SetDialogDefaultItem  

```c
OSErr SetDialogDefaultItem(DialogPtr dialog, int16_t new_item)
```

Trap: — (executor 实现，无 trap) executor=C_

### SetDialogFont  

```c
void SetDialogFont(INTEGER i)
```

Trap: — (executor 实现，无 trap)

### SetDialogItem  

```c
void SetDialogItem(DialogPtr dp, INTEGER itemno, INTEGER itype, Handle item, const Rect* r)
```

Trap: `0xA98E` executor=C_

### SetDialogItemText  

```c
void SetDialogItemText(Handle item, ConstStringPtr text)
```

Trap: `0xA98F` executor=C_

### SetDialogTracksCursor  

```c
OSErr SetDialogTracksCursor(DialogPtr dialog, Boolean tracks)
```

Trap: — (executor 实现，无 trap) executor=C_

### ShortenDITL  

```c
void ShortenDITL(DialogPtr ?, int16_t ?)
```

Trap: — (executor 实现，无 trap)

### ShowDialogItem  

```c
void ShowDialogItem(DialogPtr dp, INTEGER item)
```

Trap: `0xA828` executor=C_

### StopAlert  

```c
INTEGER StopAlert(INTEGER id, ModalFilterUPP fp)
```

Trap: `0xA986` executor=C_

### UpdateDialog  

```c
void UpdateDialog(DialogPtr dp, RgnHandle rgn)
```

Trap: `0xA978` executor=C_

## Typedefs

- **DialogPeek** = DialogRecord*
- **CDialogPtr** = CWindowPtr
- **DialogPtr** = WindowPtr
- **DialogTPtr** = DialogTemplate*
- **DialogTHndl** = DialogTPtr*
- **StageList** = int16_t
- **AlertTPtr** = AlertTemplate*
- **AlertTHndl** = AlertTPtr*
- **DITLMethod** = int16_t
- **DialogRef** = DialogPtr
- **DialogItemIndex** = int16_t
- **DialogItemType** = int16_t

## Enums

- **?**
- **?**
- **?**
- **?**
- **?**
- **?**
- **?**

### Enum Values

**anonymous**:

- `ctrlItem` = 4
- `btnCtrl` = 0
- `chkCtrl` = 1
- `radCtrl` = 2
- `resCtrl` = 3
- `statText` = 8
- `editText` = 16
- `iconItem` = 32
- `picItem` = 64
- `userItem` = 0
- `itemDisable` = 128

**anonymous**:

- `OK` = 1
- `Cancel` = 2

**anonymous**:

- `stopIcon` = 0
- `noteIcon` = 1
- `cautionIcon` = 2

**anonymous**:

- `overlayDITL` = 0
- `appendDITLRight` = 1
- `appendDITLBottom` = 2

**anonymous**:

- `TEdoFont` = 1
- `TEdoFace` = 2
- `TEdoSize` = 4
- `TEdoColor` = 8
- `TEdoAll` = 15

**anonymous**:

- `TEaddSize` = 16

**anonymous**:

- `doBColor` = 8192
- `doMode` = 16384
- `doFontName` = 32768

## Structs

- **DialogRecord** { window: WindowRecord, items: Handle, textH: TEHandle, editField: INTEGER, editOpen: INTEGER, aDefItem: INTEGER }
- **DialogTemplate** { boundsRect: Rect, procID: INTEGER, visible: Boolean, filler1: Boolean, goAwayFlag: Boolean, filler2: Boolean, refCon: LONGINT, itemsID: INTEGER, title: Str255 }
- **AlertTemplate** { boundsRect: Rect, itemsID: INTEGER, stages: StageList }

## Function Pointers

- **SoundUPP** (soundNumber: INTEGER) -> void
- **ModalFilterUPP** (theDialog: DialogPtr, theEvent: EventRecord*, itemHit: INTEGER*) -> Boolean
- **ModalFilterYDUPP** (theDialog: DialogPtr, theEvent: EventRecord*, itemHit: INTEGER*, yourDataPtr: void*) -> Boolean
- **UserItemUPP** (theDialog: DialogPtr, itemNo: INTEGER) -> void

## Dispatchers

- **DialogDispatch**—

## Low Memory Globals

- **ResumeProc** @ 0xA8C (ProcPtr) — DialogMgr IMI-411 (true);
- **ANumber** @ 0xA98 (INTEGER) — DialogMgr IMI-423 (true);
- **ACount** @ 0xA9A (INTEGER) — DialogMgr IMI-423 (true);
- **DABeeper** @ 0xA9C (SoundUPP) — DialogMgr IMI-411 (true);
- **DAStrings** @ 0xAA0 (Handle[4]) — DialogMgr IMI-421 (true);
- **DlgFont** @ 0xAFA (INTEGER) — DialogMgr IMI-412 (true);
