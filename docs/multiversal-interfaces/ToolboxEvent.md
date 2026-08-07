# ToolboxEvent Interfaces

ToolboxEvent IMI-246 (true);

Source: `multiversal/defs/ToolboxEvent.yaml`

- Functions: **12**
- Typedefs: **0**
- Structs: **0**, Unions: **0**
- Enums: **0**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **0**
- Low-memory globals: **7**

## Functions

### Button  

```c
Boolean Button()
```

Trap: `0xA974` executor=C_

### EventAvail  

```c
Boolean EventAvail(INTEGER em, EventRecord* evt)
```

Trap: `0xA971` executor=C_

### GetCaretTime  

```c
LONGINT GetCaretTime()
```

Trap: — (executor 实现，无 trap)

### GetDblTime  

```c
LONGINT GetDblTime()
```

Trap: — (executor 实现，无 trap)

### GetKeys  

```c
void GetKeys(uint8_t* keys)
```

Trap: `0xA976` executor=C_

### GetMouse  

```c
void GetMouse(Point* p)
```

Trap: `0xA972` executor=C_

### GetNextEvent  

```c
Boolean GetNextEvent(INTEGER em, EventRecord* evt)
```

Trap: `0xA970` executor=C_

### KeyTranslate  

```c
uint32_t KeyTranslate(Ptr mapp, uint16_t code, uint32_t* state)
```

Trap: `0xA9C3` executor=C_

### StillDown  

```c
Boolean StillDown()
```

Trap: `0xA973` executor=C_

### TickCount  

```c
ULONGINT TickCount()
```

Trap: `0xA975` executor=C_

### WaitMouseUp  

```c
Boolean WaitMouseUp()
```

Trap: `0xA977` executor=C_

### WaitNextEvent  

```c
Boolean WaitNextEvent(INTEGER mask, EventRecord* evp, LONGINT sleep, RgnHandle mousergn)
```

Trap: `0xA860` executor=C_

## Low Memory Globals

- **KeyThresh** @ 0x18E (INTEGER) — ToolboxEvent IMI-246 (true);
- **KeyRepThresh** @ 0x190 (INTEGER) — ToolboxEvent IMI-246 (true);
- **DoubleTime** @ 0x2F0 (LONGINT) — ToolboxEvent IMI-260 (true);
- **CaretTime** @ 0x2F4 (LONGINT) — ToolboxEvent IMI-260 (true);
- **ScrDmpEnb** @ 0x2F8 (Byte) — ToolboxEvent IMI-258 (true);
- **JournalFlag** @ 0x8DE (INTEGER) — ToolboxEvent IMI-261 (false);
- **JournalRef** @ 0x8E8 (INTEGER) — ToolboxEvent IMI-261 (false);
