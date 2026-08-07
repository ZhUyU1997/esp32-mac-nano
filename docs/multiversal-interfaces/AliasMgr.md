# AliasMgr Interfaces

Source: `multiversal/defs/AliasMgr.yaml`

- Functions: **9**
- Typedefs: **3**
- Structs: **0**, Unions: **0**
- Enums: **1**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **1**
- Low-memory globals: **0**

## Functions

### FindFolder  

```c
OSErr FindFolder(int16_t vRefNum, OSType folderType, Boolean createFolder, int16_t* foundVRefNum, int32_t* foundDirID)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetAliasInfo  

```c
OSErr GetAliasInfo(AliasHandle alias, AliasTypeInfo index, Str63 theString)
```

Trap: — (executor 实现，无 trap) executor=C_

### MatchAlias  

```c
OSErr MatchAlias(FSSpecPtr fromFile, int32_t rulesMask, AliasHandle alias, int16_t* aliasCount, FSSpecArrayPtr aliasList, Boolean* needsUpdate, AliasFilterUPP aliasFilter, Ptr yourDataPtr)
```

Trap: — (executor 实现，无 trap) executor=C_

### NewAlias  

```c
OSErr NewAlias(FSSpecPtr fromFile, FSSpecPtr target, AliasHandle* alias)
```

Trap: — (executor 实现，无 trap) executor=C_

### NewAliasMinimal  

```c
OSErr NewAliasMinimal(FSSpecPtr target, AliasHandle* alias)
```

Trap: — (executor 实现，无 trap) executor=C_

### NewAliasMinimalFromFullPath  

```c
OSErr NewAliasMinimalFromFullPath(int16_t fullpathLength, Ptr fullpath, Str32 zoneName, Str31 serverName, AliasHandle* alias)
```

Trap: — (executor 实现，无 trap) executor=C_

### ResolveAlias  

```c
OSErr ResolveAlias(FSSpecPtr fromFile, AliasHandle alias, FSSpecPtr target, Boolean* wasAliased)
```

Trap: — (executor 实现，无 trap) executor=C_

### ResolveAliasFile  

```c
OSErr ResolveAliasFile(FSSpecPtr theSpec, Boolean resolveAliasChains, Boolean* targetIsFolder, Boolean* wasAliased)
```

Trap: — (executor 实现，无 trap) executor=C_

### UpdateAlias  

```c
OSErr UpdateAlias(FSSpecPtr fromFile, FSSpecPtr target, AliasHandle alias, Boolean* wasChanged)
```

Trap: — (executor 实现，无 trap) executor=C_

## Typedefs

- **AliasFilterUPP** = ProcPtr
- **AliasHandle** = Handle
- **AliasTypeInfo** = int16_t

## Enums

- **?**

### Enum Values

**anonymous**:

- `kSystemFolderType` = 'macs'
- `kDesktopFolderType` = 'desk'
- `kTrashFolderType` = 'trsh'
- `kWhereToEmptyTrashFolderType` = 'empt'
- `kPrintMonitorDocsFolderType` = 'prnt'
- `kStartupFolderType` = 'strt'
- `kAppleMenuFolderType` = 'amnu'
- `kControlPanelFolderType` = 'ctrl'
- `kExtensionFolderType` = 'extn'
- `kPreferencesFolderType` = 'pref'
- `kTemporaryFolderType` = 'temp'
- `kFontFolderType` = 'font'

## Dispatchers

- **AliasDispatch**—

