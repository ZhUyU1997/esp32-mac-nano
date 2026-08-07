# ToolboxUtil Interfaces

Source: `multiversal/defs/ToolboxUtil.yaml`

- Functions: **41**
- Typedefs: **2**
- Structs: **1**, Unions: **0**
- Enums: **2**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **0**
- Low-memory globals: **0**

## Functions

### AngleFromSlope  

```c
INTEGER AngleFromSlope(Fixed s)
```

Trap: `0xA8C4` executor=C_

### BitAnd  

```c
LONGINT BitAnd(LONGINT a, LONGINT b)
```

Trap: `0xA858` executor=C_

### BitClr  

```c
void BitClr(Ptr bp, LONGINT bn)
```

Trap: `0xA85F` executor=C_

### BitNot  

```c
LONGINT BitNot(LONGINT a)
```

Trap: `0xA85A` executor=C_

### BitOr  

```c
LONGINT BitOr(LONGINT a, LONGINT b)
```

Trap: `0xA85B` executor=C_

### BitSet  

```c
void BitSet(Ptr bp, LONGINT bn)
```

Trap: `0xA85E` executor=C_

### BitShift  

```c
LONGINT BitShift(LONGINT a, INTEGER n)
```

Trap: `0xA85C` executor=C_

### BitTst  

```c
Boolean BitTst(Ptr bp, LONGINT bn)
```

Trap: `0xA85D` executor=C_

### BitXor  

```c
LONGINT BitXor(LONGINT a, LONGINT b)
```

Trap: `0xA859` executor=C_

### DeltaPoint  

```c
LONGINT DeltaPoint(Point a, Point b)
```

Trap: `0xA94F` executor=C_

### Fix2Frac  

```c
Fract Fix2Frac(Fixed x)
```

Trap: `0xA841` executor=C_

### Fix2Long  

```c
LONGINT Fix2Long(Fixed x)
```

Trap: `0xA840` executor=C_

### FixATan2  

```c
Fixed FixATan2(LONGINT x, LONGINT y)
```

Trap: `0xA818` executor=C_

### FixDiv  

```c
Fixed FixDiv(Fixed x, Fixed y)
```

Trap: `0xA84D` executor=C_

### FixMul  

```c
Fixed FixMul(Fixed a, Fixed b)
```

Trap: `0xA868` executor=C_

### FixRatio  

```c
Fixed FixRatio(INTEGER n, INTEGER d)
```

Trap: `0xA869` executor=C_

### FixRound  

```c
INTEGER FixRound(Fixed x)
```

Trap: `0xA86C` executor=C_

### Frac2Fix  

```c
Fixed Frac2Fix(Fract x)
```

Trap: `0xA842` executor=C_

### FracCos  

```c
Fract FracCos(Fixed x)
```

Trap: `0xA847` executor=C_

### FracDiv  

```c
Fract FracDiv(Fract x, Fract y)
```

Trap: `0xA84B` executor=C_

### FracMul  

```c
Fract FracMul(Fract x, Fract y)
```

Trap: `0xA84A` executor=C_

### FracSin  

```c
Fract FracSin(Fixed x)
```

Trap: `0xA848` executor=C_

### FracSqrt  

```c
Fract FracSqrt(Fract x)
```

Trap: `0xA849` executor=C_

### GetCursor  

```c
CursHandle GetCursor(INTEGER id)
```

Trap: `0xA9B9` executor=C_

### GetIndPattern  

```c
void GetIndPattern(Byte* op, INTEGER plistid, INTEGER index)
```

Trap: — (executor 实现，无 trap)

### GetIndString  

```c
void GetIndString(StringPtr s, INTEGER sid, INTEGER index)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetPattern  

```c
PatHandle GetPattern(INTEGER id)
```

Trap: `0xA9B8` executor=C_

### GetPicture  

```c
PicHandle GetPicture(INTEGER id)
```

Trap: `0xA9BC` executor=C_

### GetString  

```c
StringHandle GetString(INTEGER i)
```

Trap: `0xA9BA` executor=C_

### HiWord  

```c
INTEGER HiWord(LONGINT a)
```

Trap: `0xA86A` executor=C_

### LoWord  

```c
INTEGER LoWord(LONGINT a)
```

Trap: `0xA86B` executor=C_

### Long2Fix  

```c
Fixed Long2Fix(LONGINT x)
```

Trap: `0xA83F` executor=C_

### LongMul  

```c
void LongMul(LONGINT a, LONGINT b, Int64Bit* c)
```

Trap: `0xA867` executor=C_

### Munger  

```c
LONGINT Munger(Handle h, LONGINT off, Ptr p1, LONGINT len1, Ptr p2, LONGINT len2)
```

Trap: `0xA9E0` executor=C_

### NewString  

```c
StringHandle NewString(ConstStringPtr s)
```

Trap: `0xA906` executor=C_

### PackBits  

```c
void PackBits(Ptr* sp, Ptr* dp, INTEGER len)
```

Trap: `0xA8CF` executor=C_

### R_X2Fix  

```c
Fixed R_X2Fix(extended80* x)
```

Trap: `0xA844` executor=C_

### R_X2Frac  

```c
Fract R_X2Frac(extended80* x)
```

Trap: `0xA846` executor=C_

### SetString  

```c
void SetString(StringHandle h, ConstStringPtr s)
```

Trap: `0xA907` executor=C_

### SlopeFromAngle  

```c
Fixed SlopeFromAngle(INTEGER a)
```

Trap: `0xA8BC` executor=C_

### UnpackBits  

```c
void UnpackBits(Ptr* sp, Ptr* dp, INTEGER len)
```

Trap: `0xA8D0` executor=C_

## Typedefs

- **PatPtr** = Pattern*
- **PatHandle** = PatPtr*

## Enums

- **?**
- **?**

### Enum Values

**anonymous**:

- `sysPatListID` = 0

**anonymous**:

- `iBeamCursor` = 1
- `crossCursor` = 2
- `plusCursor` = 3
- `watchCursor` = 4

## Structs

- **Int64Bit** { hiLong: LONGINT, loLong: LONGINT }

