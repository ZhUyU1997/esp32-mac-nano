# Gestalt Interfaces

gestaltHardwareAttr return values

Source: `multiversal/defs/Gestalt.yaml`

- Functions: **3**
- Typedefs: **0**
- Structs: **0**, Unions: **0**
- Enums: **17**
- Function pointers: **1**
- Common blocks: **0**
- Dispatchers: **1**
- Low-memory globals: **0**

## Functions

### Gestalt  

```c
OSErr Gestalt(OSType selector, LONGINT* responsep)
```

Trap: — (executor 实现，无 trap) executor=C_

### NewGestalt  

```c
OSErr NewGestalt(OSType selector, SelectorFunctionUPP selFunc)
```

Trap: — (executor 实现，无 trap) executor=C_

### ReplaceGestalt  

```c
OSErr ReplaceGestalt(OSType selector, SelectorFunctionUPP selFunc, SelectorFunctionUPP* oldSelFuncp)
```

Trap: — (executor 实现，无 trap) executor=C_

## Enums

- **?**
- **?**
- **?**
- **?** — gestaltHardwareAttr return values
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
- **?**
- **?**

### Enum Values

**anonymous**:

- `gestaltPhysicalRAMSize` = 'ram '
- `gestaltAddressingModeAttr` = 'addr'
- `gestaltAliasMgrAttr` = 'alis'
- `gestaltAppleEventsAttr` = 'evnt'
- `gestaltAppleTalkVersion` = 'atlk'
- `gestaltAUXVersion` = 'a/ux'
- `gestaltConnMgrAttr` = 'conn'
- `gestaltCRMAttr` = 'crm '
- `gestaltCTBVersion` = 'ctbv'
- `gestaltDBAccessMgrAttr` = 'dbac'
- `gestaltDITLExtAttr` = 'ditl'
- `gestaltEasyAccessAttr` = 'easy'
- `gestaltEditionMgrAttr` = 'edtn'
- `gestaltExtToolboxTable` = 'xttt'
- `gestaltFindFolderAttr` = 'fold'
- `gestaltFontMgrAttr` = 'font'
- `gestaltFPUType` = 'fpu '
- `gestaltFSAttr` = 'fs  '
- `gestaltFXfrMgrAttr` = 'fxfr'
- `gestaltHardwareAttr` = 'hdwr'
- `gestaltHelpMgrAttr` = 'help'
- `gestaltIconUtilitiesAttr` = 'icon'
- `gestaltKeyboardType` = 'kbd '
- `gestaltLogicalPageSize` = 'pgsz'
- `gestaltLogicalRAMSize` = 'lram'
- `gestaltLowMemorySize` = 'lmem'
- `gestaltMiscAttr` = 'misc'
- `gestaltMMUType` = 'mmu '
- `gestaltNotificatinMgrAttr` = 'nmgr'
- `gestaltNuBusConnectors` = 'sltc'
- `gestaltOSAttr` = 'os  '
- `gestaltOSTable` = 'ostt'
- `gestaltParityAttr` = 'prty'
- `gestaltPopupAttr` = 'pop!'
- `gestaltPowerMgrAttr` = 'powr'
- `gestaltPPCToolboxAttr` = 'ppc '
- `gestaltProcessorType` = 'proc'
- `gestaltQuickdrawVersion` = 'qd  '
- `gestaltQuickdrawFeatures` = 'qdrw'
- `gestaltResourceMgrAttr` = 'rsrc'
- `gestaltScriptCount` = 'scr#'
- `gestaltScriptMgrVersion` = 'scri'
- `gestaltSoundAttr` = 'snd '
- `gestaltSpeechAttr` = 'ttsc'
- `gestaltStandardFileAttr` = 'stdf'
- `gestaltStdNBPAttr` = 'nlup'
- `gestaltTermMgrAttr` = 'term'
- `gestaltTextEditVersion` = 'te  '
- `gestaltTimeMgrVersion` = 'tmgr'
- `gestaltToolboxTable` = 'tbbt'
- `gestaltVersion` = 'vers'
- `gestaltVMAttr` = 'vm  '
- `gestaltMachineIcon` = 'micn'
- `gestaltMachineType` = 'mach'
- `gestaltROMSize` = 'rom '
- `gestaltROMVersion` = 'romv'
- `gestaltSystemVersion` = 'sysv'
- `gestaltNativeCPUtype` = 'cput'
- `gestaltSysArchitecture` = 'sysa'

**anonymous**:

- `gestaltMacQuadra610` = 53
- `gestaltCPU68040` = 4
- `gestaltCPU601` = 257
- `gestaltCPU603` = 259
- `gestaltCPU604` = 260
- `gestaltCPU603e` = 262
- `gestaltCPU603ev` = 263
- `gestaltCPU750` = 264
- `gestaltCPU604e` = 265
- `gestaltCPU604ev` = 266
- `gestaltCPUG4` = 268
- `gestaltNoMMU` = 0
- `gestalt68k` = 1
- `gestaltPowerPC` = 2

**anonymous**:

- `gestalt32BitAddressing` = 0
- `gestalt32BitSysZone` = 1
- `gestalt32BitCapable` = 2

**anonymous** — gestaltHardwareAttr return values:

- `gestaltHasVIA1` = 0
- `gestaltHasVIA2` = 1
- `gestaltHasASC` = 3
- `gestaltHasSSC` = 4
- `gestaltHasSCI` = 7

**anonymous**:

- `gestaltEasyAccessOff` = 1 << 0
- `gestalt68881` = 1
- `gestaltMacKbd` = 1
- `gestalt68040MMu` = 4
- `gestalt68000` = 1
- `gestalt68040` = 5

**anonymous**:

- `gestaltOriginalQD` = 0
- `gestalt8BitQD` = 256
- `gestalt32BitQD` = 512
- `gestalt32BitQD11` = 528
- `gestalt32BitQD12` = 544
- `gestalt32BitQD13` = 560

**anonymous**:

- `gestaltHasColor` = 0
- `gestaltHasDeepGWorlds` = 1
- `gestaltHasDirectPixMaps` = 2
- `gestaltHasGrayishTextOr` = 3

**anonymous**:

- `gestaltTE1` = 1
- `gestaltTE2` = 2
- `gestaltTE3` = 3
- `gestaltTE4` = 4
- `gestaltTE5` = 5

**anonymous**:

- `gestaltDITLExtPresent` = 0

**anonymous**:

- `gestaltStandardTimeMgr` = 1
- `gestaltVMPresent` = 1 << 0

**anonymous**:

- `gestaltClassic` = 1
- `gestaltMacXL` = 2
- `gestaltMac512KE` = 3
- `gestaltMacPlus` = 4
- `gestaltMacSE` = 5
- `gestaltMacII` = 6
- `gestaltMacIIx` = 7
- `gestaltMacIIcx` = 8
- `gestaltMacSE30` = 9
- `gestaltPortable` = 10
- `gestaltMacIIci` = 11
- `gestaltMacIIfx` = 13
- `gestaltMacClassic` = 17
- `gestaltMacIIsi` = 18
- `gestaltMacLC` = 19

**anonymous**:

- `gestaltHasFSSpecCalls` = 1 << 1

**anonymous**:

- `gestaltStandardFile58` = 1 << 0

**anonymous**:

- `gestaltUndefSelectorErr` = -5551
- `gestaltUnknownErr` = -5550
- `gestaltDupSelectorErr` = -5552
- `gestaltLocationErr` = -5553

**anonymous**:

- `gestaltSerialAttr` = 'ser '
- `gestaltHasGPIaToDCDa` = 0
- `gestaltHasGPIaToRTxCa` = 1
- `gestaltHasGPIbToDCDb` = 2
- `gestaltHidePortA` = 3
- `gestaltHidePortB` = 4
- `gestaltPortADisabled` = 5
- `gestaltPortBDisabled` = 6

**anonymous**:

- `gestaltOpenTpt` = 'otan'

**anonymous**:

- `_Gestalt` = 41389

## Function Pointers

- **SelectorFunctionUPP** (?: OSType, ?: LONGINT*) -> OSErr

## Dispatchers

- **GestaltDispatch**—

