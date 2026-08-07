# Serial Interfaces

Serial driver control codes

Source: `multiversal/defs/Serial.yaml`

- Functions: **9**
- Typedefs: **1**
- Structs: **2**, Unions: **0**
- Enums: **11**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **0**
- Low-memory globals: **0**

## Functions

### RAMSDClose  

```c
void RAMSDClose(SPortSel port)
```

Trap: — (executor 实现，无 trap) executor=True

### RAMSDOpen  

```c
OSErr RAMSDOpen(SPortSel port)
```

Trap: — (executor 实现，无 trap) executor=True

### SerClrBrk  

```c
OSErr SerClrBrk(INTEGER rn)
```

Trap: — (executor 实现，无 trap) executor=True

### SerGetBuf  

```c
OSErr SerGetBuf(INTEGER rn, LONGINT* lp)
```

Trap: — (executor 实现，无 trap) executor=True

### SerHShake  

```c
OSErr SerHShake(INTEGER rn, const SerShk* flags)
```

Trap: — (executor 实现，无 trap) executor=True

### SerReset  

```c
OSErr SerReset(INTEGER rn, INTEGER config)
```

Trap: — (executor 实现，无 trap) executor=True

### SerSetBrk  

```c
OSErr SerSetBrk(INTEGER rn)
```

Trap: — (executor 实现，无 trap) executor=True

### SerSetBuf  

```c
OSErr SerSetBuf(INTEGER rn, Ptr p, INTEGER len)
```

Trap: — (executor 实现，无 trap) executor=True

### SerStatus  

```c
OSErr SerStatus(INTEGER rn, SerStaRec* serstap)
```

Trap: — (executor 实现，无 trap) executor=True

## Typedefs

- **SPortSel** = SignedByte

## Enums

- **?**
- **?**
- **?**
- **?**
- **?**
- **?**
- **?**
- **?**
- **?** — Serial driver control codes
- **?** — Serial driver status codes
- **?**

### Enum Values

**anonymous**:

- `baud300` = 380
- `baud600` = 189
- `baud1200` = 94
- `baud1800` = 62
- `baud2400` = 46
- `baud3600` = 30
- `baud4800` = 22
- `baud7200` = 14
- `baud9600` = 10
- `baud14400` = 6
- `baud19200` = 4
- `baud28800` = 2
- `baud38400` = 1
- `baud57600` = 0

**anonymous**:

- `stop10` = 16384
- `stop15` = -32768
- `stop20` = -16384

**anonymous**:

- `noParity` = 0
- `oddParity` = 4096
- `evenParity` = 12288

**anonymous**:

- `data5` = 0
- `data6` = 2048
- `data7` = 1024
- `data8` = 3072

**anonymous**:

- `swOverrunErr` = 1
- `parityErr` = 16
- `hwOverrunErr` = 32
- `framingErr` = 64

**anonymous**:

- `ctsEvent` = 32
- `breakEvent` = 128

**anonymous**:

- `xOffWasSent` = 128

**anonymous**:

- `sPortA` = 0
- `sPortB` = 1

**anonymous** — Serial driver control codes:

- `kSERDConfiguration` = 8
- `kSERDInputBuffer` = 9
- `kSERDSerHShake` = 10
- `kSERDClearBreak` = 11
- `kSERDSetBreak` = 12
- `kSERDBaudRate` = 13
- `kSERDHandshake` = 14
- `kSERDClockMIDI` = 15
- `kSERDMiscOptions` = 16
- `kSERDAssertDTR` = 17
- `kSERDNegateDTR` = 18
- `kSERDSetPEChar` = 19
- `kSERDSetPEAltChar` = 20
- `kSERDSetXOffFlag` = 21
- `kSERDClearXOffFlag` = 22
- `kSERDSendXOn` = 23
- `kSERDSendXOnOut` = 24
- `kSERDSendXOff` = 25
- `kSERDSendXOffOut` = 26
- `kSERDResetChannel` = 27
- `kSERDHandshakeRS232` = 28
- `kSERDStickParity` = 29
- `kSERDAssertRTS` = 30
- `kSERDNegateRTS` = 31
- `kSERD115KBaud` = 115
- `kSERD230KBaud` = 230

**anonymous** — Serial driver status codes:

- `kSERDInputCount` = 2
- `kSERDStatus` = 8
- `kSERDVersion` = 9
- `kSERDGetDCD` = 256

**anonymous**:

- `MODEMIRNUM` = -6
- `MODEMORNUM` = -7
- `PRNTRIRNUM` = -8
- `PRNTRORNUM` = -9

## Structs

- **SerShk** { fXOn: Byte, fCTS: Byte, xOn: Byte, xOff: Byte, errs: Byte, evts: Byte, fInX: Byte, null: Byte }
- **SerStaRec** { cumErrs: Byte, xOffSent: Byte, rdPend: Byte, wrPend: Byte, ctsHold: Byte, xOffHold: Byte, dsrHold: Byte, modemStatus: Byte }

