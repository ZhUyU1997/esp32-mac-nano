# SysErr Interfaces

SysErr IMII-359 (true);

Source: `multiversal/defs/SysErr.yaml`

- Functions: **1**
- Typedefs: **0**
- Structs: **0**, Unions: **0**
- Enums: **1**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **0**
- Low-memory globals: **4**

## Functions

### SysError  

```c
void SysError(int16_t errorcode)
```

Trap: `0xA9C9` executor=C_

## Enums

- **exist_enum_t**

### Enum Values

**exist_enum_t**:

- `EXIST_YES` = 0
- `EXIST_NO` = 255

## Low Memory Globals

- **DSAlertTab** @ 0x2BA (Ptr) — SysErr IMII-359 (true);
- **DSAlertRect** @ 0x3F8 (Rect) — SysErr IMII-362 (true);
- **WWExist** @ 0x8F2 (Byte) — SysError SysEqu.a (true);
- **QDExist** @ 0x8F3 (Byte) — SysError SysEqu.a (true);
