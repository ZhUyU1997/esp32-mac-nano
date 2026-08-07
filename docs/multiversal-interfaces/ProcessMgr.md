# ProcessMgr Interfaces

chosen from /dev/random

Source: `multiversal/defs/ProcessMgr.yaml`

- Functions: **9**
- Typedefs: **3**
- Structs: **4**, Unions: **0**
- Enums: **7**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **0**
- Low-memory globals: **0**

## Functions

### GetCurrentProcess  

```c
OSErr GetCurrentProcess(ProcessSerialNumber* serial_number)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetNextProcess  

```c
OSErr GetNextProcess(ProcessSerialNumber* serial_number)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetPortNameFromProcessSerialNumber  

```c
OSErr GetPortNameFromProcessSerialNumber(PPCPortPtr port_name, ProcessSerialNumber* serial_number)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetProcessInformation  

```c
OSErr GetProcessInformation(ProcessSerialNumber* serial_number, ProcessInfoPtr info)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetProcessSerialNumberFromPortName  

```c
OSErr GetProcessSerialNumberFromPortName(PPCPortPtr port_name, ProcessSerialNumber* serial_number)
```

Trap: — (executor 实现，无 trap) executor=C_

### LaunchApplication  

```c
OSErr LaunchApplication(LaunchParamBlockRec* params)
```

Trap: `0xA9F2` executor=True

### SameProcess  

```c
OSErr SameProcess(ProcessSerialNumber* serial_number0, ProcessSerialNumber* serial_number1, Boolean* same_out)
```

Trap: — (executor 实现，无 trap) executor=C_

### SetFrontProcess  

```c
OSErr SetFrontProcess(ProcessSerialNumber* serial_number)
```

Trap: — (executor 实现，无 trap) executor=C_

### WakeUpProcess  

```c
OSErr WakeUpProcess(ProcessSerialNumber* serial_number)
```

Trap: — (executor 实现，无 trap) executor=C_

## Typedefs

- **LaunchFlags** = INTEGER
- **AppParametersPtr** = ROMlib_AppParameters_t*
- **ProcessInfoPtr** = ProcessInfoRec*

## Enums

- **?** — chosen from /dev/random
- **?**
- **?**
- **?**
- **?** — flags for the `processMode' field of the `ProcessInformationRec' record
- **?**
- **?**

### Enum Values

**anonymous** — chosen from /dev/random:

- `APP_PARAMS_MAGIC` = 3594734107

**anonymous**:

- `extendedBlock` = 19523

**anonymous**:

- `extendedBlockLen` = sizeof(LaunchParamBlockRec) - 12

**anonymous**:

- `launchContinue` = 16384

**anonymous** — flags for the `processMode' field of the `ProcessInformationRec' record:

- `modeDeskAccessory` = 131072
- `modeMultiLaunch` = 65536
- `modeNeedSuspendResume` = 16384
- `modeCanBackground` = 4096
- `modeDoesActivateOnFGSwitch` = 2048
- `modeOnlyBackground` = 1024
- `modeGetFrontClicks` = 512
- `modeGetAppDiedMsg` = 256
- `mode32BitCompatible` = 128
- `modeHighLevelEventAware` = 64
- `modeLocalAndRemoteHLEvents` = 32
- `modeStationeryAware` = 16
- `modeUseTextEditServices` = 8

**anonymous**:

- `kNoProcess` = 0
- `kSystemProcess` = 1
- `kCurrentProcess` = 2

**anonymous**:

- `procNotFound` = -600

## Structs

- **ProcessSerialNumber** { highLongOfPSN: uint32_t, lowLongOfPSN: uint32_t }
- **ROMlib_AppParameters_t** { magic: uint32_t, n_fsspec: INTEGER, fsspec: FSSpec[0] }
- **LaunchParamBlockRec** { reserved1: LONGINT, reserved2: INTEGER, launchBlockID: INTEGER, launchEPBLength: LONGINT, launchFileFlags: INTEGER, launchControlFlags: LaunchFlags, launchAppSpec: FSSpecPtr, launchProcessSN: ProcessSerialNumber, launchPreferredSize: LONGINT, launchMinimumSize: LONGINT, launchAvailableSize: LONGINT, launchAppParameters: AppParametersPtr }
- **ProcessInfoRec** { processInfoLength: uint32_t, processName: StringPtr, processNumber: ProcessSerialNumber, processType: uint32_t, processSignature: OSType, processMode: uint32_t, processLocation: Ptr, processSize: uint32_t, processFreeMem: uint32_t, processLauncher: ProcessSerialNumber, processLaunchDate: uint32_t, processActiveTime: uint32_t, processAppSpec: FSSpecPtr }

