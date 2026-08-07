# FontMgr Interfaces

FontAssoc ffAssoc; WidTable ffWidthTab; StyleTable ffStyTab; KernTable ffKernTab;

Source: `multiversal/defs/FontMgr.yaml`

- Functions: **16**
- Typedefs: **5**
- Structs: **6**, Unions: **0**
- Enums: **6**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **1**
- Low-memory globals: **16**

## Functions

### FMSwapFont  

```c
FMOutPtr FMSwapFont(FMInput* fmip)
```

Trap: `0xA901` executor=C_

### FlushFonts  

```c
OSErr FlushFonts()
```

Trap: — (executor 实现，无 trap) executor=C_

### FontMetrics  

```c
void FontMetrics(FMetricRec* metrp)
```

Trap: `0xA835` executor=C_

### GetFNum  

```c
void GetFNum(ConstStringPtr fnam, INTEGER* fnum)
```

Trap: `0xA900` executor=C_

### GetFontName  

```c
void GetFontName(INTEGER fnum, StringPtr fnam)
```

Trap: `0xA8FF` executor=C_

### GetOutlinePreferred  

```c
Boolean GetOutlinePreferred()
```

Trap: — (executor 实现，无 trap) executor=C_

### GetPreserveGlyph  

```c
Boolean GetPreserveGlyph()
```

Trap: — (executor 实现，无 trap) executor=C_

### InitFonts  

```c
void InitFonts()
```

Trap: `0xA8FE` executor=C_

### IsOutline  

```c
Boolean IsOutline(Point numer, Point denom)
```

Trap: — (executor 实现，无 trap) executor=C_

### OutlineMetrics  

```c
OSErr OutlineMetrics(int16_t byte_count, Ptr text, Point numer, Point denom, int16_t* y_max, int16_t* y_min, Fixed* aw_array, Fixed* lsb_array, Rect* bounds_array)
```

Trap: — (executor 实现，无 trap) executor=C_

### RealFont  

```c
Boolean RealFont(INTEGER fnum, INTEGER sz)
```

Trap: `0xA902` executor=C_

### SetFScaleDisable  

```c
void SetFScaleDisable(Boolean disable)
```

Trap: `0xA834` executor=C_

### SetFontLock  

```c
void SetFontLock(Boolean lflag)
```

Trap: `0xA903` executor=C_

### SetFractEnable  

```c
void SetFractEnable(Boolean enable)
```

Trap: `0xA814` executor=C_

### SetOutlinePreferred  

```c
void SetOutlinePreferred(Boolean _outline_perferred_p)
```

Trap: — (executor 实现，无 trap) executor=C_

### SetPreserveGlyph  

```c
void SetPreserveGlyph(Boolean preserve_glyph)
```

Trap: — (executor 实现，无 trap) executor=C_

## Typedefs

- **FMOutPtr** = FMOutput*
- **FamRecPtr** = FamRec*
- **FamRecHandle** = FamRecPtr*
- **WidthTablePtr** = WidthTable*
- **WidthTableHandle** = WidthTablePtr*

## Enums

- **?**
- **?**
- **?**
- **?**
- **?**
- **?**

### Enum Values

**anonymous**:

- `systemFont` = 0
- `applFont` = 1

**anonymous**:

- `kFontIDNewYork` = 2
- `kFontIDGeneva` = 3
- `kFontIDMonaco` = 4
- `kFontIDVenice` = 5
- `kFontIDLondon` = 6
- `kFontIDAthens` = 7
- `kFontIDSanFrancisco` = 8
- `kFontIDToronto` = 9
- `kFontIDCairo` = 11
- `kFontIDLosAngeles` = 12
- `kFontIDTimes` = 20
- `kFontIDHelvetica` = 21
- `kFontIDCourier` = 22
- `kFontIDSymbol` = 23
- `kFontIDTaliesin` = 24

**anonymous**:

- `commandMark` = 17
- `checkMark` = 18
- `diamondMark` = 19
- `appleMark` = 20

**anonymous**:

- `propFont` = 36864
- `prpFntH` = 36865
- `prpFntW` = 36866
- `prpFntHW` = 36867

**anonymous**:

- `fixedFont` = 45056
- `fxdFntH` = 45057
- `fxdFntW` = 45058
- `fxdFntHW` = 45059

**anonymous**:

- `fontWid` = 44208

## Structs

- **FMetricRec** { ascent: Fixed, descent: Fixed, leading: Fixed, widMax: Fixed, wTabHandle: Handle }
- **FamRec** { ffFlags: INTEGER, ffFamID: INTEGER, ffFirstChar: INTEGER, ffLastChar: INTEGER, ffAscent: INTEGER, ffDescent: INTEGER, ffLeading: INTEGER, ffWidMax: INTEGER, ffWTabOff: LONGINT, ffKernOff: LONGINT, ffStylOff: LONGINT, ffProperty: INTEGER[9], ffIntl: INTEGER[2], ffVersion: INTEGER } — FontAssoc ffAssoc; WidTable ffWidthTab; StyleTable ffStyTab; KernTable ffKernTab;
- **WidthTable** { tabData: Fixed[256], tabFont: Handle, sExtra: LONGINT, style: LONGINT, fID: INTEGER, fSize: INTEGER, face: INTEGER, device: INTEGER, inNumer: Point, inDenom: Point, aFID: INTEGER, fHand: Handle, usedFam: Boolean, aFace: Byte, vOutput: INTEGER, hOutput: INTEGER, vFactor: INTEGER, hFactor: INTEGER, aSize: INTEGER, tabSize: INTEGER }
- **FMInput** { family: INTEGER, size: INTEGER, face: Style, needBits: Boolean, device: INTEGER, numer: Point, denom: Point }
- **FMOutput** { errNum: INTEGER, fontHandle: Handle, bold: Byte, italic: Byte, ulOffset: Byte, ulShadow: Byte, ulThick: Byte, shadow: Byte, extra: SignedByte, ascent: Byte, descent: Byte, widMax: Byte, leading: SignedByte, unused: Byte, numer: Point, denom: Point }
- **FontRec** { fontType: INTEGER, firstChar: INTEGER, lastChar: INTEGER, widMax: INTEGER, kernMax: INTEGER, nDescent: INTEGER, fRectWidth: INTEGER, fRectHeight: INTEGER, owTLoc: INTEGER, ascent: INTEGER, descent: INTEGER, leading: INTEGER, rowWords: INTEGER } — more stuff is usually appended here ... bitImage, locTable, owTable

## Dispatchers

- **FontDispatch**—

## Low Memory Globals

- **JSwapFont** @ 0x8E0 (ProcPtr) — FontMgr Private.a (true-b);
- **WidthListHand** @ 0x8E4 (Handle) — FontMgr IMIV-42 (true);
- **ROMFont0** @ 0x980 (Handle) — FontMgr IMI-233 (true);
- **ApFontID** @ 0x984 (INTEGER) — FontMgr IMIV-31 (true);
- **ROMlib_myfmi** @ 0x988 (FMInput) — FontMgr ToolEqu.a (true);
- **ROMlib_fmo** @ 0x998 (FMOutput) — FontMgr Private.a (true);
- **FScaleDisable** @ 0xA63 (Byte) — FontMgr IMI-222 (true);
- **WidthPtr** @ 0xB10 (WidthTablePtr) — FontMgr IMIV-42 (true);
- **WidthTabHandle** @ 0xB2A (WidthTableHandle) — FontMgr IMIV-42 (true);
- **IntlSpec** @ 0xBA0 (LONGINT) — FontMgr IMIV-42 (true);
- **SysFontFam** @ 0xBA6 (INTEGER) — FontMgr IMIV-31 (true);
- **SysFontSiz** @ 0xBA8 (INTEGER) — FontMgr IMIV-31 (true);
- **LastFOND** @ 0xBC2 (FamRecHandle) — FontMgr IMIV-36 (true);
- **fondid** @ 0xBC6 (INTEGER) — FontMgr ToolEqu.a (true-b);
- **FractEnable** @ 0xBF4 (Byte) — FontMgr IMIV-32 (true);
- **SynListHandle** @ 0xD32 (Handle) — FontMgr IMV-182 (false);
