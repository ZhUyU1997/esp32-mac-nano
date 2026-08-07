# ControlMgr Interfaces

control color table parts

Source: `multiversal/defs/ControlMgr.yaml`

- Functions: **31**
- Typedefs: **5**
- Structs: **4**, Unions: **0**
- Enums: **9**
- Function pointers: **1**
- Common blocks: **0**
- Dispatchers: **0**
- Low-memory globals: **1**

## Functions

### DisposeControl  

```c
void DisposeControl(ControlHandle c)
```

Trap: `0xA955` executor=C_

### DragControl  

```c
void DragControl(ControlHandle c, Point p, const Rect* limit, const Rect* slop, INTEGER axis)
```

Trap: `0xA967` executor=C_

### Draw1Control  

```c
void Draw1Control(ControlHandle c)
```

Trap: `0xA96D` executor=C_

### DrawControls  

```c
void DrawControls(WindowPtr w)
```

Trap: `0xA969` executor=C_

### FindControl  

```c
INTEGER FindControl(Point p, WindowPtr w, ControlHandle* cp)
```

Trap: `0xA96C` executor=C_

### GetAuxiliaryControlRecord  

```c
Boolean GetAuxiliaryControlRecord(ControlHandle c, AuxCtlHandle* acHndl)
```

Trap: `0xAA44` executor=C_

### GetControlAction  

```c
ControlActionUPP GetControlAction(ControlHandle c)
```

Trap: `0xA96A` executor=C_

### GetControlMaximum  

```c
INTEGER GetControlMaximum(ControlHandle c)
```

Trap: `0xA962` executor=C_

### GetControlMinimum  

```c
INTEGER GetControlMinimum(ControlHandle c)
```

Trap: `0xA961` executor=C_

### GetControlReference  

```c
LONGINT GetControlReference(ControlHandle c)
```

Trap: `0xA95A` executor=C_

### GetControlTitle  

```c
void GetControlTitle(ControlHandle c, StringPtr t)
```

Trap: `0xA95E` executor=C_

### GetControlValue  

```c
INTEGER GetControlValue(ControlHandle c)
```

Trap: `0xA960` executor=C_

### GetControlVariant  

```c
INTEGER GetControlVariant(ControlHandle c)
```

Trap: `0xA809` executor=C_

### GetNewControl  

```c
ControlHandle GetNewControl(INTEGER cid, WindowPtr wst)
```

Trap: `0xA9BE` executor=C_

### HideControl  

```c
void HideControl(ControlHandle c)
```

Trap: `0xA958` executor=C_

### HiliteControl  

```c
void HiliteControl(ControlHandle c, INTEGER state)
```

Trap: `0xA95D` executor=C_

### KillControls  

```c
void KillControls(WindowPtr w)
```

Trap: `0xA956` executor=C_

### MoveControl  

```c
void MoveControl(ControlHandle c, INTEGER h, INTEGER v)
```

Trap: `0xA959` executor=C_

### NewControl  

```c
ControlHandle NewControl(WindowPtr wst, const Rect* r, ConstStringPtr title, Boolean vis, INTEGER value, INTEGER min, INTEGER max, INTEGER procid, LONGINT rc)
```

Trap: `0xA954` executor=C_

### SetControlAction  

```c
void SetControlAction(ControlHandle c, ControlActionUPP a)
```

Trap: `0xA96B` executor=C_

### SetControlColor  

```c
void SetControlColor(ControlHandle ctl, CCTabHandle ctab)
```

Trap: `0xAA43` executor=C_

### SetControlMaximum  

```c
void SetControlMaximum(ControlHandle c, INTEGER v)
```

Trap: `0xA965` executor=C_

### SetControlMinimum  

```c
void SetControlMinimum(ControlHandle c, INTEGER v)
```

Trap: `0xA964` executor=C_

### SetControlReference  

```c
void SetControlReference(ControlHandle c, LONGINT data)
```

Trap: `0xA95B` executor=C_

### SetControlTitle  

```c
void SetControlTitle(ControlHandle c, ConstStringPtr t)
```

Trap: `0xA95F` executor=C_

### SetControlValue  

```c
void SetControlValue(ControlHandle c, INTEGER v)
```

Trap: `0xA963` executor=C_

### ShowControl  

```c
void ShowControl(ControlHandle c)
```

Trap: `0xA957` executor=C_

### SizeControl  

```c
void SizeControl(ControlHandle c, INTEGER width, INTEGER height)
```

Trap: `0xA95C` executor=C_

### TestControl  

```c
INTEGER TestControl(ControlHandle c, Point p)
```

Trap: `0xA966` executor=C_

### TrackControl  

```c
INTEGER TrackControl(ControlHandle c, Point p, ControlActionUPP a)
```

Trap: `0xA968` executor=C_

### UpdateControls  

```c
void UpdateControls(WindowPtr wp, RgnHandle rh)
```

Trap: `0xA953` executor=C_

## Typedefs

- **CCTabPtr** = CtlCTab*
- **CCTabHandle** = CCTabPtr*
- **AuxCtlPtr** = AuxCtlRec*
- **AuxCtlHandle** = AuxCtlPtr*
- **ControlRef** = ControlPtr

## Enums

- **?**
- **?**
- **?**
- **?**
- **?**
- **?**
- **?**
- **?** — control color table parts
- **?**

### Enum Values

**anonymous**:

- `pushButProc` = 0
- `checkBoxProc` = 1
- `radioButProc` = 2
- `useWFont` = 8
- `scrollBarProc` = 16

**anonymous**:

- `inButton` = 10
- `inCheckBox` = 11
- `inUpButton` = 20
- `inDownButton` = 21
- `inPageUp` = 22
- `inPageDown` = 23
- `inThumb` = 129

**anonymous**:

- `popupFixedWidth` = 1 << 0
- `popupUseAddResMenu` = 1 << 2
- `popupUseWFont` = 1 << 3

**anonymous**:

- `popupTitleBold` = 1 << 8
- `popupTitleItalic` = 1 << 9
- `popupTitleUnderline` = 1 << 10
- `popupTitleOutline` = 1 << 11
- `popupTitleShadow` = 1 << 12
- `popupTitleCondense` = 1 << 13
- `popupTitleExtend` = 1 << 14
- `popupTitleNoStyle` = 1 << 15

**anonymous**:

- `popupTitleLeftJust` = 0
- `popupTitleCenterJust` = 1
- `popupTitleRightJust` = 255

**anonymous**:

- `drawCntl` = 0
- `testCntl` = 1
- `calcCRgns` = 2
- `initCntl` = 3
- `dispCntl` = 4
- `posCntl` = 5
- `thumbCntl` = 6
- `dragCntl` = 7
- `autoTrack` = 8

**anonymous**:

- `calcCntlRgn` = 10
- `calcThumbRgn` = 11

**anonymous** — control color table parts:

- `cFrameColor` = 0
- `cBodyColor` = 1
- `cTextColor` = 2
- `cThumbColor` = 3

**anonymous**:

- `cArrowsColorLight` = 5
- `cArrowsColorDark` = 6
- `cThumbLight` = 7
- `cThumbDark` = 8
- `cHiliteLight` = 9
- `cHiliteDark` = 10
- `cTitleBarLight` = 11
- `cTitleBarDark` = 12
- `cTingeLight` = 13
- `cTingeDark` = 14

## Structs

- **ControlRecord** { nextControl: ControlHandle, contrlOwner: WindowPtr, contrlRect: Rect, contrlVis: Byte, contrlHilite: Byte, contrlValue: INTEGER, contrlMin: INTEGER, contrlMax: INTEGER, contrlDefProc: Handle, contrlData: Handle, contrlAction: ControlActionUPP, contrlRfCon: LONGINT, contrlTitle: Str255 }
- **CtlCTab** { ccSeed: LONGINT, ccReserved: INTEGER, ctSize: INTEGER, ctTable: cSpecArray }
- **AuxCtlRec** {  }
- **AuxCtlRec** { acNext: AuxCtlHandle, acOwner: ControlHandle, acCTable: CCTabHandle, acFlags: INTEGER, acReserved: LONGINT, acRefCon: LONGINT }

## Function Pointers

- **ControlActionUPP** (?: ControlHandle, ?: int16_t) -> void

## Low Memory Globals

- **AuxCtlHead** @ 0xCD4 (AuxCtlHandle) — ControlMgr IMV-216 (true);
