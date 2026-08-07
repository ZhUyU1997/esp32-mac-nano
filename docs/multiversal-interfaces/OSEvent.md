# OSEvent Interfaces

OSEvent SysEqu.a (true-b);

Source: `multiversal/defs/OSEvent.yaml`

- Functions: **14**
- Typedefs: **3**
- Structs: **4**, Unions: **0**
- Enums: **4**
- Function pointers: **1**
- Common blocks: **0**
- Dispatchers: **0**
- Low-memory globals: **8**

## Functions

### AcceptHighLevelEvent  

```c
OSErr AcceptHighLevelEvent(TargetID* sender_id_return, int32_t* refcon_return, Ptr msg_buf, int32_t* msg_length_return)
```

Trap: — (executor 实现，无 trap) executor=C_

### FlushEvents  

```c
void FlushEvents(INTEGER evmask, INTEGER stopmask)
```

Trap: `0xA032`

### FlushEvents  

```c
void FlushEvents(INTEGER evmask, INTEGER stopmask)
```

Trap: `0xA032` executor=True

### GetEvQHdr  

```c
QHdrPtr GetEvQHdr()
```

Trap: — (executor 实现，无 trap)

### GetOSEvent  

```c
Boolean GetOSEvent(INTEGER evmask, EventRecord* eventp)
```

Trap: `0xA031`

### GetOSEvent  

```c
Boolean GetOSEvent(INTEGER evmask, EventRecord* eventp)
```

Trap: `0xA031` executor=True

### GetSpecificHighLevelEvent  

```c
Boolean GetSpecificHighLevelEvent(GetSpecificFilterUPP fn, Ptr data, OSErr* err_return)
```

Trap: — (executor 实现，无 trap) executor=C_

### OSEventAvail  

```c
Boolean OSEventAvail(INTEGER evmask, EventRecord* eventp)
```

Trap: `0xA030`

### OSEventAvail  

```c
Boolean OSEventAvail(INTEGER evmask, EventRecord* eventp)
```

Trap: `0xA030` executor=True

### PPostEvent  

```c
OSErr PPostEvent(INTEGER evcode, LONGINT evmsg, EvQElPtr* qelp)
```

Trap: — (executor 实现，无 trap)

### PostEvent  

```c
OSErr PostEvent(INTEGER evcode, LONGINT evmsg)
```

Trap: — (executor 实现，无 trap)

### PostHighLevelEvent  

```c
OSErr PostHighLevelEvent(EventRecord* evt, Ptr receiver_id, int32_t refcon, Ptr msg_buf, int32_t msg_length, int32_t post_options)
```

Trap: — (executor 实现，无 trap) executor=C_

### SetEventMask  

```c
void SetEventMask(INTEGER evmask)
```

Trap: — (executor 实现，无 trap)

### geteventelem  

```c
EvQEl* geteventelem()
```

Trap: — (executor 实现，无 trap)

## Typedefs

- **EvQElPtr** = EvQEl*
- **TargetIDPtr** = TargetID
- **HighLevelEventMsgPtr** = HighLevelEventMsg*

## Enums

- **?**
- **?**
- **SZ_t**
- **?**

### Enum Values

**anonymous**:

- `evtNotEnb` = 1

**anonymous**:

- `osEvt` = 15
- `SUSPENDRESUMEBITS` = 16777216
- `SUSPEND` = 0 << 0
- `RESUME` = 1 << 0
- `CONVERTCLIPBOARD` = 1 << 1
- `mouseMovedMessage` = 250

**SZ_t**:

- `SZreserved0` = 1 << 15
- `SZacceptSuspendResumeEvents` = 1 << 14
- `SZreserved1` = 1 << 13
- `SZcanBackground` = 1 << 12
- `SZdoesActivateOnFGSwitch` = 1 << 11
- `SZonlyBackground` = 1 << 10
- `SZgetFrontClicks` = 1 << 9
- `SZAcceptAppDiedEvents` = 1 << 8
- `SZis32BitCompatible` = 1 << 7
- `SZisHighLevelEventAware` = 1 << 6
- `SZlocalAndRemoveHLEvents` = 1 << 5
- `SZisStationeryAware` = 1 << 4
- `SZuseTextEditServices` = 1 << 3
- `SZreserved2` = 1 << 2
- `SZreserved3` = 1 << 1
- `SZreserved4` = 1 << 0

**anonymous**:

- `noOutstandingHLE` = -607
- `bufferIsSmall` = -608

## Structs

- **EvQEl** { qLink: QElemPtr, qType: INTEGER, evtQWhat: INTEGER, evtQMessage: LONGINT, evtQWhen: LONGINT, evtQWhere: Point, evtQModifiers: INTEGER }
- **SIZEResource** { size_flags: int16_t, preferred_size: int32_t, minimum_size: int32_t }
- **TargetID** { sessionID: int32_t, name: PPCPortRec, location: LocationNameRec, recvrName: PPCPortRec }
- **HighLevelEventMsg** { HighLevelEventMsgHeaderlength: int16_t, version: int16_t, reserved1: int32_t, theMsgEvent: EventRecord, userRefCon: int32_t, postingOptions: int32_t, msgLength: int32_t }

## Function Pointers

- **GetSpecificFilterUPP** (?: Ptr, ?: HighLevelEventMsgPtr, ?: TargetID*) -> Boolean

## Low Memory Globals

- **monkeylives** @ 0x100 (INTEGER) — OSEvent SysEqu.a (true-b);
- **SysEvtMask** @ 0x144 (INTEGER) — OSEvent IMII-70 (true);
- **EventQueue** @ 0x14A (QHdr) — OSEvent IMII-71 (true);
- **Ticks** @ 0x16A (ULONGINT) — OSEvent IMI-260 (true);
- **MBState** @ 0x172 (Byte) — EventMgr PegLeg (True-b);
- **MTemp** @ 0x828 (Point) — QuickDraw PegLeg (True-b);
- **MouseLocation** @ 0x82C (Point) — QuickDraw Vamp (true);
- **MouseLocation2** @ 0x830 (Point) — QuickDraw MacAttack (true);
