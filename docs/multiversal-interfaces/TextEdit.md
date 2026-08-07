# TextEdit Interfaces

new justification defines, accepted by `TESetAlignment ()' and `TETextBox ()'

Source: `multiversal/defs/TextEdit.yaml`

- Functions: **48**
- Typedefs: **18**
- Structs: **9**, Unions: **0**
- Enums: **7**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **1**
- Low-memory globals: **4**

## Functions

### TEActivate  

```c
void TEActivate(TEHandle teh)
```

Trap: `0xA9D8` executor=C_

### TEAutoView  

```c
void TEAutoView(Boolean autoflag, TEHandle teh)
```

Trap: `0xA813` executor=C_

### TECalText  

```c
void TECalText(TEHandle teh)
```

Trap: `0xA9D0` executor=C_

### TEClick  

```c
void TEClick(Point p, Boolean ext, TEHandle teh)
```

Trap: `0xA9D4` executor=C_

### TEContinuousStyle  

```c
Boolean TEContinuousStyle(INTEGER* modep, TextStyle* thestyle, TEHandle teh)
```

Trap: — (executor 实现，无 trap) executor=C_

### TECopy  

```c
void TECopy(TEHandle teh)
```

Trap: `0xA9D5` executor=C_

### TECustomHook  

```c
void TECustomHook(INTEGER sel, ProcPtr* addr, TEHandle teh)
```

Trap: — (executor 实现，无 trap) executor=C_

### TECut  

```c
void TECut(TEHandle teh)
```

Trap: `0xA9D6` executor=C_

### TEDeactivate  

```c
void TEDeactivate(TEHandle teh)
```

Trap: `0xA9D9` executor=C_

### TEDelete  

```c
void TEDelete(TEHandle teh)
```

Trap: `0xA9D7` executor=C_

### TEDispose  

```c
void TEDispose(TEHandle teh)
```

Trap: `0xA9CD` executor=C_

### TEFeatureFlag  

```c
int16_t TEFeatureFlag(int16_t feature, int16_t action, TEHandle te)
```

Trap: — (executor 实现，无 trap) executor=C_

### TEFromScrap  

```c
OSErr TEFromScrap()
```

Trap: — (executor 实现，无 trap) executor=True

### TEGetHeight  

```c
int32_t TEGetHeight(LONGINT endLine, LONGINT startLine, TEHandle teh)
```

Trap: — (executor 实现，无 trap) executor=C_

### TEGetOffset  

```c
INTEGER TEGetOffset(Point pt, TEHandle teh)
```

Trap: `0xA83C` executor=C_

### TEGetPoint  

```c
LONGINT TEGetPoint(INTEGER offset, TEHandle teh)
```

Trap: — (executor 实现，无 trap) executor=C_

### TEGetScrapLength  

```c
LONGINT TEGetScrapLength()
```

Trap: — (executor 实现，无 trap) executor=True

### TEGetStyle  

```c
void TEGetStyle(INTEGER offset, TextStyle* theStyle, INTEGER* lineHeight, INTEGER* fontAscent, TEHandle teh)
```

Trap: — (executor 实现，无 trap) executor=C_

### TEGetStyleHandle  

```c
TEStyleHandle TEGetStyleHandle(TEHandle teh)
```

Trap: — (executor 实现，无 trap) executor=C_

### TEGetStyleScrapHandle  

```c
StScrpHandle TEGetStyleScrapHandle(TEHandle teh)
```

Trap: — (executor 实现，无 trap) executor=C_

### TEGetText  

```c
CharsHandle TEGetText(TEHandle teh)
```

Trap: `0xA9CB` executor=C_

### TEIdle  

```c
void TEIdle(TEHandle teh)
```

Trap: `0xA9DA` executor=C_

### TEInit  

```c
void TEInit()
```

Trap: `0xA9CC` executor=C_

### TEInsert  

```c
void TEInsert(Ptr p, LONGINT ln, TEHandle teh)
```

Trap: `0xA9DE` executor=C_

### TEKey  

```c
void TEKey(CharParameter thec, TEHandle teh)
```

Trap: `0xA9DC` executor=C_

### TENew  

```c
TEHandle TENew(const Rect* dst, const Rect* view)
```

Trap: `0xA9D2` executor=C_

### TENumStyles  

```c
LONGINT TENumStyles(LONGINT start, LONGINT stop, TEHandle teh)
```

Trap: — (executor 实现，无 trap) executor=C_

### TEPaste  

```c
void TEPaste(TEHandle teh)
```

Trap: `0xA9DB` executor=C_

### TEPinScroll  

```c
void TEPinScroll(INTEGER dh, INTEGER dv, TEHandle teh)
```

Trap: `0xA812` executor=C_

### TEReplaceStyle  

```c
void TEReplaceStyle(INTEGER mode, TextStyle* oldStyle, TextStyle* newStyle, Boolean redraw, TEHandle teh)
```

Trap: — (executor 实现，无 trap) executor=C_

### TEScrapHandle  

```c
Handle TEScrapHandle()
```

Trap: — (executor 实现，无 trap) executor=True

### TEScroll  

```c
void TEScroll(INTEGER dh, INTEGER dv, TEHandle teh)
```

Trap: `0xA9DD` executor=C_

### TESelView  

```c
void TESelView(TEHandle teh)
```

Trap: `0xA811` executor=C_

### TESetAlignment  

```c
void TESetAlignment(INTEGER j, TEHandle teh)
```

Trap: `0xA9DF` executor=C_

### TESetClickLoop  

```c
void TESetClickLoop(ProcPtr cp, TEHandle teh)
```

Trap: — (executor 实现，无 trap) executor=True

### TESetScrapLength  

```c
void TESetScrapLength(LONGINT ln)
```

Trap: — (executor 实现，无 trap) executor=True

### TESetSelect  

```c
void TESetSelect(LONGINT start, LONGINT stop, TEHandle teh)
```

Trap: `0xA9D1` executor=C_

### TESetStyle  

```c
void TESetStyle(INTEGER mode, TextStyle* newStyle, Boolean redraw, TEHandle teh)
```

Trap: — (executor 实现，无 trap) executor=C_

### TESetStyleHandle  

```c
void TESetStyleHandle(TEStyleHandle theHandle, TEHandle teh)
```

Trap: — (executor 实现，无 trap) executor=C_

### TESetText  

```c
void TESetText(Ptr p, LONGINT ln, TEHandle teh)
```

Trap: `0xA9CF` executor=C_

### TESetWordBreak  

```c
void TESetWordBreak(ProcPtr wb, TEHandle teh)
```

Trap: — (executor 实现，无 trap) executor=True

### TEStyleInsert  

```c
void TEStyleInsert(Ptr text, LONGINT length, StScrpHandle hST, TEHandle teh)
```

Trap: — (executor 实现，无 trap) executor=C_

### TEStyleNew  

```c
TEHandle TEStyleNew(const Rect* dst, const Rect* view)
```

Trap: `0xA83E` executor=C_

### TEStylePaste  

```c
void TEStylePaste(TEHandle teh)
```

Trap: — (executor 实现，无 trap) executor=C_

### TETextBox  

```c
void TETextBox(Ptr p, LONGINT ln, const Rect* r, INTEGER j)
```

Trap: `0xA9CE` executor=C_

### TEToScrap  

```c
OSErr TEToScrap()
```

Trap: — (executor 实现，无 trap) executor=True

### TEUpdate  

```c
void TEUpdate(const Rect* r, TEHandle teh)
```

Trap: `0xA9D3` executor=C_

### TEUseStyleScrap  

```c
void TEUseStyleScrap(LONGINT start, LONGINT stop, StScrpHandle newstyles, Boolean redraw, TEHandle teh)
```

Trap: — (executor 实现，无 trap) executor=C_

## Typedefs

- **TEPtr** = TERec*
- **TEHandle** = TEPtr*
- **TEStyleTable** = STElement[1]
- **STPtr** = STElement*
- **STHandle** = STPtr*
- **LHTable** = LHElement[1]
- **LHPtr** = LHElement*
- **LHHandle** = LHPtr*
- **ScrpSTTable** = ScrpSTElement[1]
- **StScrpPtr** = StScrpRec*
- **StScrpHandle** = StScrpPtr*
- **NullSTPtr** = NullSTRec*
- **NullSTHandle** = NullSTPtr*
- **TEStylePtr** = TEStyleRec*
- **TEStyleHandle** = TEStylePtr*
- **Chars** = Byte[1]
- **CharsPtr** = Byte*
- **CharsHandle** = Byte**

## Enums

- **?** — new justification defines, accepted by `TESetAlignment ()' and `TETextBox ()'
- **?** — older justification defines
- **?**
- **?**
- **?**
- **?**
- **?**

### Enum Values

**anonymous** — new justification defines, accepted by `TESetAlignment ()' and `TETextBox ()':

- `teFlushDefault` = 0
- `teCenter` = 1
- `teFlushRight` = -1
- `teFlushLeft` = -2

**anonymous** — older justification defines:

- `teJustLeft` = 0
- `teJustCenter` = 1
- `teJustRight` = -1
- `teForceLeft` = -2

**anonymous**:

- `doFont` = 1
- `doFace` = 2
- `doSize` = 4
- `doColor` = 8
- `doAll` = 15
- `addSize` = 16
- `doToggle` = 32

**anonymous**:

- `teFind` = 0
- `teHilite` = 1
- `teDraw` = -1
- `teCaret` = -2

**anonymous**:

- `caret_vis` = -1
- `caret_invis` = 255
- `hilite_vis` = 0

**anonymous**:

- `teFAutoScroll` = 0
- `teFTextBuffering` = 1
- `teFOutlineHilite` = 2
- `teFInlineInput` = 3
- `teFUseTextServices` = 4

**anonymous**:

- `teBitClear` = 0
- `teBitSet` = 1
- `teBitTest` = -1

## Structs

- **TERec** { destRect: Rect, viewRect: Rect, selRect: Rect, lineHeight: INTEGER, fontAscent: INTEGER, selPoint: Point, selStart: INTEGER, selEnd: INTEGER, active: INTEGER, wordBreak: ProcPtr, clikLoop: ProcPtr, clickTime: LONGINT, clickLoc: INTEGER, caretTime: LONGINT, caretState: INTEGER, just: INTEGER, teLength: INTEGER, hText: Handle, recalBack: INTEGER, recalLines: INTEGER, clikStuff: INTEGER, crOnly: INTEGER, txFont: INTEGER, txFace: Style, filler: Byte, txMode: INTEGER, txSize: INTEGER, inPort: GrafPtr, highHook: ProcPtr, caretHook: ProcPtr, nLines: INTEGER, lineStarts: INTEGER[1] }
- **StyleRun** { startChar: INTEGER, styleIndex: INTEGER }
- **STElement** { stCount: INTEGER, stHeight: INTEGER, stAscent: INTEGER, stFont: INTEGER, stFace: Style, filler: Byte, stSize: INTEGER, stColor: RGBColor }
- **LHElement** { lhHeight: INTEGER, lhAscent: INTEGER }
- **TextStyle** { tsFont: INTEGER, tsFace: Style, filler: Byte, tsSize: INTEGER, tsColor: RGBColor }
- **ScrpSTElement** { scrpStartChar: LONGINT, scrpHeight: INTEGER, scrpAscent: INTEGER, scrpFont: INTEGER, scrpFace: Style, filler: Byte, scrpSize: INTEGER, scrpColor: RGBColor }
- **StScrpRec** { scrpNStyles: INTEGER, scrpStyleTab: ScrpSTTable }
- **NullSTRec** { TEReserved: LONGINT, nullScrap: StScrpHandle }
- **TEStyleRec** { nRuns: INTEGER, nStyles: INTEGER, styleTab: STHandle, lhTab: LHHandle, teRefCon: LONGINT, nullStyle: NullSTHandle, runs: StyleRun[1] }

## Dispatchers

- **TEDispatch**—

## Low Memory Globals

- **TEDoText** @ 0xA70 (ProcPtr) — TextEdit IMI-391 (true);
- **TERecal** @ 0xA74 (ProcPtr) — TextEdit IMI-391 (false);
- **TEScrpLength** @ 0xAB0 (INTEGER) — TextEdit IMI-389 (true);
- **TEScrpHandle** @ 0xAB4 (Handle) — TextEdit IMI-389 (true);
