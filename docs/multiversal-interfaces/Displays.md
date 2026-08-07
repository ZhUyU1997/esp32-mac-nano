# Displays Interfaces

Source: `multiversal/defs/Displays.yaml`

- Functions: **1**
- Typedefs: **0**
- Structs: **0**, Unions: **0**
- Enums: **0**
- Function pointers: **1**
- Common blocks: **0**
- Dispatchers: **1**
- Low-memory globals: **0**

## Functions

### DMRegisterNotifyProc  

```c
OSErr DMRegisterNotifyProc(DMNotificationUPP proc, ProcessSerialNumber* psn)
```

Trap: — (executor 实现，无 trap) executor=C_

## Function Pointers

- **DMNotificationUPP** (theEvent: AppleEvent*) -> void

## Dispatchers

- **DisplayDispatch**—

