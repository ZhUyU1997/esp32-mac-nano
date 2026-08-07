# QuickDraw Interfaces

can't use 0

Source: `multiversal/defs/QuickDraw.yaml`

- Functions: **166**
- Typedefs: **27**
- Structs: **23**, Unions: **0**
- Enums: **13**
- Function pointers: **13**
- Common blocks: **0**
- Dispatchers: **0**
- Low-memory globals: **29**

## Functions

### AddPt  

```c
void AddPt(Point src, Point* dst)
```

Trap: `0xA87E` executor=C_

### BackColor  

```c
void BackColor(LONGINT c)
```

Trap: `0xA863` executor=C_

### BackPat  

```c
void BackPat(const Pattern* pp)
```

Trap: `0xA87C` executor=C_

### CalcCMask  

```c
void CalcCMask(BitMap* srcbp, BitMap* dstbp, const Rect* srcrp, const Rect* dstrp, RGBColor* seedrgbp, ProcPtr matchprocp, LONGINT matchdata)
```

Trap: `0xAA4F` executor=C_

### CalcMask  

```c
void CalcMask(Ptr srcp, Ptr dstp, INTEGER srcr, INTEGER dstr, INTEGER height, INTEGER width)
```

Trap: `0xA838` executor=C_

### CharExtra  

```c
void CharExtra(Fixed Extra)
```

Trap: `0xAA23` executor=C_

### CharWidth  

```c
INTEGER CharWidth(CharParameter thec)
```

Trap: `0xA88D` executor=C_

### ClipRect  

```c
void ClipRect(Rect* r)
```

Trap: `0xA87B` executor=C_

### ClosePicture  

```c
void ClosePicture()
```

Trap: `0xA8F4` executor=C_

### ClosePoly  

```c
void ClosePoly()
```

Trap: `0xA8CC` executor=C_

### ClosePort  

```c
void ClosePort(GrafPtr p)
```

Trap: `0xA87D` executor=C_

### CloseRgn  

```c
void CloseRgn(RgnHandle rh)
```

Trap: `0xA8DB` executor=C_

### ColorBit  

```c
void ColorBit(INTEGER b)
```

Trap: `0xA864` executor=C_

### CopyBits  

```c
void CopyBits(BitMap* src_bitmap, BitMap* dst_bitmap, const Rect* src_rect, const Rect* dst_rect, INTEGER mode, RgnHandle mask)
```

Trap: `0xA8EC` executor=C_

### CopyDeepMask  

```c
void CopyDeepMask(BitMap* srcBits, BitMap* maskBits, BitMap* dstBits, const Rect* srcRect, const Rect* maskRect, const Rect* dstRect, INTEGER mode, RgnHandle maskRgn)
```

Trap: `0xAA51` executor=C_

### CopyMask  

```c
void CopyMask(BitMap* srcbp, BitMap* mskbp, BitMap* dstbp, const Rect* srcrp, const Rect* mskrp, const Rect* dstrp)
```

Trap: `0xA817` executor=C_

### CopyRgn  

```c
void CopyRgn(RgnHandle s, RgnHandle d)
```

Trap: `0xA8DC` executor=C_

### DiffRgn  

```c
void DiffRgn(RgnHandle s1, RgnHandle s2, RgnHandle dest)
```

Trap: `0xA8E6` executor=C_

### DisposeRgn  

```c
void DisposeRgn(RgnHandle rh)
```

Trap: `0xA8D9` executor=C_

### DrawChar  

```c
void DrawChar(CharParameter thec)
```

Trap: `0xA883` executor=C_

### DrawPicture  

```c
void DrawPicture(PicHandle pic, const Rect* destrp)
```

Trap: `0xA8F6` executor=C_

### DrawString  

```c
void DrawString(ConstStringPtr s)
```

Trap: `0xA884` executor=C_

### DrawText  

```c
void DrawText(Ptr tb, INTEGER fb, INTEGER bc)
```

Trap: `0xA885` executor=C_

### EmptyRect  

```c
Boolean EmptyRect(const Rect* r)
```

Trap: `0xA8AE` executor=C_

### EmptyRgn  

```c
Boolean EmptyRgn(RgnHandle rh)
```

Trap: `0xA8E2` executor=C_

### EqualPt  

```c
Boolean EqualPt(Point p1, Point p2)
```

Trap: `0xA881` executor=C_

### EqualRect  

```c
Boolean EqualRect(const Rect* r1, const Rect* r2)
```

Trap: `0xA8A6` executor=C_

### EqualRgn  

```c
Boolean EqualRgn(RgnHandle r1, RgnHandle r2)
```

Trap: `0xA8E3` executor=C_

### EraseArc  

```c
void EraseArc(const Rect* r, INTEGER start, INTEGER angle)
```

Trap: `0xA8C0` executor=C_

### EraseOval  

```c
void EraseOval(const Rect* r)
```

Trap: `0xA8B9` executor=C_

### ErasePoly  

```c
void ErasePoly(PolyHandle poly)
```

Trap: `0xA8C8` executor=C_

### EraseRect  

```c
void EraseRect(const Rect* r)
```

Trap: `0xA8A3` executor=C_

### EraseRgn  

```c
void EraseRgn(RgnHandle rh)
```

Trap: `0xA8D4` executor=C_

### EraseRoundRect  

```c
void EraseRoundRect(const Rect* r, INTEGER ow, INTEGER oh)
```

Trap: `0xA8B2` executor=C_

### FillArc  

```c
void FillArc(const Rect* r, INTEGER start, INTEGER angle, const Pattern* pat)
```

Trap: `0xA8C2` executor=C_

### FillOval  

```c
void FillOval(const Rect* r, const Pattern* pat)
```

Trap: `0xA8BB` executor=C_

### FillPoly  

```c
void FillPoly(PolyHandle poly, const Pattern* pat)
```

Trap: `0xA8CA` executor=C_

### FillRect  

```c
void FillRect(const Rect* r, const Pattern* pat)
```

Trap: `0xA8A5` executor=C_

### FillRgn  

```c
void FillRgn(RgnHandle rh, const Pattern* pat)
```

Trap: `0xA8D6` executor=C_

### FillRoundRect  

```c
void FillRoundRect(const Rect* r, INTEGER ow, INTEGER oh, const Pattern* pat)
```

Trap: `0xA8B4` executor=C_

### ForeColor  

```c
void ForeColor(LONGINT c)
```

Trap: `0xA862` executor=C_

### FrameArc  

```c
void FrameArc(const Rect* r, INTEGER start, INTEGER angle)
```

Trap: `0xA8BE` executor=C_

### FrameOval  

```c
void FrameOval(const Rect* r)
```

Trap: `0xA8B7` executor=C_

### FramePoly  

```c
void FramePoly(PolyHandle poly)
```

Trap: `0xA8C6` executor=C_

### FrameRect  

```c
void FrameRect(const Rect* r)
```

Trap: `0xA8A1` executor=C_

### FrameRgn  

```c
void FrameRgn(RgnHandle rh)
```

Trap: `0xA8D2` executor=C_

### FrameRoundRect  

```c
void FrameRoundRect(const Rect* r, INTEGER ow, INTEGER oh)
```

Trap: `0xA8B0` executor=C_

### GetCPixel  

```c
void GetCPixel(INTEGER h, INTEGER v, RGBColor* colorp)
```

Trap: `0xAA17` executor=C_

### GetClip  

```c
void GetClip(RgnHandle r)
```

Trap: `0xA87A` executor=C_

### GetFontInfo  

```c
void GetFontInfo(FontInfo* ip)
```

Trap: `0xA88B` executor=C_

### GetMaskTable  

```c
INTEGER* GetMaskTable()
```

Trap: `0xA836` executor=C_

### GetPen  

```c
void GetPen(Point* ptp)
```

Trap: `0xA89A` executor=C_

### GetPenState  

```c
void GetPenState(PenState* ps)
```

Trap: `0xA898` executor=C_

### GetPixel  

```c
Boolean GetPixel(INTEGER h, INTEGER v)
```

Trap: `0xA865` executor=C_

### GetPort  

```c
void GetPort(GrafPtr* pp)
```

Trap: `0xA874` executor=C_

### GlobalToLocal  

```c
void GlobalToLocal(Point* pt)
```

Trap: `0xA871` executor=C_

### GrafDevice  

```c
void GrafDevice(INTEGER d)
```

Trap: `0xA872` executor=C_

### HideCursor  

```c
void HideCursor()
```

Trap: `0xA852` executor=C_

### HidePen  

```c
void HidePen()
```

Trap: `0xA896` executor=C_

### InitCursor  

```c
void InitCursor()
```

Trap: `0xA850` executor=C_

### InitGraf  

```c
void InitGraf(GrafPtr* gp)
```

Trap: `0xA86E` executor=C_

### InitPort  

```c
void InitPort(GrafPtr p)
```

Trap: `0xA86D` executor=C_

### InsetRect  

```c
void InsetRect(Rect* r, INTEGER dh, INTEGER dv)
```

Trap: `0xA8A9` executor=C_

### InsetRgn  

```c
void InsetRgn(RgnHandle rh, INTEGER dh, INTEGER dv)
```

Trap: `0xA8E1` executor=C_

### InvertArc  

```c
void InvertArc(const Rect* r, INTEGER start, INTEGER angle)
```

Trap: `0xA8C1` executor=C_

### InvertOval  

```c
void InvertOval(const Rect* r)
```

Trap: `0xA8BA` executor=C_

### InvertPoly  

```c
void InvertPoly(PolyHandle poly)
```

Trap: `0xA8C9` executor=C_

### InvertRect  

```c
void InvertRect(const Rect* r)
```

Trap: `0xA8A4` executor=C_

### InvertRgn  

```c
void InvertRgn(RgnHandle rh)
```

Trap: `0xA8D5` executor=C_

### InvertRoundRect  

```c
void InvertRoundRect(const Rect* r, INTEGER ow, INTEGER oh)
```

Trap: `0xA8B3` executor=C_

### KillPicture  

```c
void KillPicture(PicHandle pic)
```

Trap: `0xA8F5` executor=C_

### KillPoly  

```c
void KillPoly(PolyHandle poly)
```

Trap: `0xA8CD` executor=C_

### Line  

```c
void Line(INTEGER dh, INTEGER dv)
```

Trap: `0xA892` executor=C_

### LineTo  

```c
void LineTo(INTEGER h, INTEGER v)
```

Trap: `0xA891` executor=C_

### LocalToGlobal  

```c
void LocalToGlobal(Point* pt)
```

Trap: `0xA870` executor=C_

### MakeRGBPat  

```c
void MakeRGBPat(PixPatHandle ph, RGBColor* colorp)
```

Trap: `0xAA0D` executor=C_

### MapPoly  

```c
void MapPoly(PolyHandle poly, const Rect* srcr, const Rect* dstr)
```

Trap: `0xA8FC` executor=C_

### MapPt  

```c
void MapPt(Point* pt, const Rect* srcr, const Rect* dstr)
```

Trap: `0xA8F9` executor=C_

### MapRect  

```c
void MapRect(Rect* r, const Rect* srcr, const Rect* dstr)
```

Trap: `0xA8FA` executor=C_

### MapRgn  

```c
void MapRgn(RgnHandle rh, const Rect* srcr, const Rect* dstr)
```

Trap: `0xA8FB` executor=C_

### MeasureText  

```c
void MeasureText(INTEGER n, Ptr text, Ptr chars)
```

Trap: `0xA837` executor=C_

### Move  

```c
void Move(INTEGER dh, INTEGER dv)
```

Trap: `0xA894` executor=C_

### MovePortTo  

```c
void MovePortTo(INTEGER lg, INTEGER tg)
```

Trap: `0xA877` executor=C_

### MoveTo  

```c
void MoveTo(INTEGER h, INTEGER v)
```

Trap: `0xA893` executor=C_

### NewRgn  

```c
RgnHandle NewRgn()
```

Trap: `0xA8D8` executor=C_

### ObscureCursor  

```c
void ObscureCursor()
```

Trap: `0xA856` executor=C_

### OffsetPoly  

```c
void OffsetPoly(PolyHandle poly, INTEGER dh, INTEGER dv)
```

Trap: `0xA8CE` executor=C_

### OffsetRect  

```c
void OffsetRect(Rect* r, INTEGER dh, INTEGER dv)
```

Trap: `0xA8A8` executor=C_

### OffsetRgn  

```c
void OffsetRgn(RgnHandle rh, INTEGER dh, INTEGER dv)
```

Trap: `0xA8E0` executor=C_

### OpenPicture  

```c
PicHandle OpenPicture(const Rect* pf)
```

Trap: `0xA8F3` executor=C_

### OpenPoly  

```c
PolyHandle OpenPoly()
```

Trap: `0xA8CB` executor=C_

### OpenPort  

```c
void OpenPort(GrafPtr p)
```

Trap: `0xA86F` executor=C_

### OpenRgn  

```c
void OpenRgn()
```

Trap: `0xA8DA` executor=C_

### PaintArc  

```c
void PaintArc(const Rect* r, INTEGER start, INTEGER angle)
```

Trap: `0xA8BF` executor=C_

### PaintOval  

```c
void PaintOval(const Rect* r)
```

Trap: `0xA8B8` executor=C_

### PaintPoly  

```c
void PaintPoly(PolyHandle poly)
```

Trap: `0xA8C7` executor=C_

### PaintRect  

```c
void PaintRect(const Rect* r)
```

Trap: `0xA8A2` executor=C_

### PaintRgn  

```c
void PaintRgn(RgnHandle rh)
```

Trap: `0xA8D3` executor=C_

### PaintRoundRect  

```c
void PaintRoundRect(const Rect* r, INTEGER ow, INTEGER oh)
```

Trap: `0xA8B1` executor=C_

### PenMode  

```c
void PenMode(INTEGER m)
```

Trap: `0xA89C` executor=C_

### PenNormal  

```c
void PenNormal()
```

Trap: `0xA89E` executor=C_

### PenPat  

```c
void PenPat(const Pattern* pp)
```

Trap: `0xA89D` executor=C_

### PenSize  

```c
void PenSize(INTEGER w, INTEGER h)
```

Trap: `0xA89B` executor=C_

### PicComment  

```c
void PicComment(INTEGER kind, INTEGER size, Handle hand)
```

Trap: `0xA8F2` executor=C_

### PortSize  

```c
void PortSize(INTEGER w, INTEGER h)
```

Trap: `0xA876` executor=C_

### Pt2Rect  

```c
void Pt2Rect(Point p1, Point p2, Rect* dest)
```

Trap: `0xA8AC` executor=C_

### PtInRect  

```c
Boolean PtInRect(Point p, const Rect* r)
```

Trap: `0xA8AD` executor=C_

### PtInRgn  

```c
Boolean PtInRgn(Point p, RgnHandle rh)
```

Trap: `0xA8E8` executor=C_

### PtToAngle  

```c
void PtToAngle(const Rect* rp, Point p, INTEGER* angle)
```

Trap: `0xA8C3` executor=C_

### Random  

```c
INTEGER Random()
```

Trap: `0xA861` executor=C_

### RectInRgn  

```c
Boolean RectInRgn(const Rect* rp, RgnHandle rh)
```

Trap: `0xA8E9` executor=C_

### RectRgn  

```c
void RectRgn(RgnHandle rh, const Rect* rect)
```

Trap: `0xA8DF` executor=C_

### ScalePt  

```c
void ScalePt(Point* pt, const Rect* srcr, const Rect* dstr)
```

Trap: `0xA8F8` executor=C_

### ScrollRect  

```c
void ScrollRect(Rect* rp, INTEGER dh, INTEGER dv, RgnHandle updatergn)
```

Trap: `0xA8EF` executor=C_

### SectRect  

```c
Boolean SectRect(const Rect* s1, const Rect* s2, Rect* dest)
```

Trap: `0xA8AA` executor=C_

### SectRgn  

```c
void SectRgn(RgnHandle s1, RgnHandle s2, RgnHandle dest)
```

Trap: `0xA8E4` executor=C_

### SeedCFill  

```c
void SeedCFill(BitMap* srcbp, BitMap* dstbp, const Rect* srcrp, const Rect* dstrp, INTEGER seedh, INTEGER seedv, ProcPtr matchprocp, LONGINT matchdata)
```

Trap: `0xAA50` executor=C_

### SeedFill  

```c
void SeedFill(Ptr srcp, Ptr dstp, INTEGER srcr, INTEGER dstr, INTEGER height, INTEGER width, INTEGER seedh, INTEGER seedv)
```

Trap: `0xA839` executor=C_

### SetCPixel  

```c
void SetCPixel(INTEGER h, INTEGER v, RGBColor* colorp)
```

Trap: `0xAA16` executor=C_

### SetClip  

```c
void SetClip(RgnHandle r)
```

Trap: `0xA879` executor=C_

### SetCursor  

```c
void SetCursor(Cursor* cp)
```

Trap: `0xA851` executor=C_

### SetEmptyRgn  

```c
void SetEmptyRgn(RgnHandle rh)
```

Trap: `0xA8DD` executor=C_

### SetOrigin  

```c
void SetOrigin(INTEGER h, INTEGER v)
```

Trap: `0xA878` executor=C_

### SetPenState  

```c
void SetPenState(PenState* ps)
```

Trap: `0xA899` executor=C_

### SetPort  

```c
void SetPort(GrafPtr p)
```

Trap: `0xA873` executor=C_

### SetPortBits  

```c
void SetPortBits(BitMap* bm)
```

Trap: `0xA875` executor=C_

### SetPt  

```c
void SetPt(Point* pt, INTEGER h, INTEGER v)
```

Trap: `0xA880` executor=C_

### SetRect  

```c
void SetRect(Rect* r, INTEGER left, INTEGER top, INTEGER right, INTEGER bottom)
```

Trap: `0xA8A7` executor=C_

### SetRectRgn  

```c
void SetRectRgn(RgnHandle rh, INTEGER left, INTEGER top, INTEGER right, INTEGER bottom)
```

Trap: `0xA8DE` executor=C_

### SetStdProcs  

```c
void SetStdProcs(QDProcs* procs)
```

Trap: `0xA8EA` executor=C_

### ShieldCursor  

```c
void ShieldCursor(Rect* rp, Point p)
```

Trap: `0xA855` executor=C_

### ShowCursor  

```c
void ShowCursor()
```

Trap: `0xA853` executor=C_

### ShowPen  

```c
void ShowPen()
```

Trap: `0xA897` executor=C_

### SpaceExtra  

```c
void SpaceExtra(Fixed e)
```

Trap: `0xA88E` executor=C_

### StdArc  

```c
void StdArc(GrafVerb verb, const Rect* r, INTEGER starta, INTEGER arca)
```

Trap: `0xA8BD` executor=C_

### StdBits  

```c
void StdBits(const BitMap* srcbmp, const Rect* srcrp, const Rect* dstrp, INTEGER mode, RgnHandle mask)
```

Trap: `0xA8EB` executor=C_

### StdComment  

```c
void StdComment(INTEGER kind, INTEGER size, Handle hand)
```

Trap: `0xA8F1` executor=C_

### StdGetPic  

```c
void StdGetPic(void* dp, INTEGER bc)
```

Trap: `0xA8EE` executor=C_

### StdLine  

```c
void StdLine(Point p)
```

Trap: `0xA890` executor=C_

### StdOval  

```c
void StdOval(GrafVerb v, const Rect* rp)
```

Trap: `0xA8B6` executor=C_

### StdPoly  

```c
void StdPoly(GrafVerb verb, PolyHandle ph)
```

Trap: `0xA8C5` executor=C_

### StdPutPic  

```c
void StdPutPic(const void* sp, INTEGER bc)
```

Trap: `0xA8F0` executor=C_

### StdRRect  

```c
void StdRRect(GrafVerb verb, const Rect* r, INTEGER width, INTEGER height)
```

Trap: `0xA8AF` executor=C_

### StdRect  

```c
void StdRect(GrafVerb v, const Rect* rp)
```

Trap: `0xA8A0` executor=C_

### StdRgn  

```c
void StdRgn(GrafVerb verb, RgnHandle rgn)
```

Trap: `0xA8D1` executor=C_

### StdText  

```c
void StdText(INTEGER n, Ptr textbufp, Point num, Point den)
```

Trap: `0xA882` executor=C_

### StdTxMeas  

```c
INTEGER StdTxMeas(INTEGER n, Ptr p, Point* nump, Point* denp, FontInfo* finfop)
```

Trap: `0xA8ED` executor=C_

### StringWidth  

```c
INTEGER StringWidth(ConstStringPtr s)
```

Trap: `0xA88C` executor=C_

### StuffHex  

```c
void StuffHex(Ptr p, ConstStringPtr s)
```

Trap: `0xA866` executor=C_

### SubPt  

```c
void SubPt(Point src, Point* dst)
```

Trap: `0xA87F` executor=C_

### TextFace  

```c
void TextFace(INTEGER thef)
```

Trap: `0xA888` executor=C_

### TextFont  

```c
void TextFont(INTEGER f)
```

Trap: `0xA887` executor=C_

### TextMode  

```c
void TextMode(INTEGER m)
```

Trap: `0xA889` executor=C_

### TextSize  

```c
void TextSize(INTEGER s)
```

Trap: `0xA88A` executor=C_

### TextWidth  

```c
INTEGER TextWidth(Ptr tb, INTEGER fb, INTEGER bc)
```

Trap: `0xA886` executor=C_

### UnionRect  

```c
void UnionRect(const Rect* s1, const Rect* s2, Rect* dest)
```

Trap: `0xA8AB` executor=C_

### UnionRgn  

```c
void UnionRgn(RgnHandle s1, RgnHandle s2, RgnHandle dest)
```

Trap: `0xA8E5` executor=C_

### XorRgn  

```c
void XorRgn(RgnHandle s1, RgnHandle s2, RgnHandle dest)
```

Trap: `0xA8E7` executor=C_

### GetPortBitMapForCopyBits  

```c
BitMap * GetPortBitMapForCopyBits(CGrafPtr p)
```

Trap: — (executor 实现，无 trap) **[carbon]**

### GetPortBounds  

```c
void GetPortBounds(CGrafPtr p, Rect * r)
```

Trap: — (executor 实现，无 trap) **[carbon]**

### GetPortTextFont  

```c
int16_t GetPortTextFont(CGrafPtr p)
```

Trap: — (executor 实现，无 trap) **[carbon]**

### GetPortTextSize  

```c
int16_t GetPortTextSize(CGrafPtr p)
```

Trap: — (executor 实现，无 trap) **[carbon]**

### GetPortVisibleRegion  

```c
void GetPortVisibleRegion(CGrafPtr p, RgnHandle rgn)
```

Trap: — (executor 实现，无 trap) **[carbon]**

### GetQDGlobalsScreenBits  

```c
void GetQDGlobalsScreenBits(BitMap * bm)
```

Trap: — (executor 实现，无 trap) **[carbon]**

### GetRegionBounds  

```c
void GetRegionBounds(RgnHandle rgn, Rect * r)
```

Trap: — (executor 实现，无 trap) **[carbon]**

### QDFlushPortBuffer  

```c
void QDFlushPortBuffer(CGrafPtr p, RgnHandle rgn)
```

Trap: — (executor 实现，无 trap) **[carbon]**

## Typedefs

- **Style** = SignedByte
- **RgnPtr** = Region*
- **RgnHandle** = RgnPtr*
- **Bits16** = INTEGER[16]
- **CursPtr** = Cursor*
- **CursHandle** = CursPtr*
- **GrafVerb** = SignedByte
- **PolyPtr** = Polygon*
- **PolyHandle** = PolyPtr*
- **QDProcsPtr** = QDProcs*
- **GrafPtr** = GrafPort*
- **PicPtr** = Picture*
- **PicHandle** = PicPtr*
- **cSpecArray** = ColorSpec[1] — can't use 0
- **CTabPtr** = ColorTable*
- **CTabHandle** = CTabPtr*
- **CQDProcsPtr** = CQDProcs*
- **PixMapPtr** = PixMap*
- **PixMapHandle** = PixMapPtr*
- **PixPatPtr** = PixPat*
- **PixPatHandle** = PixPatPtr*
- **CGrafPtr** = CGrafPort*
- **CGrafPtr** = GrafPtr — In Carbon, all ports are color and you're not allowed to access the struct anyway.
- **CCrsrPtr** = CCrsr*
- **CCrsrHandle** = CCrsrPtr*
- **BytePtr** = Byte*
- **QDGlobalsPtr** = QDGlobals*

## Enums

- **?** — number of bytes InitGraf needs
- **?**
- **?**
- **?**
- **?**
- **?**
- **?**
- **StyleItem**
- **?**
- **?** — IMV stuff is used when we parse Version 2 pictures, but the IMV calls are not supported in V1.0
- **?**
- **?**
- **?**

### Enum Values

**anonymous** — number of bytes InitGraf needs:

- `grafSize` = 206

**anonymous**:

- `srcCopy` = 0
- `srcOr` = 1
- `srcXor` = 2
- `srcBic` = 3
- `notSrcCopy` = 4
- `notSrcOr` = 5
- `notSrcXor` = 6
- `notSrcBic` = 7

**anonymous**:

- `patCopy` = 8
- `patOr` = 9
- `patXor` = 10
- `patBic` = 11
- `notPatCopy` = 12
- `notPatOr` = 13
- `notPatXor` = 14
- `notPatBic` = 15

**anonymous**:

- `grayishTextOr` = 49

**anonymous**:

- `hilite` = 50

**anonymous**:

- `blackColor` = 33
- `whiteColor` = 30
- `redColor` = 205
- `greenColor` = 341
- `blueColor` = 409
- `cyanColor` = 273
- `magentaColor` = 137
- `yellowColor` = 69

**anonymous**:

- `picLParen` = 0
- `picRParen` = 1

**StyleItem**:

- `normal` = 0
- `bold` = 1
- `italic` = 2
- `underline` = 4
- `outline` = 8
- `shadow` = 16
- `condense` = 32
- `extend` = 64

**anonymous**:

- `frame` = 0
- `paint` = 1
- `erase` = 2
- `invert` = 3
- `fill` = 4

**anonymous** — IMV stuff is used when we parse Version 2 pictures, but the IMV calls are not supported in V1.0:

- `blend` = 32
- `addPin` = ?
- `addOver` = ?
- `subPin` = ?
- `transparent` = ?
- `adMax` = ?
- `subOver` = ?
- `adMin` = ?
- `mask` = 64

**anonymous**:

- `pHiliteBit` = 0
- `hiliteBit` = 7

**anonymous**:

- `defQDColors` = 127

**anonymous**:

- `ROWMASK` = 8191

## Structs

- **Region** { rgnSize: INTEGER, rgnBBox: Rect }
- **BitMap** { baseAddr: Ptr, rowBytes: INTEGER, bounds: Rect }
- **Pattern** { pat: Byte[8] }
- **Cursor** { data: Bits16, mask: Bits16, hotSpot: Point }
- **Polygon** { polySize: INTEGER, polyBBox: Rect, polyPoints: Point[1] }
- **FontInfo** { ascent: INTEGER, descent: INTEGER, widMax: INTEGER, leading: INTEGER }
- **QDProcs** { textProc: QDTextUPP, lineProc: QDLineUPP, rectProc: QDRectUPP, rRectProc: QDRRectUPP, ovalProc: QDOvalUPP, arcProc: QDArcUPP, polyProc: QDPolyUPP, rgnProc: QDRgnUPP, bitsProc: QDBitsUPP, commentProc: QDCommentUPP, txMeasProc: QDTexMeasUPP, getPicProc: QDGetPicUPP, putPicProc: QDPutPicUPP }
- **GrafPort** { device: INTEGER, portBits: BitMap, portRect: Rect, visRgn: RgnHandle, clipRgn: RgnHandle, bkPat: Pattern, fillPat: Pattern, pnLoc: Point, pnSize: Point, pnMode: INTEGER, pnPat: Pattern, pnVis: INTEGER, txFont: INTEGER, txFace: Style, filler: Byte, txMode: INTEGER, txSize: INTEGER, spExtra: Fixed, fgColor: LONGINT, bkColor: LONGINT, colrBit: INTEGER, patStretch: INTEGER, picSave: Handle, rgnSave: Handle, polySave: Handle, grafProcs: QDProcsPtr }
- **Picture** { picSize: INTEGER, picFrame: Rect }
- **PenState** { pnLoc: Point, pnSize: Point, pnMode: INTEGER, pnPat: Pattern }
- **RGBColor** { red: uint16_t, green: uint16_t, blue: uint16_t }
- **HSVColor** { hue: SmallFract, saturation: SmallFract, value: SmallFract }
- **HSLColor** { hue: SmallFract, saturation: SmallFract, lightness: SmallFract }
- **CMYColor** { cyan: SmallFract, magenta: SmallFract, yellow: SmallFract }
- **ColorSpec** { value: INTEGER, rgb: RGBColor }
- **ColorTable** { ctSeed: LONGINT, ctFlags: uint16_t, ctSize: INTEGER, ctTable: cSpecArray }
- **CQDProcs** { textProc: QDTextUPP, lineProc: QDLineUPP, rectProc: QDRectUPP, rRectProc: QDRRectUPP, ovalProc: QDOvalUPP, arcProc: QDArcUPP, polyProc: QDPolyUPP, rgnProc: QDRgnUPP, bitsProc: QDBitsUPP, commentProc: QDCommentUPP, txMeasProc: QDTexMeasUPP, getPicProc: QDGetPicUPP, putPicProc: QDPutPicUPP, opcodeProc: Ptr, newProc1Proc: Ptr, newProc2Proc: Ptr, newProc3Proc: Ptr, newProc4Proc: Ptr, newProc5Proc: Ptr, newProc6Proc: Ptr }
- **PixMap** { baseAddr: Ptr, rowBytes: INTEGER, bounds: Rect, pmVersion: INTEGER, packType: INTEGER, packSize: LONGINT, hRes: Fixed, vRes: Fixed, pixelType: INTEGER, pixelSize: INTEGER, cmpCount: INTEGER, cmpSize: INTEGER, planeBytes: LONGINT, pmTable: CTabHandle, pmReserved: LONGINT }
- **PixPat** { patType: INTEGER, patMap: PixMapHandle, patData: Handle, patXData: Handle, patXValid: INTEGER, patXMap: Handle, pat1Data: Pattern }
- **CGrafPort** { device: INTEGER, portPixMap: PixMapHandle, portVersion: INTEGER, grafVars: Handle, chExtra: INTEGER, pnLocHFrac: INTEGER, portRect: Rect, visRgn: RgnHandle, clipRgn: RgnHandle, bkPixPat: PixPatHandle, rgbFgColor: RGBColor, rgbBkColor: RGBColor, pnLoc: Point, pnSize: Point, pnMode: INTEGER, pnPixPat: PixPatHandle, fillPixPat: PixPatHandle, pnVis: INTEGER, txFont: INTEGER, txFace: Style, filler: Byte, txMode: INTEGER, txSize: INTEGER, spExtra: Fixed, fgColor: LONGINT, bkColor: LONGINT, colrBit: INTEGER, patStretch: INTEGER, picSave: Handle, rgnSave: Handle, polySave: Handle, grafProcs: CQDProcsPtr }
- **CCrsr** { crsrType: INTEGER, crsrMap: PixMapHandle, crsrData: Handle, crsrXData: Handle, crsrXValid: INTEGER, crsrXHandle: Handle, crsr1Data: Bits16, crsrMask: Bits16, crsrHotSpot: Point, crsrXTable: LONGINT, crsrID: LONGINT }
- **MatchRec** { red: uint16_t, green: uint16_t, blue: uint16_t, matchData: int32_t }
- **QDGlobals** { privates: char[76], randSeed: int32_t, screenBits: BitMap, arrow: Cursor, dkGray: Pattern, ltGray: Pattern, gray: Pattern, black: Pattern, white: Pattern, thePort: GrafPtr }

## Function Pointers

- **QDTextUPP** (bc: INTEGER, textb: Ptr, num: Point, den: Point) -> void
- **QDLineUPP** (drawto: Point) -> void
- **QDRectUPP** (verb: GrafVerb, rp: const Rect*) -> void
- **QDRRectUPP** (verb: GrafVerb, rp: const Rect*, ow: INTEGER, oh: INTEGER) -> void
- **QDOvalUPP** (verb: GrafVerb, rp: const Rect*) -> void
- **QDArcUPP** (verb: GrafVerb, rp: const Rect*, ang: INTEGER, arc: INTEGER) -> void
- **QDPolyUPP** (verb: GrafVerb, poly: PolyHandle) -> void
- **QDRgnUPP** (verb: GrafVerb, rgn: RgnHandle) -> void
- **QDBitsUPP** (srcb: const BitMap*, srcr: const Rect*, dstr: const Rect*, mod: INTEGER, mask: RgnHandle) -> void
- **QDCommentUPP** (kind: INTEGER, size: INTEGER, data: Handle) -> void
- **QDTexMeasUPP** (bc: INTEGER, texta: Ptr, numer: Point*, denom: Point*, info: FontInfo*) -> INTEGER
- **QDGetPicUPP** (data: void*, bc: INTEGER) -> void
- **QDPutPicUPP** (data: const void*, bc: INTEGER) -> void

## Low Memory Globals

- **ScrVRes** @ 0x102 (INTEGER) — QuickDraw IMI-473 (true);
- **ScrHRes** @ 0x104 (INTEGER) — QuickDraw IMI-473 (true);
- **ScreenRow** @ 0x106 (INTEGER) — QuickDraw ThinkC (true);
- **RndSeed** @ 0x156 (LONGINT) — QuickDraw IMI-195 (true);
- **ScreenVars** @ 0x292 (Byte[8]) — QuickDraw MPW (false);
- **Key1Trans** @ 0x29E (Ptr) — * NOTE: Key1Trans in the keyboard translator procedure, and Key2Trans in the * numeric keypad translator procedure (MPW). QuickDraw MPW (false);
- **Key2Trans** @ 0x2A2 (Ptr) — QuickDraw MPW (false);
- **JUnknown574** @ 0x574 (ProcPtr) — QuickDraw IMV (true-b);
- **JADBProc** @ 0x6B8 (ProcPtr) — QuickDraw IMV (false);
- **JHideCursor** @ 0x800 (ProcPtr) — QuickDraw Private.a (true-b);
- **JShowCursor** @ 0x804 (ProcPtr) — QuickDraw Private.a (true-b);
- **JShieldCursor** @ 0x808 (ProcPtr) — QuickDraw Private.a (true-b);
- **JScrnAddr** @ 0x80C (ProcPtr) — QuickDraw Private.a (false);
- **JScrnSize** @ 0x810 (ProcPtr) — QuickDraw Private.a (false);
- **JInitCrsr** @ 0x814 (ProcPtr) — QuickDraw Private.a (true-b);
- **JSetCrsr** @ 0x818 (ProcPtr) — QuickDraw Private.a (true-b);
- **JCrsrObscure** @ 0x81C (ProcPtr) — QuickDraw Private.a (true-b);
- **JUpdateProc** @ 0x820 (ProcPtr) — QuickDraw Private.a (false);
- **ScrnBase** @ 0x824 (Ptr) — QuickDraw IMII-19 (true);
- **CrsrPin** @ 0x834 (Rect) — * MouseLocation used to be 0x830, but that doesn't jibe with what I've * seen of Crystal Quest --ctm QuickDraw ThinkC (false);
- **QDColors** @ 0x8B0 (Byte) — QuickDraw IMV (false);
- **CrsrVis** @ 0x8CC (Boolean) — QuickDraw SysEqu.a (true);
- **CrsrBusy** @ 0x8CD (Byte) — QuickDraw SysEqu.a (true);
- **CrsrState** @ 0x8D0 (INTEGER) — QuickDraw SysEqu.a (true);
- **mousemask** @ 0x8D6 (LONGINT) — QuickDraw .a (true-b);
- **mouseoffset** @ 0x8DA (LONGINT) — QuickDraw SysEqu.a (true-b);
- **JCrsrTask** @ 0x8EE (ProcPtr) — (true);
- **HiliteMode** @ 0x938 (Byte) — QuickDraw IMV (true-b);
- **PortList** @ 0xD66 (Handle) — undocumented; 2-byte count followed by array of GrafPtrs
