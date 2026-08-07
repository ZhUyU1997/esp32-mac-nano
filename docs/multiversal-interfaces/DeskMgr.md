# DeskMgr Interfaces

DeskMgr IMI-443 (false);

Source: `multiversal/defs/DeskMgr.yaml`

- Functions: **7**
- Typedefs: **0**
- Structs: **0**, Unions: **0**
- Enums: **2**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **0**
- Low-memory globals: **1**

## Functions

### CloseDeskAcc  

```c
void CloseDeskAcc(INTEGER rn)
```

Trap: `0xA9B7` executor=C_

### OpenDeskAcc  

```c
INTEGER OpenDeskAcc(ConstStringPtr acc)
```

Trap: `0xA9B6` executor=C_

### SystemClick  

```c
void SystemClick(EventRecord* evp, WindowPtr wp)
```

Trap: `0xA9B3` executor=C_

### SystemEdit  

```c
Boolean SystemEdit(INTEGER editcmd)
```

Trap: `0xA9C2` executor=C_

### SystemEvent  

```c
Boolean SystemEvent(EventRecord* evp)
```

Trap: `0xA9B2` executor=C_

### SystemMenu  

```c
void SystemMenu(LONGINT menu)
```

Trap: `0xA9B5` executor=C_

### SystemTask  

```c
void SystemTask()
```

Trap: `0xA9B4` executor=C_

## Enums

- **?**
- **?**

### Enum Values

**anonymous**:

- `undoCmd` = 0
- `cutCmd` = 2
- `copyCmd` = 3
- `pasteCmd` = 4
- `clearCmd` = 5

**anonymous**:

- `accEvent` = 64
- `accRun` = 65
- `accMenu` = 67
- `accUndo` = 68

## Low Memory Globals

- **SEvtEnb** @ 0x15C (Byte) — DeskMgr IMI-443 (false);
