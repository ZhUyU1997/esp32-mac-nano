# CQuickDraw Interfaces

QDExtensions trap

Source: `multiversal/defs/CQuickDraw.yaml`

- Functions: **138**
- Typedefs: **24**
- Structs: **14**, Unions: **0**
- Enums: **10**
- Function pointers: **1**
- Common blocks: **0**
- Dispatchers: **4**
- Low-memory globals: **4**

## Functions

### ActivatePalette  

```c
void ActivatePalette(WindowPtr ?)
```

Trap: `0xAA94` executor=C_

### AddComp  

```c
void AddComp(ProcPtr ?)
```

Trap: `0xAA3B` executor=C_

### AddSearch  

```c
void AddSearch(ProcPtr ?)
```

Trap: `0xAA3A` executor=C_

### AllocCursor  

```c
void AllocCursor()
```

Trap: `0xAA1D` executor=C_

### AllowPurgePixels  

```c
void AllowPurgePixels(PixMapHandle ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### AnimateEntry  

```c
void AnimateEntry(WindowPtr ?, INTEGER ?, RGBColor* ?)
```

Trap: `0xAA99` executor=C_

### AnimatePalette  

```c
void AnimatePalette(WindowPtr ?, CTabHandle ?, INTEGER ?, INTEGER ?, INTEGER ?)
```

Trap: `0xAA9A` executor=C_

### BackPixPat  

```c
void BackPixPat(PixPatHandle ?)
```

Trap: `0xAA0B` executor=C_

### BitMapToRegion  

```c
OSErr BitMapToRegion(RgnHandle ?, const BitMap* ?)
```

Trap: `0xA8D7` executor=C_

### CMY2RGB  

```c
void CMY2RGB(CMYColor* ?, RGBColor* ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### CTab2Palette  

```c
void CTab2Palette(CTabHandle ?, PaletteHandle ?, INTEGER ?, INTEGER ?)
```

Trap: `0xAA9F` executor=C_

### CTabChanged  

```c
void CTabChanged(CTabHandle ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### CloseCPort  

```c
void CloseCPort(CGrafPtr ?)
```

Trap: `0xAA02` executor=C_

### Color2Index  

```c
LONGINT Color2Index(RGBColor* ?)
```

Trap: `0xAA33` executor=C_

### CopyPalette  

```c
void CopyPalette(PaletteHandle src_palette, PaletteHandle dst_palette, int16_t src_start, int16_t dst_start, int16_t n_entries)
```

Trap: `0xAAA1` executor=C_

### CopyPixMap  

```c
void CopyPixMap(PixMapHandle ?, PixMapHandle ?)
```

Trap: `0xAA05` executor=C_

### CopyPixPat  

```c
void CopyPixPat(PixPatHandle ?, PixPatHandle ?)
```

Trap: `0xAA09` executor=C_

### DelComp  

```c
void DelComp(ProcPtr ?)
```

Trap: `0xAA4D` executor=C_

### DelSearch  

```c
void DelSearch(ProcPtr ?)
```

Trap: `0xAA4C` executor=C_

### DeviceLoop  

```c
void DeviceLoop(RgnHandle ?, DeviceLoopDrawingUPP ?, LONGINT ?, DeviceLoopFlags ?)
```

Trap: `0xABCA` executor=C_

### DisposeCCursor  

```c
void DisposeCCursor(CCrsrHandle ?)
```

Trap: `0xAA26` executor=C_

### DisposeCTable  

```c
void DisposeCTable(CTabHandle ?)
```

Trap: `0xAA24` executor=C_

### DisposeGDevice  

```c
void DisposeGDevice(GDHandle ?)
```

Trap: `0xAA30` executor=C_

### DisposeGWorld  

```c
void DisposeGWorld(GWorldPtr ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### DisposePalette  

```c
void DisposePalette(PaletteHandle ?)
```

Trap: `0xAA93` executor=C_

### DisposePictInfo  

```c
OSErr DisposePictInfo(PictInfoID ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### DisposePixMap  

```c
void DisposePixMap(PixMapHandle ?)
```

Trap: `0xAA04` executor=C_

### DisposePixPat  

```c
void DisposePixPat(PixPatHandle ?)
```

Trap: `0xAA08` executor=C_

### DisposeScreenBuffer  

```c
void DisposeScreenBuffer(PixMapHandle ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### Entry2Index  

```c
LONGINT Entry2Index(INTEGER ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### FillCArc  

```c
void FillCArc(const Rect* ?, int16_t ?, int16_t ?, PixPatHandle ?)
```

Trap: `0xAA11` executor=C_

### FillCOval  

```c
void FillCOval(const Rect* ?, PixPatHandle ?)
```

Trap: `0xAA0F` executor=C_

### FillCPoly  

```c
void FillCPoly(PolyHandle ?, PixPatHandle ?)
```

Trap: `0xAA13` executor=C_

### FillCRect  

```c
void FillCRect(const Rect* ?, PixPatHandle ?)
```

Trap: `0xAA0E` executor=C_

### FillCRgn  

```c
void FillCRgn(RgnHandle ?, PixPatHandle ?)
```

Trap: `0xAA12` executor=C_

### FillCRoundRect  

```c
void FillCRoundRect(const Rect* ?, int16_t ?, int16_t ?, PixPatHandle ?)
```

Trap: `0xAA10` executor=C_

### Fix2SmallFract  

```c
SmallFract Fix2SmallFract(Fixed ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### GDeviceChanged  

```c
void GDeviceChanged(GDHandle ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetBackColor  

```c
void GetBackColor(RGBColor* ?)
```

Trap: `0xAA1A` executor=C_

### GetCCursor  

```c
CCrsrHandle GetCCursor(INTEGER ?)
```

Trap: `0xAA1B` executor=C_

### GetCTSeed  

```c
LONGINT GetCTSeed()
```

Trap: `0xAA28` executor=C_

### GetCTable  

```c
CTabHandle GetCTable(INTEGER ?)
```

Trap: `0xAA18` executor=C_

### GetCWMgrPort  

```c
void GetCWMgrPort(CGrafPtr* ?)
```

Trap: `0xAA48` executor=C_

### GetColor  

```c
Boolean GetColor(Point ?, Str255 ?, RGBColor* ?, RGBColor* ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetDeviceList  

```c
GDHandle GetDeviceList()
```

Trap: `0xAA29` executor=C_

### GetEntryColor  

```c
void GetEntryColor(PaletteHandle ?, INTEGER ?, RGBColor* ?)
```

Trap: `0xAA9B` executor=C_

### GetEntryUsage  

```c
void GetEntryUsage(PaletteHandle ?, INTEGER ?, INTEGER* ?, INTEGER* ?)
```

Trap: `0xAA9D` executor=C_

### GetForeColor  

```c
void GetForeColor(RGBColor* ?)
```

Trap: `0xAA19` executor=C_

### GetGDevice  

```c
GDHandle GetGDevice()
```

Trap: `0xAA32` executor=C_

### GetGWorld  

```c
void GetGWorld(CGrafPtr* ?, GDHandle* ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetGWorldDevice  

```c
GDHandle GetGWorldDevice(GWorldPtr ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetGWorldPixMap  

```c
PixMapHandle GetGWorldPixMap(GWorldPtr ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetGray  

```c
Boolean GetGray(GDHandle ?, RGBColor* ?, RGBColor* ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetMainDevice  

```c
GDHandle GetMainDevice()
```

Trap: `0xAA2A` executor=C_

### GetMaxDevice  

```c
GDHandle GetMaxDevice(const Rect* ?)
```

Trap: `0xAA27` executor=C_

### GetNewPalette  

```c
PaletteHandle GetNewPalette(INTEGER ?)
```

Trap: `0xAA92` executor=C_

### GetNextDevice  

```c
GDHandle GetNextDevice(GDHandle ?)
```

Trap: `0xAA2B` executor=C_

### GetPalette  

```c
PaletteHandle GetPalette(WindowPtr ?)
```

Trap: `0xAA96` executor=C_

### GetPaletteUpdates  

```c
INTEGER GetPaletteUpdates(PaletteHandle ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetPictInfo  

```c
OSErr GetPictInfo(PicHandle ?, PictInfo* ?, int16_t ?, int16_t ?, int16_t ?, int16_t ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetPixBaseAddr  

```c
Ptr GetPixBaseAddr(PixMapHandle ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetPixMapInfo  

```c
OSErr GetPixMapInfo(PixMapHandle ?, PictInfo* ?, int16_t ?, int16_t ?, int16_t ?, int16_t ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetPixPat  

```c
PixPatHandle GetPixPat(INTEGER ?)
```

Trap: `0xAA0C` executor=C_

### GetPixelsState  

```c
GWorldFlags GetPixelsState(PixMapHandle ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetSubTable  

```c
void GetSubTable(CTabHandle ?, INTEGER ?, CTabHandle ?)
```

Trap: `0xAA37` executor=C_

### HSL2RGB  

```c
void HSL2RGB(HSLColor* ?, RGBColor* ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### HSV2RGB  

```c
void HSV2RGB(HSVColor* ?, RGBColor* ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### HasDepth  

```c
INTEGER HasDepth(GDHandle ?, INTEGER ?, INTEGER ?, INTEGER ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### HiliteColor  

```c
void HiliteColor(RGBColor* ?)
```

Trap: `0xAA22` executor=C_

### Index2Color  

```c
void Index2Color(LONGINT ?, RGBColor* ?)
```

Trap: `0xAA34` executor=C_

### InitCPort  

```c
void InitCPort(CGrafPtr ?)
```

Trap: `0xAA01` executor=C_

### InitGDevice  

```c
void InitGDevice(INTEGER ?, LONGINT ?, GDHandle ?)
```

Trap: `0xAA2E` executor=C_

### InitPalettes  

```c
void InitPalettes()
```

Trap: `0xAA90` executor=C_

### InvertColor  

```c
void InvertColor(RGBColor* ?)
```

Trap: `0xAA35` executor=C_

### LockPixels  

```c
Boolean LockPixels(PixMapHandle ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### MakeITable  

```c
void MakeITable(CTabHandle ?, ITabHandle ?, INTEGER ?)
```

Trap: `0xAA39` executor=C_

### NSetPalette  

```c
void NSetPalette(WindowPtr ?, PaletteHandle ?, INTEGER updates)
```

Trap: `0xAA95` executor=C_

### NewGDevice  

```c
GDHandle NewGDevice(INTEGER ?, LONGINT ?)
```

Trap: `0xAA2F` executor=C_

### NewGWorld  

```c
QDErr NewGWorld(GWorldPtr* ?, INTEGER ?, const Rect* ?, CTabHandle ?, GDHandle ?, GWorldFlags ?)
```

Trap: — (executor 实现，无 trap) executor=C_ — QDExtensions trap

### NewPalette  

```c
PaletteHandle NewPalette(INTEGER ?, CTabHandle ?, INTEGER ?, INTEGER ?)
```

Trap: `0xAA91` executor=C_

### NewPictInfo  

```c
OSErr NewPictInfo(PictInfoID* ?, int16_t ?, int16_t ?, int16_t ?, int16_t ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### NewPixMap  

```c
PixMapHandle NewPixMap()
```

Trap: `0xAA03` executor=C_

### NewPixPat  

```c
PixPatHandle NewPixPat()
```

Trap: `0xAA07` executor=C_

### NewScreenBuffer  

```c
QDErr NewScreenBuffer(const Rect* ?, Boolean ?, GDHandle* ?, PixMapHandle* ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### NewTempScreenBuffer  

```c
QDErr NewTempScreenBuffer(const Rect* ?, Boolean ?, GDHandle* ?, PixMapHandle* ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### NoPurgePixels  

```c
void NoPurgePixels(PixMapHandle ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### OffscreenVersion  

```c
LONGINT OffscreenVersion()
```

Trap: — (executor 实现，无 trap) executor=C_

### OpColor  

```c
void OpColor(RGBColor* ?)
```

Trap: `0xAA21` executor=C_

### OpenCPicture  

```c
PicHandle OpenCPicture(OpenCPicParams* newheaderp)
```

Trap: `0xAA20` executor=C_

### OpenCPort  

```c
void OpenCPort(CGrafPtr ?)
```

Trap: `0xAA00` executor=C_

### PMgrVersion  

```c
INTEGER PMgrVersion()
```

Trap: — (executor 实现，无 trap) executor=C_

### Palette2CTab  

```c
void Palette2CTab(PaletteHandle ?, CTabHandle ?)
```

Trap: `0xAAA0` executor=C_

### PenPixPat  

```c
void PenPixPat(PixPatHandle ?)
```

Trap: `0xAA0A` executor=C_

### PixMap32Bit  

```c
Boolean PixMap32Bit(PixMapHandle ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### PixPatChanged  

```c
void PixPatChanged(PixPatHandle ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### PmBackColor  

```c
void PmBackColor(INTEGER ?)
```

Trap: `0xAA98` executor=C_

### PmForeColor  

```c
void PmForeColor(INTEGER ?)
```

Trap: `0xAA97` executor=C_

### PortChanged  

```c
void PortChanged(GrafPtr ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### ProtectEntry  

```c
void ProtectEntry(INTEGER ?, Boolean ?)
```

Trap: `0xAA3D` executor=C_

### QDDone  

```c
Boolean QDDone(GrafPtr ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### QDError  

```c
INTEGER QDError()
```

Trap: `0xAA40` executor=C_

### RGB2CMY  

```c
void RGB2CMY(RGBColor* ?, CMYColor* ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### RGB2HSL  

```c
void RGB2HSL(RGBColor* ?, HSLColor* ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### RGB2HSV  

```c
void RGB2HSV(RGBColor* ?, HSVColor* ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### RGBBackColor  

```c
void RGBBackColor(RGBColor* ?)
```

Trap: `0xAA15` executor=C_

### RGBForeColor  

```c
void RGBForeColor(RGBColor* ?)
```

Trap: `0xAA14` executor=C_

### RealColor  

```c
Boolean RealColor(RGBColor* ?)
```

Trap: `0xAA36` executor=C_

### RecordPictInfo  

```c
OSErr RecordPictInfo(PictInfoID ?, PicHandle ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### RecordPixMapInfo  

```c
OSErr RecordPixMapInfo(PictInfoID ?, PixMapHandle ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### ReserveEntry  

```c
void ReserveEntry(INTEGER ?, Boolean ?)
```

Trap: `0xAA3E` executor=C_

### ResizePalette  

```c
void ResizePalette(PaletteHandle ?, INTEGER ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### RestoreBack  

```c
void RestoreBack(ColorSpec* ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### RestoreDeviceClut  

```c
void RestoreDeviceClut(GDHandle ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### RestoreEntries  

```c
void RestoreEntries(CTabHandle ?, CTabHandle ?, ReqListRec* ?)
```

Trap: `0xAA4A` executor=C_

### RestoreFore  

```c
void RestoreFore(ColorSpec* ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### RetrievePictInfo  

```c
OSErr RetrievePictInfo(PictInfoID ?, PictInfo* ?, int16_t ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### SaveBack  

```c
void SaveBack(ColorSpec* ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### SaveEntries  

```c
void SaveEntries(CTabHandle ?, CTabHandle ?, ReqListRec* ?)
```

Trap: `0xAA49` executor=C_

### SaveFore  

```c
void SaveFore(ColorSpec* ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### ScreenRes  

```c
void ScreenRes(INTEGER* ?, INTEGER* ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### SetCCursor  

```c
void SetCCursor(CCrsrHandle ?)
```

Trap: `0xAA1C` executor=C_

### SetClientID  

```c
void SetClientID(INTEGER ?)
```

Trap: `0xAA3C` executor=C_

### SetDepth  

```c
OSErr SetDepth(GDHandle ?, INTEGER ?, INTEGER ?, INTEGER ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### SetDeviceAttribute  

```c
void SetDeviceAttribute(GDHandle ?, INTEGER ?, Boolean ?)
```

Trap: `0xAA2D` executor=C_

### SetEntries  

```c
void SetEntries(INTEGER ?, INTEGER ?, ColorSpec* cSpecArray)
```

Trap: `0xAA3F` executor=C_

### SetEntryColor  

```c
void SetEntryColor(PaletteHandle ?, INTEGER ?, RGBColor* ?)
```

Trap: `0xAA9C` executor=C_

### SetEntryUsage  

```c
void SetEntryUsage(PaletteHandle ?, INTEGER ?, INTEGER ?, INTEGER ?)
```

Trap: `0xAA9E` executor=C_

### SetGDevice  

```c
void SetGDevice(GDHandle ?)
```

Trap: `0xAA31` executor=C_

### SetGWorld  

```c
void SetGWorld(CGrafPtr ?, GDHandle ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### SetPalette  

```c
void SetPalette(WindowPtr ?, PaletteHandle ?, Boolean ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### SetPaletteUpdates  

```c
void SetPaletteUpdates(PaletteHandle ?, INTEGER ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### SetPixelsState  

```c
void SetPixelsState(PixMapHandle ?, GWorldFlags ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### SetPortPix  

```c
void SetPortPix(PixMapHandle ?)
```

Trap: `0xAA06` executor=C_

### SetStdCProcs  

```c
void SetStdCProcs(CQDProcs* cProcs)
```

Trap: `0xAA4E` executor=C_

### SmallFract2Fix  

```c
Fixed SmallFract2Fix(SmallFract ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### TestDeviceAttribute  

```c
Boolean TestDeviceAttribute(GDHandle ?, INTEGER ?)
```

Trap: `0xAA2C` executor=C_

### UnlockPixels  

```c
void UnlockPixels(PixMapHandle ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### UpdateGWorld  

```c
GWorldFlags UpdateGWorld(GWorldPtr* ?, INTEGER ?, const Rect* ?, CTabHandle ?, GDHandle ?, GWorldFlags ?)
```

Trap: — (executor 实现，无 trap) executor=C_

## Typedefs

- **ITabPtr** = ITab* — can't use [0]; make this an unsigned char even tho the mac has SignedByte; it is treated as unsigned
- **ITabHandle** = ITabPtr*
- **GDPtr** = GDevice*
- **GDevicePtr** = GDPtr
- **GDHandle** = GDevicePtr*
- **SProcPtr** = SProcRec*
- **SProcHndl** = SProcPtr*
- **CProcPtr** = CProcRec*
- **CProcHndl** = CProcPtr*
- **DeviceLoopFlags** = uint32_t
- **PalettePtr** = Palette*
- **PaletteHandle** = PalettePtr*
- **GWorldFlags** = LONGINT
- **GWorld** = CGrafPort
- **GWorldPtr** = CGrafPort*
- **CommentSpec** = CommonSpec
- **CommentSpecPtr** = CommentSpec*
- **CommentSpecHandle** = CommentSpecPtr*
- **FontSpecPtr** = FontSpec*
- **FontSpecHandle** = FontSpecPtr*
- **PictInfoPtr** = PictInfo*
- **PictInfoHandle** = PictInfoPtr*
- **PictInfoID** = int32_t
- **QDErr** = int16_t

## Enums

- **?**
- **?** — DeviceLoop flags.
- **?**
- **pmColorUsage**
- **pmUpdates**
- **?**
- **?** — a pixmap pixelType of `native_rgb_pixel_type' means that the format of the pixmap is the same as that of the screen
- **?**
- **?** — error codes returned by QDError
- **?** — TODO:  FIXME -- #warning find out correct value for colReqErr -158 is just a guess

### Enum Values

**anonymous**:

- `minSeed` = 1024

**anonymous** — DeviceLoop flags.:

- `singleDevices` = 1 << 0
- `dontMatchSeeds` = 1 << 1
- `allDevices` = 1 << 2

**anonymous**:

- `CI_USAGE_TYPE_BITS` = 15

**pmColorUsage**:

- `pmCourteous` = 0
- `pmDithered` = 1
- `pmTolerant` = 2
- `pmAnimated` = 4
- `pmExplicit` = 8
- `pmInhibitG2` = 256
- `pmInhibitC2` = 512
- `pmInhibitG4` = 1024
- `pmInhibitC4` = 2048
- `pmInhibitG8` = 4096
- `pmInhibitC8` = 8192

**pmUpdates**:

- `pmNoUpdates` = 32768
- `pmBkUpdates` = 40960
- `pmFgUpdates` = 49152
- `pmAllUpdates` = 57344

**anonymous**:

- `RGBDirect` = 16
- `Indirect` = 0

**anonymous** — a pixmap pixelType of `native_rgb_pixel_type' means that the format of the pixmap is the same as that of the screen:

- `vdriver_rgb_pixel_type` = 185

**anonymous**:

- `pixPurge` = 1 << 0
- `noNewDevice` = 1 << 1
- `useTempMem` = 1 << 2
- `keepLocal` = 1 << 3
- `pixelsPurgeable` = 1 << 6
- `pixelsLocked` = 1 << 7
- `mapPix` = 1 << 16
- `newDepth` = 1 << 17
- `alignPix` = 1 << 18
- `newRowBytes` = 1 << 19
- `reallocPix` = 1 << 20
- `clipPix` = 1 << 28
- `stretchPix` = 1 << 29
- `ditherPix` = 1 << 30
- `gwFlagErr` = 1 << 31

**anonymous** — error codes returned by QDError:

- `noMemForPictPlaybackErr` = -145
- `regionTooBigErr` = -147
- `pixmapTooDeepErr` = -148
- `nsStackErr` = -149
- `cMatchErr` = -150
- `cTempMemErr` = -151
- `cNoMemErr` = -152
- `cRangeErr` = -153
- `cProtectErr` = -154
- `cDevErr` = -155
- `cResErr` = -156
- `cDepthErr` = -157
- `rgnTooBigErr` = -500

**anonymous** — TODO:  FIXME -- #warning find out correct value for colReqErr -158 is just a guess:

- `colReqErr` = -158

## Structs

- **ITab** { iTabSeed: LONGINT, iTabRes: INTEGER, iTTable: uint8_t[1] }
- **GDevice** {  }
- **SProcRec** {  }
- **SProcRec** { nxtSrch: SProcHndl, srchProc: ProcPtr }
- **CProcRec** {  }
- **CProcRec** { nxtComp: CProcHndl, compProc: ProcPtr }
- **GDevice** { gdRefNum: INTEGER, gdID: INTEGER, gdType: INTEGER, gdITable: ITabHandle, gdResPref: INTEGER, gdSearchProc: SProcHndl, gdCompProc: CProcHndl, gdFlags: INTEGER, gdPMap: PixMapHandle, gdRefCon: LONGINT, gdNextGD: GDHandle, gdRect: Rect, gdMode: LONGINT, gdCCBytes: INTEGER, gdCCDepth: INTEGER, gdCCXData: Handle, gdCCXMask: Handle, gdReserved: LONGINT }
- **ColorInfo** { ciRGB: RGBColor, ciUsage: INTEGER, ciTolerance: INTEGER, ciFlags: INTEGER, ciPrivate: LONGINT }
- **Palette** { pmEntries: INTEGER, pmWindow: GrafPtr, pmPrivate: INTEGER, pmDevices: LONGINT, pmSeeds: Handle, pmInfo: ColorInfo[1] }
- **ReqListRec** { reqLSize: INTEGER, reqLData: INTEGER[1] }
- **OpenCPicParams** { srcRect: Rect, hRes: Fixed, vRes: Fixed, version: int16_t, reserved1: int16_t, reserved2: int32_t } — extended version 2 picture datastructures
- **CommonSpec** { count: int16_t, ID: int16_t }
- **FontSpec** { pictFontID: int16_t, sysFontID: int16_t, size: int32_t[4], style: int16_t, nameOffset: int32_t }
- **PictInfo** { version: int16_t, uniqueColors: int32_t, thePalette: PaletteHandle, theColorTable: CTabHandle, hRes: Fixed, vRes: Fixed, depth: INTEGER, sourceRect: Rect, textCount: int32_t, lineCount: int32_t, rectCount: int32_t, rRectCount: int32_t, ovalCount: int32_t, arcCount: int32_t, polyCount: int32_t, regionCount: int32_t, bitMapCount: int32_t, pixMapCount: int32_t, commentCount: int32_t, uniqueComments: int32_t, commentHandle: CommentSpecHandle, uniqueFonts: int32_t, fontHandle: FontSpecHandle, fontNamesHandle: Handle, reserved1: int32_t, reserved2: int32_t }

## Function Pointers

- **DeviceLoopDrawingUPP** (depth: INTEGER, deviceFlags: INTEGER, targetDevice: GDHandle, userData: LONGINT) -> void

## Dispatchers

- **PaletteDispatch**— D0W? D0<0xFF> ### — D0W? D0<0xFF> ###
- **Pack12**—
- **QDExtensions**—
- **Pack15**— D0<0xFF>? ### — D0<0xFF>? ###

## Low Memory Globals

- **TheGDevice** @ 0xCC8 (GDHandle) — QuickDraw IMV (true);
- **MainDevice** @ 0x8A4 (GDHandle) — QuickDraw IMV (true);
- **DeviceList** @ 0x8A8 (GDHandle) — QuickDraw IMV (true);
- **HiliteRGB** @ 0xDA0 (RGBColor) — QuickDraw IMV-62 (true);
