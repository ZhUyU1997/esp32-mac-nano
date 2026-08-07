# TimeMgr Interfaces

Source: `multiversal/defs/TimeMgr.yaml`

- Functions: **4**
- Typedefs: **0**
- Structs: **1**, Unions: **0**
- Enums: **0**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **0**
- Low-memory globals: **0**

## Functions

### InsTime  

```c
void InsTime(QElemPtr taskp)
```

Trap: `0xA058` executor=True

### InsXTime  

```c
void InsXTime(QElemPtr taskp)
```

Trap: `0xA458`

### PrimeTime  

```c
void PrimeTime(QElemPtr taskp, LONGINT count)
```

Trap: `0xA05A` executor=True

### RmvTime  

```c
void RmvTime(QElemPtr taskp)
```

Trap: `0xA059` executor=True

## Structs

- **TMTask** { qLink: QElemPtr, qType: INTEGER, tmAddr: ProcPtr, tmCount: LONGINT, tmWakeUp: LONGINT, tmReserved: LONGINT }

