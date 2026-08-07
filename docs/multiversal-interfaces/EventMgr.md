# EventMgr Interfaces

#define networkMask	1024

Source: `multiversal/defs/EventMgr.yaml`

- Functions: **0**
- Typedefs: **0**
- Structs: **1**, Unions: **0**
- Enums: **6**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **0**
- Low-memory globals: **1**

## Enums

- **?**
- **?**
- **?**
- **?** — #define networkMask	1024
- **?**
- **?**

### Enum Values

**anonymous**:

- `nullEvent` = 0
- `mouseDown` = 1
- `mouseUp` = 2
- `keyDown` = 3
- `keyUp` = 4
- `autoKey` = 5
- `updateEvt` = 6
- `diskEvt` = 7
- `activateEvt` = 8
- `networkEvt` = 10
- `driverEvt` = 11
- `app1Evt` = 12
- `app2Evt` = 13
- `app3Evt` = 14
- `app4Evt` = 15
- `kHighLevelEvent` = 23

**anonymous**:

- `charCodeMask` = 255
- `keyCodeMask` = 65280

**anonymous**:

- `mDownMask` = 2
- `mUpMask` = 4
- `keyDownMask` = 8
- `keyUpMask` = 16
- `autoKeyMask` = 32
- `updateMask` = 64
- `diskMask` = 128
- `activMask` = 256

**anonymous** — #define networkMask	1024:

- `highLevelEventMask` = 1024
- `driverMask` = 2048
- `app1Mask` = 4096
- `app2Mask` = 8192
- `app3Mask` = 16384
- `app4Mask` = -32768
- `everyEvent` = -1

**anonymous**:

- `activeFlag` = 1
- `changeFlag` = 2
- `btnState` = 128
- `cmdKey` = 256
- `shiftKey` = 512
- `alphaLock` = 1024
- `optionKey` = 2048
- `ControlKey` = 4096

**anonymous**:

- `rightShiftKey` = 8192
- `rightOptionKey` = 16384
- `rightControlKey` = 32768

## Structs

- **EventRecord** { what: INTEGER, message: LONGINT, when: LONGINT, where: Point, modifiers: INTEGER }

## Low Memory Globals

- **KeyMap** @ 0x174 (uint8_t[16]) — was LONGINT KeypadMap[2]; EventMgr SysEqu.a (true-b);
