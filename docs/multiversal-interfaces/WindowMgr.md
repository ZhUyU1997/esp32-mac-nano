# WindowMgr Interfaces

color table entries

Source: `multiversal/defs/WindowMgr.yaml`

- Functions: **57**
- Typedefs: **8**
- Structs: **5**, Unions: **0**
- Enums: **11**
- Function pointers: **1**
- Common blocks: **0**
- Dispatchers: **0**
- Low-memory globals: **18**

## Functions

### BeginUpdate  

```c
void BeginUpdate(WindowPtr w)
```

Trap: `0xA922` executor=C_

### BringToFront  

```c
void BringToFront(WindowPtr w)
```

Trap: `0xA920` executor=C_

### CalcVis  

```c
void CalcVis(WindowPeek w)
```

Trap: `0xA909` executor=C_

### CalcVisBehind  

```c
void CalcVisBehind(WindowPeek w, RgnHandle clobbered)
```

Trap: `0xA90A` executor=C_

### CheckUpdate  

```c
Boolean CheckUpdate(EventRecord* ev)
```

Trap: `0xA911` executor=C_

### ClipAbove  

```c
void ClipAbove(WindowPeek w)
```

Trap: `0xA90B` executor=C_

### CloseWindow  

```c
void CloseWindow(WindowPtr w)
```

Trap: `0xA92D` executor=C_

### DisposeWindow  

```c
void DisposeWindow(WindowPtr w)
```

Trap: `0xA914` executor=C_

### DragGrayRgn  

```c
LONGINT DragGrayRgn(RgnHandle rgn, Point startp, const Rect* limit, const Rect* slop, INTEGER axis, ProcPtr proc)
```

Trap: `0xA905` executor=C_

### DragTheRgn  

```c
LONGINT DragTheRgn(RgnHandle rgn, Point startp, const Rect* limit, const Rect* slop, INTEGER axis, ProcPtr proc)
```

Trap: `0xA926` executor=C_

### DragWindow  

```c
void DragWindow(WindowPtr wp, Point p, const Rect* rp)
```

Trap: `0xA925` executor=C_

### DrawGrowIcon  

```c
void DrawGrowIcon(WindowPtr w)
```

Trap: `0xA904` executor=C_

### DrawNew  

```c
void DrawNew(WindowPeek w, Boolean flag)
```

Trap: `0xA90F` executor=C_

### EndUpdate  

```c
void EndUpdate(WindowPtr w)
```

Trap: `0xA923` executor=C_

### FindWindow  

```c
INTEGER FindWindow(Point p, WindowPtr* wpp)
```

Trap: `0xA92C` executor=C_

### FrontWindow  

```c
WindowPtr FrontWindow()
```

Trap: `0xA924` executor=C_

### GetAuxWin  

```c
Boolean GetAuxWin(WindowPtr ?, AuxWinHandle* ?)
```

Trap: `0xAA42` executor=C_

### GetNewCWindow  

```c
WindowPtr GetNewCWindow(INTEGER ?, void* ?, WindowPtr ?)
```

Trap: `0xAA46` executor=C_

### GetNewWindow  

```c
WindowPtr GetNewWindow(INTEGER wid, void* wst, WindowPtr behind)
```

Trap: `0xA9BD` executor=C_

### GetWMgrPort  

```c
void GetWMgrPort(GrafPtr* wp)
```

Trap: `0xA910` executor=C_

### GetWRefCon  

```c
LONGINT GetWRefCon(WindowPtr w)
```

Trap: `0xA917` executor=C_

### GetWTitle  

```c
void GetWTitle(WindowPtr w, StringPtr t)
```

Trap: `0xA919` executor=C_

### GetWVariant  

```c
INTEGER GetWVariant(WindowPtr w)
```

Trap: `0xA80A` executor=C_

### GetWindowKind  

```c
INTEGER GetWindowKind(WindowRef win)
```

Trap: — (executor 实现，无 trap)

### GetWindowPic  

```c
PicHandle GetWindowPic(WindowPtr w)
```

Trap: `0xA92F` executor=C_

### GetWindowPortBounds  

```c
void GetWindowPortBounds(WindowRef window, Rect* bounds)
```

Trap: — (executor 实现，无 trap)

### GrowWindow  

```c
LONGINT GrowWindow(WindowPtr w, Point startp, const Rect* rp)
```

Trap: `0xA92B` executor=C_

### HideWindow  

```c
void HideWindow(WindowPtr w)
```

Trap: `0xA916` executor=C_

### HiliteWindow  

```c
void HiliteWindow(WindowPtr w, Boolean flag)
```

Trap: `0xA91C` executor=C_

### InitWindows  

```c
void InitWindows()
```

Trap: `0xA912` executor=C_

### InvalRect  

```c
void InvalRect(const Rect* r)
```

Trap: `0xA928` executor=C_

### InvalRgn  

```c
void InvalRgn(RgnHandle r)
```

Trap: `0xA927` executor=C_

### MoveWindow  

```c
void MoveWindow(WindowPtr wp, INTEGER h, INTEGER v, Boolean front)
```

Trap: `0xA91B` executor=C_

### NewCWindow  

```c
WindowPtr NewCWindow(void* storage, const Rect* ?, ConstStringPtr ?, Boolean ?, INTEGER ?, WindowPtr ?, Boolean ?, LONGINT ?)
```

Trap: `0xAA45` executor=C_

### NewWindow  

```c
WindowPtr NewWindow(void* wst, const Rect* r, ConstStringPtr title, Boolean vis, INTEGER procid, WindowPtr behind, Boolean gaflag, LONGINT rc)
```

Trap: `0xA913` executor=C_

### PaintBehind  

```c
void PaintBehind(WindowPeek w, RgnHandle clobbered)
```

Trap: `0xA90D` executor=C_

### PaintOne  

```c
void PaintOne(WindowPeek w, RgnHandle clobbered)
```

Trap: `0xA90C` executor=C_

### PinRect  

```c
LONGINT PinRect(const Rect* r, Point p)
```

Trap: `0xA94E` executor=C_

### SaveOld  

```c
void SaveOld(WindowPeek w)
```

Trap: `0xA90E` executor=C_

### SelectWindow  

```c
void SelectWindow(WindowPtr w)
```

Trap: `0xA91F` executor=C_

### SendBehind  

```c
void SendBehind(WindowPtr w, WindowPtr behind)
```

Trap: `0xA921` executor=C_

### SetDeskCPat  

```c
void SetDeskCPat(PixPatHandle ?)
```

Trap: `0xAA47` executor=C_

### SetPortWindowPort  

```c
void SetPortWindowPort(WindowRef win)
```

Trap: — (executor 实现，无 trap)

### SetWRefCon  

```c
void SetWRefCon(WindowPtr w, LONGINT data)
```

Trap: `0xA918` executor=C_

### SetWTitle  

```c
void SetWTitle(WindowPtr w, ConstStringPtr t)
```

Trap: `0xA91A` executor=C_

### SetWinColor  

```c
void SetWinColor(WindowPtr w, CTabHandle new_w_ctab)
```

Trap: `0xAA41` executor=C_

### SetWindowPic  

```c
void SetWindowPic(WindowPtr w, PicHandle p)
```

Trap: `0xA92E` executor=C_

### ShowHide  

```c
void ShowHide(WindowPtr w, Boolean flag)
```

Trap: `0xA908` executor=C_

### ShowWindow  

```c
void ShowWindow(WindowPtr w)
```

Trap: `0xA915` executor=C_

### SizeWindow  

```c
void SizeWindow(WindowPtr w, INTEGER width, INTEGER height, Boolean flag)
```

Trap: `0xA91D` executor=C_

### TrackBox  

```c
Boolean TrackBox(WindowPtr wp, Point pt, INTEGER part)
```

Trap: `0xA83B` executor=C_

### TrackGoAway  

```c
Boolean TrackGoAway(WindowPtr w, Point p)
```

Trap: `0xA91E` executor=C_

### ValidRect  

```c
void ValidRect(const Rect* r)
```

Trap: `0xA92A` executor=C_

### ValidRgn  

```c
void ValidRgn(RgnHandle r)
```

Trap: `0xA929` executor=C_

### ZoomWindow  

```c
void ZoomWindow(WindowPtr wp, INTEGER part, Boolean front)
```

Trap: `0xA83A` executor=C_

### GetWindowPort  

```c
CGrafPtr GetWindowPort(WindowRef w)
```

Trap: — (executor 实现，无 trap) **[carbon]**

### InvalWindowRect  

```c
void InvalWindowRect(WindowRef w, const Rect* r)
```

Trap: — (executor 实现，无 trap) **[carbon]**

## Typedefs

- **WindowPtr** = GrafPtr
- **CWindowPtr** = CGrafPtr
- **ControlPtr** = ControlRecord*
- **ControlHandle** = ControlPtr*
- **WindowPeek** = WindowRecord*
- **AuxWinPtr** = AuxWinRec*
- **AuxWinHandle** = AuxWinPtr*
- **WindowRef** = WindowPtr

## Enums

- **?**
- **?**
- **?**
- **?**
- **?**
- **?**
- **?**
- **?** — color table entries
- **?**
- **?**
- **?**

### Enum Values

**anonymous**:

- `documentProc` = 0
- `dBoxProc` = 1
- `plainDBox` = 2
- `altDBoxProc` = 3
- `noGrowDocProc` = 4
- `movableDBoxProc` = 5
- `rDocProc` = 16

**anonymous**:

- `dialogKind` = 2
- `userKind` = 8

**anonymous**:

- `inDesk` = 0
- `inMenuBar` = 1
- `inSysWindow` = 2
- `inContent` = 3
- `inDrag` = 4
- `inGrow` = 5
- `inGoAway` = 6

**anonymous**:

- `noConstraint` = 0
- `hAxisOnly` = 1
- `vAxisOnly` = 2

**anonymous**:

- `wDraw` = 0
- `wHit` = 1
- `wCalcRgns` = 2
- `wNew` = 3
- `wDispose` = 4
- `wGrow` = 5
- `wDrawGIcon` = 6

**anonymous**:

- `kWindowMsgDraw` = 0
- `kWindowMsgHitTest` = 1
- `kWindowMsgCalculateShape` = 2
- `kWindowMsgInitialize` = 3
- `kWindowMsgCleanUp` = 4
- `kWindowMsgDrawGrowOutline` = 5
- `kWindowMsgDrawGrowBox` = 6

**anonymous**:

- `wNoHit` = 0
- `wInContent` = 1
- `wInDrag` = 2
- `wInGrow` = 3
- `wInGoAway` = 4

**anonymous** — color table entries:

- `wContentColor` = 0
- `wFrameColor` = 1
- `wTextColor` = 2
- `wHiliteColor` = 3
- `wTitleBarColor` = 4
- `wHiliteColorLight` = 5
- `wHiliteColorDark` = 6
- `wTitleBarLight` = 7
- `wTitleBarDark` = 8
- `wDialogLight` = 9
- `wDialogDark` = 10
- `wTingeLight` = 11
- `wTingeDark` = 12

**anonymous**:

- `deskPatID` = 16

**anonymous**:

- `inZoomIn` = 7
- `inZoomOut` = 8

**anonymous**:

- `wInZoomIn` = 5
- `wInZoomOut` = 6

## Structs

- **ControlRecord** {  }
- **WindowRecord** { port: GrafPort, windowKind: INTEGER, visible: Boolean, hilited: Boolean, goAwayFlag: Boolean, spareFlag: Boolean, strucRgn: RgnHandle, contRgn: RgnHandle, updateRgn: RgnHandle, windowDefProc: Handle, dataHandle: Handle, titleHandle: StringHandle, titleWidth: INTEGER, controlList: ControlHandle, nextWindow: WindowRecord*, windowPic: PicHandle, refCon: LONGINT }
- **WStateData** { userState: Rect, stdState: Rect }
- **AuxWinRec** {  }
- **AuxWinRec** { awNext: AuxWinHandle, awOwner: WindowPtr, awCTable: CTabHandle, dialogCItem: Handle, awFlags: LONGINT, awReserved: CTabHandle, awRefCon: LONGINT }

## Function Pointers

- **WindowDefUPP** (varcode: INTEGER, wp: WindowPtr, message: INTEGER, param: LONGINT) -> LONGINT

## Low Memory Globals

- **WindowList** @ 0x9D6 (WindowPeek) — WindowMgr IMI-274 (true);
- **SaveUpdate** @ 0x9DA (INTEGER) — WindowMgr IMI-297 (true);
- **PaintWhite** @ 0x9DC (INTEGER) — WindowMgr IMI-297 (true);
- **WMgrPort** @ 0x9DE (GrafPtr) — WindowMgr IMI-282 (true);
- **WMgrCPort** @ 0xD2C (CGrafPtr) — QuickDraw IMV-205 (false);
- **OldStructure** @ 0x9E6 (RgnHandle) — WindowMgr IMI-296 (true);
- **OldContent** @ 0x9EA (RgnHandle) — WindowMgr IMI-296 (true);
- **GrayRgn** @ 0x9EE (RgnHandle) — WindowMgr IMI-282 (true);
- **SaveVisRgn** @ 0x9F2 (RgnHandle) — WindowMgr IMI-293 (true);
- **DragHook** @ 0x9F6 (ProcPtr) — WindowMgr IMI-324 (true);
- **DragPattern** @ 0xA34 (Pattern) — WindowMgr IMI-324 (true);
- **DeskPattern** @ 0xA3C (Pattern) — WindowMgr IMI-282 (true);
- **CurActivate** @ 0xA64 (WindowPtr) — WindowMgr IMI-280 (true);
- **CurDeactive** @ 0xA68 (WindowPtr) — WindowMgr IMI-280 (true);
- **DeskHook** @ 0xA6C (ProcPtr) — WindowMgr IMI-282 (true);
- **GhostWindow** @ 0xA84 (WindowPtr) — WindowMgr IMI-287 (true);
- **AuxWinHead** @ 0xCD0 (AuxWinHandle) — WindowMgr IMV-200 (true);
- **DeskCPat** @ 0xCD8 (PixPatHandle) — WindowMgr SysEqua.a (true);
