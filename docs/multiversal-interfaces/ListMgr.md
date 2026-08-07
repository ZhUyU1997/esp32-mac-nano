# ListMgr Interfaces

Source: `multiversal/defs/ListMgr.yaml`

- Functions: **26**
- Typedefs: **6**
- Structs: **1**, Unions: **0**
- Enums: **4**
- Function pointers: **1**
- Common blocks: **0**
- Dispatchers: **1**
- Low-memory globals: **0**

## Functions

### LActivate  

```c
void LActivate(Boolean act, ListHandle list)
```

Trap: — (executor 实现，无 trap) executor=C_

### LAddColumn  

```c
INTEGER LAddColumn(INTEGER count, INTEGER coln, ListHandle list)
```

Trap: — (executor 实现，无 trap) executor=C_

### LAddRow  

```c
INTEGER LAddRow(INTEGER count, INTEGER rown, ListHandle list)
```

Trap: — (executor 实现，无 trap) executor=C_

### LAddToCell  

```c
void LAddToCell(Ptr dp, INTEGER dl, Cell cell, ListHandle list)
```

Trap: — (executor 实现，无 trap) executor=C_

### LAutoScroll  

```c
void LAutoScroll(ListHandle list)
```

Trap: — (executor 实现，无 trap) executor=C_

### LCellSize  

```c
void LCellSize(Point csize, ListHandle list)
```

Trap: — (executor 实现，无 trap) executor=C_

### LClick  

```c
Boolean LClick(Point pt, INTEGER mods, ListHandle list)
```

Trap: — (executor 实现，无 trap) executor=C_

### LClrCell  

```c
void LClrCell(Cell cell, ListHandle list)
```

Trap: — (executor 实现，无 trap) executor=C_

### LDelColumn  

```c
void LDelColumn(INTEGER count, INTEGER coln, ListHandle list)
```

Trap: — (executor 实现，无 trap) executor=C_

### LDelRow  

```c
void LDelRow(INTEGER count, INTEGER rown, ListHandle list)
```

Trap: — (executor 实现，无 trap) executor=C_

### LDispose  

```c
void LDispose(ListHandle list)
```

Trap: — (executor 实现，无 trap) executor=C_

### LDraw  

```c
void LDraw(Cell cell, ListHandle list)
```

Trap: — (executor 实现，无 trap) executor=C_

### LGetCell  

```c
void LGetCell(Ptr dp, INTEGER* dlp, Cell cell, ListHandle list)
```

Trap: — (executor 实现，无 trap) executor=C_

### LGetCellDataLocation  

```c
void LGetCellDataLocation(INTEGER* offsetp, INTEGER* lenp, Cell cell, ListHandle list)
```

Trap: — (executor 实现，无 trap) executor=C_

### LGetSelect  

```c
Boolean LGetSelect(Boolean next, Cell* cellp, ListHandle list)
```

Trap: — (executor 实现，无 trap) executor=C_

### LLastClick  

```c
LONGINT LLastClick(ListHandle list)
```

Trap: — (executor 实现，无 trap) executor=C_

### LNew  

```c
ListHandle LNew(const Rect* rview, const Rect* bounds, Point csize, INTEGER proc, WindowPtr wind, Boolean draw, Boolean grow, Boolean scrollh, Boolean scrollv)
```

Trap: — (executor 实现，无 trap) executor=C_

### LNextCell  

```c
Boolean LNextCell(Boolean hnext, Boolean vnext, Cell* cellp, ListHandle list)
```

Trap: — (executor 实现，无 trap) executor=C_

### LRect  

```c
void LRect(Rect* cellrect, Cell cell, ListHandle list)
```

Trap: — (executor 实现，无 trap) executor=C_

### LScroll  

```c
void LScroll(INTEGER ncol, INTEGER nrow, ListHandle list)
```

Trap: — (executor 实现，无 trap) executor=C_

### LSearch  

```c
Boolean LSearch(Ptr dp, INTEGER dl, Ptr proc, Cell* cellp, ListHandle list)
```

Trap: — (executor 实现，无 trap) executor=C_

### LSetCell  

```c
void LSetCell(Ptr dp, INTEGER dl, Cell cell, ListHandle list)
```

Trap: — (executor 实现，无 trap) executor=C_

### LSetDrawingMode  

```c
void LSetDrawingMode(Boolean draw, ListHandle list)
```

Trap: — (executor 实现，无 trap) executor=C_

### LSetSelect  

```c
void LSetSelect(Boolean setit, Cell cell, ListHandle list)
```

Trap: — (executor 实现，无 trap) executor=C_

### LSize  

```c
void LSize(INTEGER width, INTEGER height, ListHandle list)
```

Trap: — (executor 实现，无 trap) executor=C_

### LUpdate  

```c
void LUpdate(RgnHandle rgn, ListHandle list)
```

Trap: — (executor 实现，无 trap) executor=C_

## Typedefs

- **Cell** = Point
- **DataArray** = Byte[32001]
- **DataPtr** = DataArray*
- **DataHandle** = DataPtr*
- **ListPtr** = ListRec*
- **ListHandle** = ListPtr*

## Enums

- **?**
- **?**
- **?**
- **?**

### Enum Values

**anonymous**:

- `lGrowBox` = 32
- `lMysteryFlags` = 20
- `lDrawingModeOff` = 8
- `lDoVAutoscroll` = 2
- `lDoHAutoscroll` = 1

**anonymous**:

- `lOnlyOne` = -128
- `lExtendDrag` = 64
- `lNoDisjoint` = 32

**anonymous**:

- `lNoExtend` = 16
- `lNoRect` = 8
- `lUseSense` = 4
- `lNoNilHilite` = 2

**anonymous**:

- `lInitMsg` = 0
- `lDrawMsg` = 1
- `lHiliteMsg` = 2
- `lCloseMsg` = 3

## Structs

- **ListRec** { rView: Rect, port: GrafPtr, indent: Point, cellSize: Point, visible: Rect, vScroll: ControlHandle, hScroll: ControlHandle, selFlags: SignedByte, lActive: Boolean, lReserved: SignedByte, listFlags: SignedByte, clikTime: LONGINT, clikLoc: Point, mouseLoc: Point, lClikLoop: ListClickLoopUPP, lastClick: Cell, refCon: LONGINT, listDefProc: Handle, userHandle: Handle, dataBounds: Rect, cells: DataHandle, maxIndex: INTEGER, cellArray: INTEGER[1] }

## Function Pointers

- **ListClickLoopUPP** () -> Boolean

## Dispatchers

- **Pack0**—

