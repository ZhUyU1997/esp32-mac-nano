# CodeFragments Interfaces

Source: `multiversal/defs/CodeFragments.yaml`

- Functions: **7**
- Typedefs: **4**
- Structs: **9**, Unions: **0**
- Enums: **5**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **1**
- Low-memory globals: **0**

## Functions

### CloseConnection  

```c
OSErr CloseConnection(ConnectionID* cidp)
```

Trap: — (executor 实现，无 trap) executor=C_

### CountSymbols  

```c
OSErr CountSymbols(ConnectionID id, LONGINT* countp)
```

Trap: — (executor 实现，无 trap) executor=C_

### FindSymbol  

```c
OSErr FindSymbol(ConnectionID connID, ConstStr255Param symName, Ptr* symAddr, SymClass* symClass)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetDiskFragment  

```c
OSErr GetDiskFragment(FSSpecPtr fsp, LONGINT offset, LONGINT length, ConstStr63Param fragname, LoadFlags flags, ConnectionID* connp, Ptr* mainAddrp, Str255 errname)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetIndSymbol  

```c
OSErr GetIndSymbol(ConnectionID id, LONGINT index, Str255 name, Ptr* addrp, SymClass* classp)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetMemFragment  

```c
OSErr GetMemFragment(void* addr, uint32_t length, ConstStr63Param fragname, LoadFlags flags, ConnectionID* connp, Ptr* mainAddrp, Str255 errname)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetSharedLibrary  

```c
OSErr GetSharedLibrary(ConstStr63Param library, OSType arch, LoadFlags loadflags, ConnectionID* cidp, Ptr* mainaddrp, Str255 errName)
```

Trap: — (executor 实现，无 trap) executor=C_

## Typedefs

- **LoadFlags** = uint32_t
- **ConnectionID** = CFragConnection*
- **CFragClosureID** = CFragClosure*
- **SymClass** = uint8_t

## Enums

- **?**
- **?**
- **?**
- **?**
- **?**

### Enum Values

**anonymous**:

- `kImportLibraryCFrag` = ?
- `kApplicationCFrag` = ?
- `kDropInAdditionCFrag` = ?
- `kStubLibraryCFrag` = ?
- `kWeakStubLibraryCFrag` = ?

**anonymous**:

- `kWholeFork` = 0

**anonymous**:

- `kInMem` = ?
- `kOnDiskFlat` = ?
- `kOnDiskSegmented` = ?

**anonymous**:

- `kPowerPCArch` = 'pwpc'
- `kMotorola68KArch` = 'm68k'

**anonymous**:

- `kLoadLib` = 1
- `kReferenceCFrag` = 1
- `kFindLib` = 2
- `kLoadNewCopy` = 5

## Structs

- **cfrg_resource_t** { reserved0: uint32_t, reserved1: uint32_t, version: uint32_t, reserved2: uint32_t, reserved3: uint32_t, reserved4: uint32_t, reserved5: uint32_t, n_descripts: int32_t }
- **cfir_t** { isa: OSType, update_level: uint32_t, current_version: uint32_t, oldest_definition_version: uint32_t, stack_size: uint32_t, appl_library_dir: int16_t, fragment_type: uint8_t, fragment_location: uint8_t, offset_to_fragment: int32_t, fragment_length: int32_t, reserved0: uint32_t, reserved1: uint32_t, cfir_length: uint16_t, name: uint8_t[1] }
- **MemFragment** { address: Ptr, length: uint32_t, inPlace: Boolean, reservedA: uint8_t, reservedB: uint16_t }
- **DiskFragment** { fileSpec: FSSpecPtr, offset: uint32_t, length: uint32_t }
- **SegmentedFragment** { fileSpec: FSSpecPtr, rsrcType: OSType, rsrcID: INTEGER, reservedA: uint16_t }
- **FragmentLocator** { where: uint32_t, u: ? }
- **InitBlock** { contextID: uint32_t, closureID: uint32_t, connectionID: uint32_t, fragLocator: FragmentLocator, libName: StringPtr, reserved4: uint32_t }
- **CFragConnection** {  }
- **CFragClosure** {  }

## Dispatchers

- **CodeFragmentDispatch**—

