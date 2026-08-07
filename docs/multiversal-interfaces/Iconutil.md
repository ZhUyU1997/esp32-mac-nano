# Iconutil Interfaces

IconAlignmentType values

Source: `multiversal/defs/Iconutil.yaml`

- Functions: **35**
- Typedefs: **7**
- Structs: **1**, Unions: **0**
- Enums: **7**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **1**
- Low-memory globals: **0**

## Functions

### AddIconToSuite  

```c
OSErr AddIconToSuite(Handle icon_data, Handle suite, ResType type)
```

Trap: — (executor 实现，无 trap) executor=C_

### DisposeCIcon  

```c
void DisposeCIcon(CIconHandle icon)
```

Trap: `0xAA25` executor=C_

### DisposeIconSuite  

```c
OSErr DisposeIconSuite(Handle suite, Boolean dispose_data_p)
```

Trap: — (executor 实现，无 trap) executor=C_

### ForEachIconDo  

```c
OSErr ForEachIconDo(Handle suite, IconSelectorValue selector, IconActionUPP action, void* data)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetCIcon  

```c
CIconHandle GetCIcon(int16_t icon_id)
```

Trap: `0xAA1E` executor=C_

### GetIcon  

```c
Handle GetIcon(int16_t icon_id)
```

Trap: `0xA9BB` executor=C_

### GetIconCacheData  

```c
OSErr GetIconCacheData(Handle cache, void** data)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetIconCacheProc  

```c
OSErr GetIconCacheProc(Handle cache, IconGetterUPP* proc)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetIconFromSuite  

```c
OSErr GetIconFromSuite(Handle* icon_data, Handle suite, ResType type)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetIconSuite  

```c
OSErr GetIconSuite(Handle* suite, int16_t res_id, IconSelectorValue selector)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetLabel  

```c
OSErr GetLabel(int16_t label, RGBColor* label_color, Str255 label_string)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetSuiteLabel  

```c
int16_t GetSuiteLabel(Handle suite)
```

Trap: — (executor 实现，无 trap) executor=C_

### IconIDToRgn  

```c
OSErr IconIDToRgn(RgnHandle rgn, const Rect* rect, IconAlignmentType align, int16_t icon_id)
```

Trap: — (executor 实现，无 trap) executor=C_

### IconMethodToRgn  

```c
OSErr IconMethodToRgn(RgnHandle rgn, const Rect* rect, IconAlignmentType align, IconGetterUPP method, void* data)
```

Trap: — (executor 实现，无 trap) executor=C_

### IconSuiteToRgn  

```c
OSErr IconSuiteToRgn(RgnHandle rgn, const Rect* rect, IconAlignmentType align, Handle suite)
```

Trap: — (executor 实现，无 trap) executor=C_

### LoadIconCache  

```c
OSErr LoadIconCache(const Rect* rect, IconAlignmentType align, IconTransformType transform, Handle cache)
```

Trap: — (executor 实现，无 trap) executor=C_

### MakeIconCache  

```c
OSErr MakeIconCache(Handle* cache, IconGetterUPP make_icon, void* data)
```

Trap: — (executor 实现，无 trap) executor=C_

### NewIconSuite  

```c
OSErr NewIconSuite(Handle* suite)
```

Trap: — (executor 实现，无 trap) executor=C_

### PlotCIcon  

```c
void PlotCIcon(const Rect* rect, CIconHandle icon)
```

Trap: `0xAA1F` executor=C_

### PlotCIconHandle  

```c
OSErr PlotCIconHandle(const Rect* rect, IconAlignmentType align, IconTransformType transform, CIconHandle icon)
```

Trap: — (executor 实现，无 trap) executor=C_

### PlotIcon  

```c
void PlotIcon(const Rect* rect, Handle icon)
```

Trap: `0xA94B` executor=C_

### PlotIconHandle  

```c
OSErr PlotIconHandle(const Rect* rect, IconAlignmentType align, IconTransformType transform, Handle icon)
```

Trap: — (executor 实现，无 trap) executor=C_

### PlotIconID  

```c
OSErr PlotIconID(const Rect* rect, IconAlignmentType align, IconTransformType tranform, int16_t res_id)
```

Trap: — (executor 实现，无 trap) executor=C_

### PlotIconMethod  

```c
OSErr PlotIconMethod(const Rect* rect, IconAlignmentType align, IconTransformType transform, IconGetterUPP method, void* data)
```

Trap: — (executor 实现，无 trap) executor=C_

### PlotIconSuite  

```c
OSErr PlotIconSuite(const Rect* rect, IconAlignmentType align, IconTransformType transform, Handle suite)
```

Trap: — (executor 实现，无 trap) executor=C_

### PlotSICNHandle  

```c
OSErr PlotSICNHandle(const Rect* rect, IconAlignmentType align, IconTransformType transform, Handle icon)
```

Trap: — (executor 实现，无 trap) executor=C_

### PtInIconID  

```c
Boolean PtInIconID(Point test_pt, const Rect* rect, IconAlignmentType align, int16_t icon_id)
```

Trap: — (executor 实现，无 trap) executor=C_

### PtInIconMethod  

```c
Boolean PtInIconMethod(Point test_pt, const Rect* rect, IconAlignmentType align, IconGetterUPP method, void* data)
```

Trap: — (executor 实现，无 trap) executor=C_

### PtInIconSuite  

```c
Boolean PtInIconSuite(Point test_pt, const Rect* rect, IconAlignmentType align, Handle suite)
```

Trap: — (executor 实现，无 trap) executor=C_

### RectInIconID  

```c
Boolean RectInIconID(const Rect* test_rect, const Rect* rect, IconAlignmentType align, int16_t icon_id)
```

Trap: — (executor 实现，无 trap) executor=C_

### RectInIconMethod  

```c
Boolean RectInIconMethod(const Rect* test_rect, const Rect* rect, IconAlignmentType align, IconGetterUPP method, void* data)
```

Trap: — (executor 实现，无 trap) executor=C_

### RectInIconSuite  

```c
Boolean RectInIconSuite(const Rect* test_rect, const Rect* rect, IconAlignmentType align, Handle suite)
```

Trap: — (executor 实现，无 trap) executor=C_

### SetIconCacheData  

```c
OSErr SetIconCacheData(Handle cache, void* data)
```

Trap: — (executor 实现，无 trap) executor=C_

### SetIconCacheProc  

```c
OSErr SetIconCacheProc(Handle cache, IconGetterUPP proc)
```

Trap: — (executor 实现，无 trap) executor=C_

### SetSuiteLabel  

```c
OSErr SetSuiteLabel(Handle suite, int16_t label)
```

Trap: — (executor 实现，无 trap) executor=C_

## Typedefs

- **IconActionUPP** = ProcPtr
- **IconGetterUPP** = ProcPtr
- **IconSelectorValue** = uint32_t
- **IconAlignmentType** = int16_t
- **IconTransformType** = int16_t
- **CIconPtr** = CIcon*
- **CIconHandle** = CIconPtr*

## Enums

- **?**
- **?** — IconAlignmentType values
- **?**
- **?** — IconTranformType values
- **?**
- **?** — IconSelectorValue values #### what kind of eediot at apple named all the other icon flag types *Type, except this one?
- **?**

### Enum Values

**anonymous**:

- `large1BitMask` = 'ICN#'
- `large4BitData` = 'icl4'
- `large8BitData` = 'icl8'
- `small1BitMask` = 'ics#'
- `small4BitData` = 'ics4'
- `small8BitData` = 'ics8'
- `mini1BitMask` = 'icm#'
- `mini4BitData` = 'icm4'
- `mini8BitData` = 'icm8'

**anonymous** — IconAlignmentType values:

- `atNone` = 0
- `atVerticalCenter` = 1
- `atTop` = 2
- `atBottom` = 3
- `atHorizontalCenter` = 4
- `atAbsoluteCenter` = atVerticalCenter | atHorizontalCenter
- `atCenterTop` = atTop | atHorizontalCenter
- `atCenterBottom` = atBottom | atHorizontalCenter
- `atLeft` = 8
- `atCenterLeft` = atVerticalCenter | atLeft
- `atTopLeft` = atTop | atLeft
- `atBottomLeft` = atBottom | atLeft
- `atRight` = 12
- `atCenterRight` = atVerticalCenter | atRight
- `atTopRight` = atTop | atRight
- `atBottomRight` = atBottom | atRight

**anonymous**:

- `kAlignNode` = 0
- `kAlignVerticalCenter` = 1
- `kAlignTop` = 2
- `kAlignBottom` = 3
- `kAlignHorizontalCenter` = 4
- `kAlignAbsoluteCenter` = kAlignVerticalCenter | kAlignHorizontalCenter

**anonymous** — IconTranformType values:

- `ttNone` = 0
- `ttDisabled` = 1
- `ttOffline` = 2
- `ttOpen` = 3
- `ttLabel1` = 256
- `ttLabel2` = 512
- `ttLabel3` = 768
- `ttLabel4` = 1024
- `ttLabel5` = 1280
- `ttLabel6` = 1536
- `ttLabel7` = 1792
- `ttSelected` = 16384
- `ttSelectedDisabled` = ttSelected | ttDisabled
- `ttSelectedOffline` = ttSelected | ttOffline
- `ttSelectedOpen` = ttSelected | ttOpen

**anonymous**:

- `kTransformNone` = 0

**anonymous** — IconSelectorValue values #### what kind of eediot at apple named all the other icon flag types *Type, except this one?:

- `svLarge1Bit` = 1
- `svLarge4Bit` = 2
- `svLarge8Bit` = 4
- `svSmall1Bit` = 256
- `svSmall4Bit` = 512
- `svSmall8Bit` = 1024
- `svMini1Bit` = 65536
- `svMini4Bit` = 131072
- `svMini8Bit` = 262144
- `svAllLargeData` = 255
- `svAllSmallData` = 65280
- `svAllMiniData` = 16711680
- `svAll1BitData` = 65793
- `svAll4BitData` = 131586
- `svAll8BitData` = 263172
- `svAllAvailableData` = 16777215

**anonymous**:

- `noMaskFoundErr` = -1000

## Structs

- **CIcon** { iconPMap: PixMap, iconMask: BitMap, iconBMap: BitMap, iconData: Handle, iconMaskData: int16_t[1] }

## Dispatchers

- **IconDispatch**— icon utility function prototypes — icon utility function prototypes

