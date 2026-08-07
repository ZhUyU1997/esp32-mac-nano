# MixedMode Interfaces

Executor's version does not declare variable arguments. We get them directly from the PowerPC stack instead.

Source: `multiversal/defs/MixedMode.yaml`

- Functions: **7**
- Typedefs: **7**
- Structs: **3**, Unions: **0**
- Enums: **11**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **1**
- Low-memory globals: **0**

## Functions

### CallUniversalProc  

```c
LONGINT CallUniversalProc(UniversalProcPtr theProcPtr, ProcInfoType procInfo, ... ?)
```

Trap: — (executor 实现，无 trap)

### CallUniversalProc  

```c
LONGINT CallUniversalProc(UniversalProcPtr theProcPtr, ProcInfoType procInfo)
```

Trap: — (executor 实现，无 trap) executor=C_ — Executor's version does not declare variable arguments. We get them directly from the PowerPC stack instead.

### DisposeRoutineDescriptor  

```c
void DisposeRoutineDescriptor(UniversalProcPtr ptr)
```

Trap: — (executor 实现，无 trap) executor=C_

### NewFatRoutineDescriptor  

```c
UniversalProcPtr NewFatRoutineDescriptor(ProcPtr m68k, ProcPtr ppc, ProcInfoType info)
```

Trap: — (executor 实现，无 trap) executor=C_

### NewRoutineDescriptor  

```c
UniversalProcPtr NewRoutineDescriptor(ProcPtr proc, ProcInfoType info, ISAType isa)
```

Trap: — (executor 实现，无 trap) executor=C_

### RestoreMixedModeState  

```c
OSErr RestoreMixedModeState(void* statep, uint32_t vers)
```

Trap: — (executor 实现，无 trap) executor=C_

### SaveMixedModeState  

```c
OSErr SaveMixedModeState(void* statep, uint32_t vers)
```

Trap: — (executor 实现，无 trap) executor=C_

## Typedefs

- **ISAType** = uint8_t
- **CallingConventionType** = uint16_t
- **ProcInfoType** = uint32_t
- **RegisterSelectorType** = uint16_t
- **RoutineFlagsType** = uint16_t
- **RDFlagsType** = uint8_t
- **UniversalProcPtr** = RoutineDescriptor*

## Enums

- **?**
- **?**
- **?**
- **?**
- **?**
- **?**
- **?**
- **?**
- **?**
- **?**
- **?**

### Enum Values

**anonymous**:

- `kM68kISA` = 0
- `kPowerPCISA` = 1

**anonymous**:

- `kPascalStackBased` = ?
- `kCStackBased` = ?
- `kRegisterBased` = ?
- `kThinkCStackBased` = 5
- `kD0DispatchedPascalStackBased` = 8
- `kD0DispatchedCStackBased` = 9
- `kD1DispatchedPascalStackBased` = 12
- `kStackDispatchedPascalStackBased` = 14
- `kSpecialCase` = ?

**anonymous**:

- `MIXED_MODE_TRAP` = 43774

**anonymous**:

- `kRoutineDescriptorVersion` = 7

**anonymous**:

- `kSelectorsAreNotIndexable` = 0

**anonymous**:

- `kNoByteCode` = ?
- `kOneByteCode` = ?
- `kTwoByteCode` = ?
- `kFourByteCode` = ?

**anonymous**:

- `kCallingConventionWidth` = 4

**anonymous**:

- `kStackParameterPhase` = 6

**anonymous**:

- `kStackParameterWidth` = 2

**anonymous**:

- `kResultSizeWidth` = 2

**anonymous**:

- `kRegisterD0` = 0
- `kRegisterD1` = ?
- `kRegisterD2` = ?
- `kRegisterD3` = ?
- `kRegisterA0` = ?
- `kRegisterA1` = ?
- `kRegisterA2` = ?
- `kRegisterA3` = ?
- `kRegisterD4` = ?
- `kRegisterD5` = ?
- `kRegisterD6` = ?
- `kREgisterD7` = ?
- `kRegisterA4` = ?
- `kRegisterA5` = ?
- `kRegisterA6` = ?
- `kCCRegisterCBit` = 16
- `kCCRegisterVBit` = ?
- `kCCRegisterZBit` = ?
- `kCCRegisterNBit` = ?
- `kCCRegisterXBit` = ?

## Structs

- **PPCProcDescriptor** { code: uint32_t, rtoc: uint32_t }
- **RoutineRecord** { procInfo: ProcInfoType, reserved1: uint8_t, ISA: ISAType, routineFlags: RoutineFlagsType, procDescriptor: ProcPtr, reserved2: uint32_t, selector: uint32_t }
- **RoutineDescriptor** { goMixedModeTrap: uint16_t, version: uint8_t, routineDescriptorFlags: RDFlagsType, reserved1: uint32_t, reserved2: uint8_t, selectorInfo: uint8_t, routineCount: uint16_t, routineRecords: RoutineRecord[1] }

## Dispatchers

- **MixedModeDispatch**—

