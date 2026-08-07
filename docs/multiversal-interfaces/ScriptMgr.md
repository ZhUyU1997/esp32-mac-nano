# ScriptMgr Interfaces

the above four functions are actually one entry point on 68K:

Source: `multiversal/defs/ScriptMgr.yaml`

- Functions: **48**
- Typedefs: **14**
- Structs: **6**, Unions: **0**
- Enums: **14**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **1**
- Low-memory globals: **1**

## Functions

### Char2Pixel  

```c
INTEGER Char2Pixel(Ptr textbufp, INTEGER len, INTEGER slop, INTEGER offset, SignedByte dir)
```

Trap: — (executor 实现，无 trap) executor=C_

### CharByte  

```c
INTEGER CharByte(Ptr textBuf, INTEGER textOffset)
```

Trap: — (executor 实现，无 trap) executor=C_

### CharToPixel  

```c
INTEGER CharToPixel(Ptr textBuf, LONGINT textLen, Fixed slop, LONGINT offset, INTEGER direction, JustStyleCode styleRunPosition, Point numer, Point denom)
```

Trap: — (executor 实现，无 trap) executor=C_

### CharType  

```c
INTEGER CharType(Ptr textbufp, INTEGER offset)
```

Trap: — (executor 实现，无 trap) executor=C_

### CharacterByteType  

```c
INTEGER CharacterByteType(Ptr textBuf, INTEGER textOffset, ScriptCode script)
```

Trap: — (executor 实现，无 trap) executor=C_

### CharacterType  

```c
INTEGER CharacterType(Ptr textbufp, INTEGER offset, ScriptCode script)
```

Trap: — (executor 实现，无 trap) executor=C_

### DrawJust  

```c
void DrawJust(Ptr textbufp, INTEGER length, INTEGER slop)
```

Trap: — (executor 实现，无 trap) executor=C_

### DrawJustified  

```c
void DrawJustified(Ptr textPtr, LONGINT textLength, Fixed slop, JustStyleCode styleRunPosition, Point numer, Point denom)
```

Trap: — (executor 实现，无 trap) executor=C_

### ExtendedToString  

```c
FormatStatus ExtendedToString(Extended80* xp, NumFormatStringRec* formatp, NumberParts* partsp, Str255 string)
```

Trap: — (executor 实现，无 trap) executor=C_

### FillParseTable  

```c
Boolean FillParseTable(CharByteTable table, ScriptCode script)
```

Trap: — (executor 实现，无 trap) executor=C_

### FindScriptRun  

```c
ScriptRunStatus FindScriptRun(Ptr textPtr, LONGINT textLen, LONGINT* lenUsedp)
```

Trap: — (executor 实现，无 trap) executor=C_

### FindWord  

```c
void FindWord(Ptr textbufp, INTEGER length, INTEGER offset, Boolean leftside, Ptr breaks, INTEGER* offsets)
```

Trap: — (executor 实现，无 trap) executor=C_

### FontScript  

```c
INTEGER FontScript()
```

Trap: — (executor 实现，无 trap) executor=C_

### FontToScript  

```c
INTEGER FontToScript(INTEGER fontnum)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetScriptManagerVariable  

```c
LONGINT GetScriptManagerVariable(INTEGER verb)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetScriptVariable  

```c
LONGINT GetScriptVariable(INTEGER script, INTEGER verb)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetSysDirection  

```c
INTEGER GetSysDirection()
```

Trap: — (executor 实现，无 trap)

### HiliteText  

```c
void HiliteText(Ptr textbufp, INTEGER firstoffset, INTEGER secondoffset, INTEGER* offsets)
```

Trap: — (executor 实现，无 trap) executor=C_

### InitDateCache  

```c
OSErr InitDateCache(DateCachePtr theCache)
```

Trap: — (executor 实现，无 trap) executor=C_

### IntlScript  

```c
INTEGER IntlScript()
```

Trap: — (executor 实现，无 trap) executor=C_

### KeyScript  

```c
void KeyScript(INTEGER scriptcode)
```

Trap: — (executor 实现，无 trap) executor=C_

### LongDateToSeconds  

```c
void LongDateToSeconds(LongDateRec* ldatep, ULONGINT* secs_outp)
```

Trap: — (executor 实现，无 trap) executor=C_

### LongSecondsToDate  

```c
void LongSecondsToDate(ULONGINT* secs_inp, LongDateRec* ldatep)
```

Trap: — (executor 实现，无 trap) executor=C_

### LowercaseText  

```c
void LowercaseText(Ptr textp, INTEGER len, ScriptCode script)
```

Trap: — (executor 实现，无 trap) executor=C_

### MeasureJust  

```c
void MeasureJust(Ptr textbufp, INTEGER length, INTEGER slop, Ptr charlocs)
```

Trap: — (executor 实现，无 trap) executor=C_

### MeasureJustified  

```c
void MeasureJustified(Ptr text, int32_t length, Fixed slop, Ptr charLocs, JustStyleCode run_pos, Point numer, Point denom)
```

Trap: — (executor 实现，无 trap) executor=C_

### ParseTable  

```c
Boolean ParseTable(CharByteTable table)
```

Trap: — (executor 实现，无 trap) executor=C_

### Pixel2Char  

```c
INTEGER Pixel2Char(Ptr textbufp, INTEGER len, INTEGER slop, INTEGER pixwidth, Boolean* leftsidep)
```

Trap: — (executor 实现，无 trap) executor=C_

### PixelToChar  

```c
INTEGER PixelToChar(Ptr textBuf, LONGINT textLen, Fixed slop, Fixed pixelWidth, Boolean* leadingEdgep, Fixed* widthRemainingp, JustStyleCode styleRunPosition, Point numer, Point denom)
```

Trap: — (executor 实现，无 trap) executor=C_

### PortionLine  

```c
Fixed PortionLine(Ptr textPtr, LONGINT textLen, JustStyleCode styleRunPosition, Point numer, Point denom)
```

Trap: — (executor 实现，无 trap) executor=C_

### ReplaceText  

```c
INTEGER ReplaceText(Handle base_text, Handle subst_text, Str15 key)
```

Trap: — (executor 实现，无 trap) executor=C_

### SetScriptManagerVariable  

```c
OSErr SetScriptManagerVariable(INTEGER verb, LONGINT param)
```

Trap: — (executor 实现，无 trap) executor=C_

### SetScriptVariable  

```c
OSErr SetScriptVariable(INTEGER script, INTEGER verb, LONGINT param)
```

Trap: — (executor 实现，无 trap) executor=C_

### SetSysDirection  

```c
void SetSysDirection(INTEGER just)
```

Trap: — (executor 实现，无 trap)

### StringToDate  

```c
String2DateStatus StringToDate(Ptr text, int32_t length, DateCachePtr cache, int32_t* length_used_ret, LongDatePtr date_time)
```

Trap: — (executor 实现，无 trap) executor=C_

### StringToExtended  

```c
FormatStatus StringToExtended(Str255 string, NumFormatStringRec* formatp, NumberParts* partsp, Extended80* xp)
```

Trap: — (executor 实现，无 trap) executor=C_

### StringToFormatRec  

```c
FormatStatus StringToFormatRec(ConstStringPtr in_string, NumberParts* partsp, NumFormatStringRec* out_string)
```

Trap: — (executor 实现，无 trap) executor=C_

### StringToTime  

```c
String2DateStatus StringToTime(Ptr textp, LONGINT len, Ptr cachep, LONGINT* lenusedp, Ptr* datetimep)
```

Trap: — (executor 实现，无 trap) executor=C_

### StripDiacritics  

```c
void StripDiacritics(Ptr textp, INTEGER len, ScriptCode script)
```

Trap: — (executor 实现，无 trap) executor=C_

### StyledLineBreak  

```c
StyledLineBreakCode StyledLineBreak(Ptr textp, int32_t length, int32_t text_start, int32_t text_end, int32_t flags, Fixed* text_width_fp, int32_t* text_offset)
```

Trap: — (executor 实现，无 trap) executor=C_

### TextUtilFunctions  

```c
void TextUtilFunctions(int16_t selector, Ptr textp, INTEGER len, ScriptCode script)
```

Trap: — (executor 实现，无 trap) executor=C_ — the above four functions are actually one entry point on 68K:

### ToggleDate  

```c
ToggleResults ToggleDate(LongDateTime* lsecsp, LongDateField field, DateDelta delta, INTEGER ch, TogglePB* paramsp)
```

Trap: — (executor 实现，无 trap) executor=C_

### Transliterate  

```c
INTEGER Transliterate(Handle srch, Handle dsth, INTEGER target, LONGINT srcmask)
```

Trap: — (executor 实现，无 trap) executor=C_

### TransliterateText  

```c
INTEGER TransliterateText(Handle srch, Handle dsth, INTEGER target, LONGINT srcmask, ScriptCode script)
```

Trap: — (executor 实现，无 trap) executor=C_

### TruncString  

```c
INTEGER TruncString(INTEGER width, Str255 string, TruncCode code)
```

Trap: — (executor 实现，无 trap) executor=C_

### UppercaseStripDiacritics  

```c
void UppercaseStripDiacritics(Ptr textp, INTEGER len, ScriptCode script)
```

Trap: — (executor 实现，无 trap) executor=C_

### UppercaseText  

```c
void UppercaseText(Ptr textp, INTEGER len, ScriptCode script)
```

Trap: — (executor 实现，无 trap) executor=C_

### VisibleLength  

```c
LONGINT VisibleLength(Ptr textp, LONGINT len)
```

Trap: — (executor 实现，无 trap) executor=C_

## Typedefs

- **String2DateStatus** = INTEGER
- **StyledLineBreakCode** = uint8_t
- **DateCachePtr** = DateCacheRec*
- **LongDatePtr** = LongDateRec*
- **TruncCode** = INTEGER
- **JustStyleCode** = int16_t
- **CharByteTable** = int8_t[256]
- **ScriptRunStatus** = int16_t — Not sure this is correct, since in IM ScriptRunStatus is a record with two Signed Bytes
- **FormatStatus** = INTEGER
- **WideChar** = uint16_t
- **Extended80** = extended80
- **ToggleResults** = int16_t
- **LongDateField** = uint8_t
- **DateDelta** = char

## Enums

- **?**
- **?**
- **?**
- **?**
- **?**
- **?**
- **?**
- **?**
- **?**
- **?**
- **?**
- **?**
- **?** — TruncText return codes
- **?**

### Enum Values

**anonymous**:

- `smRoman` = 0

**anonymous**:

- `smCharPunct` = 0
- `smCharAscii` = 1
- `smCharEuro` = 7

**anonymous**:

- `smPunctNormal` = 0
- `smPunctNumber` = 256
- `smPunctSymbol` = 512
- `smPunctBlank` = 768

**anonymous**:

- `smCharLeft` = 0
- `smCharRight` = 8192

**anonymous**:

- `smCharLower` = 0
- `smCharUpper` = 16384

**anonymous**:

- `smChar1byte` = 0
- `smChar2byte` = 32768

**anonymous**:

- `smTransAscii` = 0
- `smTransNative` = 1
- `smTransLower` = 16384
- `smTransUpper` = 32768
- `smMaskAscii` = 1
- `smMaskNative` = 2
- `smMaskAll` = -1

**anonymous**:

- `smScriptVersion` = 0
- `smScriptMunged` = 2
- `smScriptEnabled` = 4
- `smScriptRight` = 6
- `smScriptJust` = 8
- `smScriptRedraw` = 10
- `smScriptSysFond` = 12
- `smScriptAppFond` = 14
- `smScriptNumber` = 16
- `smScriptDate` = 18
- `smScriptSort` = 20
- `smScriptRsvd1` = 22
- `smScriptRsvd2` = 24
- `smScriptRsvd3` = 26
- `smScriptRsvd4` = 28
- `smScriptRsvd5` = 30
- `smScriptKeys` = 32
- `smScriptIcon` = 34
- `smScriptPrint` = 36
- `smScriptTrap` = 38
- `smScriptCreator` = 40
- `smScriptFile` = 42
- `smScriptName` = 44

**anonymous**:

- `smVersion` = 0
- `smMunged` = 2
- `smEnabled` = 4
- `smBiDirect` = 6
- `smFontForce` = 8
- `smIntlForce` = 10
- `smForced` = 12
- `smDefault` = 14
- `smPrint` = 16
- `smSysScript` = 18
- `smAppScript` = 20
- `smKeyScript` = 22
- `smSysRef` = 24
- `smKeyCache` = 26
- `smKeySwap` = 28

**anonymous**:

- `smKCHRCache` = 38

**anonymous**:

- `smVerbNotFound` = -1

**anonymous**:

- `smBreakWord` = 0
- `smBreakChar` = 1
- `smBreakOverflow` = 2

**anonymous** — TruncText return codes:

- `NotTruncated` = 0
- `Truncated` = 1
- `TruncErr` = -1

**anonymous**:

- `smSystemScript` = -1

## Structs

- **DateCacheRec** { hidden: int16_t[256] }
- **LongDateRec** { era: int16_t, year: int16_t, month: int16_t, day: int16_t, hour: int16_t, minute: int16_t, second: int16_t, dayOfWeek: int16_t, dayOfYear: int16_t, weekOfYear: int16_t, pm: int16_t, res1: int16_t, res2: int16_t, res3: int16_t }
- **NumFormatStringRec** { fLength: Byte, fVersion: Byte, data: SignedByte[254] }
- **WideCharArr** { size: INTEGER, data: WideChar[10] }
- **NumberParts** { version: INTEGER, data: WideChar[31], pePlus: WideCharArr, peMinus: WideCharArr, peMinusPlus: WideCharArr, altNumTable: WideCharArr, reserved: char[20] }
- **TogglePB** { togFlags: int32_t, amChars: ResType, pmChars: ResType, reserved: int32_t[4] }

## Dispatchers

- **ScriptUtil**—

## Low Memory Globals

- **TESysJust** @ 0xBAC (INTEGER) — ScriptMgr ToolEqu.a (true-b);
