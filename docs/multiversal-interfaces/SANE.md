# SANE Interfaces

For backwards compatibility with old stuff.

Source: `multiversal/defs/SANE.yaml`

- Functions: **48**
- Typedefs: **4**
- Structs: **5**, Unions: **0**
- Enums: **7**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **3**
- Low-memory globals: **0**

## Functions

### ROMlib_FX2x  

```c
void ROMlib_FX2x(extended80* sp, void* dp, uint16_t sel)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_FabsX  

```c
void ROMlib_FabsX(extended80* dp, uint16_t sel)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_Faddx  

```c
void ROMlib_Faddx(void* sp, extended80* dp, uint16_t sel)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_Fannuity  

```c
void ROMlib_Fannuity(extended80* sp2, extended80* sp, extended80* dp)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_FatanX  

```c
void ROMlib_FatanX(extended80* dp)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_Fclassx  

```c
void ROMlib_Fclassx(void* sp, INTEGER* dp, uint16_t sel)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_Fcmpx  

```c
void ROMlib_Fcmpx(void* sp, extended80* dp, uint16_t sel)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_Fcompound  

```c
void ROMlib_Fcompound(extended80* sp2, extended80* sp, extended80* dp)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_FcosX  

```c
void ROMlib_FcosX(extended80* dp)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_FcpXx  

```c
void ROMlib_FcpXx(void* sp, extended80* dp, uint16_t sel)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_Fcpysgnx  

```c
void ROMlib_Fcpysgnx(x80_t* sp, x80_t* dp, uint16_t sel)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_Fcstr2dec  

```c
void ROMlib_Fcstr2dec(Decstr sp2, INTEGER* sp, Decimal* dp2, Byte* dp)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_Fdec2str  

```c
void ROMlib_Fdec2str(DecForm* sp2, Decimal* sp, Decstr dp)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_Fdec2x  

```c
void ROMlib_Fdec2x(Decimal* sp, void* dp, uint16_t sel)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_Fdivx  

```c
void ROMlib_Fdivx(void* sp, extended80* dp, uint16_t sel)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_Fexp1X  

```c
void ROMlib_Fexp1X(extended80* dp)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_Fexp21X  

```c
void ROMlib_Fexp21X(extended80* dp)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_Fexp2X  

```c
void ROMlib_Fexp2X(extended80* dp)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_FexpX  

```c
void ROMlib_FexpX(extended80* dp)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_Fgetenv  

```c
void ROMlib_Fgetenv(INTEGER* dp, INTEGER sel)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_Fgethv  

```c
void ROMlib_Fgethv(LONGINT* hvp, uint16_t sel)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_Fln1X  

```c
void ROMlib_Fln1X(extended80* dp)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_FlnX  

```c
void ROMlib_FlnX(extended80* dp)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_Flog21X  

```c
void ROMlib_Flog21X(extended80* dp)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_Flog2X  

```c
void ROMlib_Flog2X(extended80* dp)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_FlogbX  

```c
void ROMlib_FlogbX(extended80* dp, uint16_t sel)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_Fmulx  

```c
void ROMlib_Fmulx(void* sp, extended80* dp, uint16_t sel)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_FnegX  

```c
void ROMlib_FnegX(extended80* dp, uint16_t sel)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_FnextX  

```c
void ROMlib_FnextX(uint8_t* x, uint8_t* y, uint16_t sel)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_Fprocentry  

```c
void ROMlib_Fprocentry(INTEGER* dp, INTEGER sel)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_Fprocexit  

```c
void ROMlib_Fprocexit(INTEGER* dp, INTEGER sel)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_Fpstr2dec  

```c
void ROMlib_Fpstr2dec(Decstr sp2, INTEGER* sp, Decimal* dp2, Byte* dp)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_FrandX  

```c
void ROMlib_FrandX(extended80* dp)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_Fremx  

```c
void ROMlib_Fremx(void* sp, extended80* dp, uint16_t sel)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_FrintX  

```c
void ROMlib_FrintX(extended80* dp, uint16_t sel)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_FscalbX  

```c
void ROMlib_FscalbX(INTEGER* sp, extended80* dp, uint16_t sel)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_Fsetenv  

```c
void ROMlib_Fsetenv(INTEGER* dp, INTEGER sel)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_Fsethv  

```c
void ROMlib_Fsethv(LONGINT* hvp, uint16_t sel)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_FsinX  

```c
void ROMlib_FsinX(extended80* dp)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_FsqrtX  

```c
void ROMlib_FsqrtX(extended80* dp, uint16_t sel)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_Fsubx  

```c
void ROMlib_Fsubx(void* sp, extended80* dp, uint16_t sel)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_FtanX  

```c
void ROMlib_FtanX(extended80* dp)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_Ftestxcp  

```c
void ROMlib_Ftestxcp(INTEGER* dp, INTEGER sel)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_FtintX  

```c
void ROMlib_FtintX(extended80* dp, uint16_t sel)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_Fx2X  

```c
void ROMlib_Fx2X(void* sp, extended80* dp, uint16_t sel)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_Fx2dec  

```c
void ROMlib_Fx2dec(DecForm* sp2, void* sp, Decimal* dp, uint16_t sel)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_Fxpwri  

```c
void ROMlib_Fxpwri(INTEGER* sp, extended80* dp)
```

Trap: — (executor 实现，无 trap) executor=C_

### ROMlib_Fxpwry  

```c
void ROMlib_Fxpwry(extended80* sp, extended80* dp)
```

Trap: — (executor 实现，无 trap) executor=C_

## Typedefs

- **extended80** = x80_t — For backwards compatibility with old stuff.
- **comp** = comp_t
- **DecFormStyle** = INTEGER
- **Decstr** = char*

## Enums

- **?**
- **toobigdecformstyle_t**
- **?**
- **NumClass**
- **?**
- **?**
- **?**

### Enum Values

**anonymous**:

- `SIGDIGLEN` = 20

**toobigdecformstyle_t**:

- `FloatDecimal` = ?
- `FixedDecimal` = 256

**anonymous**:

- `DECIMALTYPEMASK` = 256

**NumClass**:

- `SNaN` = 1
- `QNaN` = ?
- `Infinite` = ?
- `ZeroNum` = ?
- `NormalNum` = ?
- `DenormalNum` = ?

**anonymous**:

- `FX_OPERAND` = 0
- `FD_OPERAND` = 2048
- `FS_OPERAND` = 4096
- `FC_OPERAND` = 12288
- `FI_OPERAND` = 8192
- `FL_OPERAND` = 10240

**anonymous**:

- `Fx2X_OPCODE` = 14

**anonymous**:

- `FI2X` = FI_OPERAND | Fx2X_OPCODE

## Structs

- **comp_t** { val: int64_t } — Big-endian 64 bit "comp" data type.  Note that this has a NaN value! typedef union { struct { ULONGINT hi; ULONGINT lo; } hilo; signed long long val; } comp_t;
- **x80_t** { sgn_and_exp: uint16_t, mantissa: uint64_t } — "Packed" IEEE 80 bit FP representation (zero field omitted).
- **extended96** { exp: INTEGER, zero: INTEGER, man: INTEGER[4] }
- **Decimal** { sgn: uint8_t, unused_filler: uint8_t, exp: INTEGER, sig: uint8_t[SIGDIGLEN] }
- **DecForm** { style: DecFormStyle, digits: INTEGER }

## Dispatchers

- **Pack4**—
- **Pack5**—
- **Pack7**—

