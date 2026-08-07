# PEFBinaryFormat Interfaces

flags for PEFImportedLibrary::options

Source: `multiversal/defs/PEFBinaryFormat.yaml`

- Functions: **9**
- Typedefs: **2**
- Structs: **7**, Unions: **0**
- Enums: **10**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **0**
- Low-memory globals: **0**

## Functions

### PEFComposeImportedSymbol  

```c
uint32_t PEFComposeImportedSymbol(uint32_t cls, uint32_t nameOffset)
```

Trap: — (executor 实现，无 trap)

### PEFExportedSymbolClass  

```c
uint32_t PEFExportedSymbolClass(uint32_t classAndName)
```

Trap: — (executor 实现，无 trap)

### PEFExportedSymbolNameOffset  

```c
uint32_t PEFExportedSymbolNameOffset(uint32_t classAndName)
```

Trap: — (executor 实现，无 trap)

### PEFRelocComposeBySectC  

```c
uint16_t PEFRelocComposeBySectC(uint16_t runLength)
```

Trap: — (executor 实现，无 trap)

### PEFRelocComposeBySectD  

```c
uint16_t PEFRelocComposeBySectD(uint16_t runLength)
```

Trap: — (executor 实现，无 trap)

### PEFRelocComposeLgByImport_1st  

```c
uint16_t PEFRelocComposeLgByImport_1st(uint32_t fullIndex)
```

Trap: — (executor 实现，无 trap)

### PEFRelocComposeLgByImport_2nd  

```c
uint16_t PEFRelocComposeLgByImport_2nd(uint32_t fullIndex)
```

Trap: — (executor 实现，无 trap)

### PEFRelocComposeSetPosition_1st  

```c
uint16_t PEFRelocComposeSetPosition_1st(uint32_t fullOffset)
```

Trap: — (executor 实现，无 trap)

### PEFRelocComposeSetPosition_2nd  

```c
uint16_t PEFRelocComposeSetPosition_2nd(uint32_t fullOffset)
```

Trap: — (executor 实现，无 trap)

## Typedefs

- **PEFExportedSymbolKey** = uint32_t
- **PEFExportedSymbolHashSlot** = uint32_t

## Enums

- **?**
- **?**
- **?**
- **?**
- **?** — flags for PEFImportedLibrary::options
- **?**
- **?**
- **?**
- **?**
- **?**

### Enum Values

**anonymous**:

- `kPEFTag1` = 'Joy!'
- `kPEFTag2` = 'peff'
- `kPEFVersion` = 1

**anonymous**:

- `kPEFProcessShare` = 1
- `kPEFGlobalShare` = 4
- `kPEFProtectedShare` = 5

**anonymous**:

- `kPEFCodeSection` = 0
- `kPEFUnpackedDataSection` = ?
- `kPEFPatternDataSection` = ?
- `kPEFConstantSection` = ?
- `kPEFLoaderSection` = ?
- `kPEFDebugSection` = ?
- `kPEFExecutableDataSection` = ?
- `kPEFExceptionSection` = ?
- `kPEFTracebackSection` = ?

**anonymous**:

- `kPEFCodeSymbol` = ?
- `kPEFDataSymbol` = ?
- `kPEFTVectorSymbol` = ?
- `kPEFTOCSymbol` = ?
- `kPEFGlueSymbol` = ?

**anonymous** — flags for PEFImportedLibrary::options:

- `kPEFWeakImportLibMask` = 64
- `kPEFInitLibBeforeMask` = 128

**anonymous**:

- `kPEFFirstSectionHeaderOffset` = sizeof(PEFContainerHeader)

**anonymous**:

- `kExponentLimit` = 16
- `kAverageChainLimit` = 10

**anonymous**:

- `kPEFHashLengthShift` = 16
- `kPEFHashValueMask` = 65535

**anonymous**:

- `FIRST_INDEX_SHIFT` = 0
- `FIRST_INDEX_MASK` = 262143
- `CHAIN_COUNT_SHIFT` = 18
- `CHAIN_COUNT_MASK` = 16383

**anonymous**:

- `NAME_MASK` = 16777215

## Structs

- **PEFContainerHeader** { tag1: OSType, tag2: OSType, architecture: OSType, formatVersion: uint32_t, dateTimeStamp: uint32_t, oldDefVersion: uint32_t, oldImpVersion: uint32_t, currentVersion: uint32_t, sectionCount: uint16_t, instSectionCount: uint16_t, reservedA: uint32_t }
- **PEFSectionHeader** { nameOffset: int32_t, defaultAddress: uint32_t, totalLength: uint32_t, unpackedLength: uint32_t, containerLength: uint32_t, containerOffset: uint32_t, sectionKind: uint8_t, shareKind: uint8_t, alignment: uint8_t, reservedA: uint8_t }
- **PEFLoaderInfoHeader** { mainSection: int32_t, mainOffset: uint32_t, initSection: int32_t, initOffset: uint32_t, termSection: int32_t, termOffset: uint32_t, importedLibraryCount: uint32_t, totalImportedSymbolCount: uint32_t, relocSectionCount: uint32_t, relocInstrOffset: uint32_t, loaderStringsOffset: uint32_t, exportHashOffset: uint32_t, exportHashTablePower: uint32_t, exportedSymbolCount: uint32_t }
- **PEFImportedLibrary** { nameOffset: uint32_t, oldImpVersion: uint32_t, currentVersion: uint32_t, importedSymbolCount: uint32_t, firstImportedSymbol: uint32_t, options: uint8_t, reservedA: uint8_t, reservedB: uint16_t }
- **PEFLoaderRelocationHeader** { sectionIndex: uint16_t, reservedA: uint16_t, relocCount: uint32_t, firstRelocOffset: uint32_t }
- **PEFExportedSymbol** { classAndName: uint32_t, symbolValue: uint32_t, sectionIndex: int16_t }
- **PEFImportedSymbol** { classAndName: uint32_t }

