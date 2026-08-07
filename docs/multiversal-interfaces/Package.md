# Package Interfaces

PackageMgr ThinkC (true-b);

Source: `multiversal/defs/Package.yaml`

- Functions: **2**
- Typedefs: **0**
- Structs: **0**, Unions: **0**
- Enums: **1**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **0**
- Low-memory globals: **1**

## Functions

### InitAllPacks  

```c
void InitAllPacks()
```

Trap: `0xA9E6` executor=C_

### InitPack  

```c
void InitPack(INTEGER packid)
```

Trap: `0xA9E5` executor=C_

## Enums

- **?**

### Enum Values

**anonymous**:

- `dskInit` = 2
- `stdFile` = 3
- `flPoint` = 4
- `trFunc` = 5
- `intUtil` = 6
- `bdConv` = 7

## Low Memory Globals

- **AppPacks** @ 0xAB8 (Handle[8]) — PackageMgr ThinkC (true-b);
