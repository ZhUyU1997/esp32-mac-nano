# ADB Interfaces

QuickDraw IMV-367 (false);

Source: `multiversal/defs/ADB.yaml`

- Functions: **6**
- Typedefs: **0**
- Structs: **2**, Unions: **0**
- Enums: **0**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **0**
- Low-memory globals: **2**

## Functions

### ADBOp  

```c
OSErr ADBOp(Ptr data, ProcPtr procp, Ptr buffer, INTEGER command)
```

Trap: — (executor 实现，无 trap)

### ADBReInit  

```c
void ADBReInit()
```

Trap: `0xA07B` executor=True

### CountADBs  

```c
INTEGER CountADBs()
```

Trap: `0xA077` executor=True

### GetADBInfo  

```c
OSErr GetADBInfo(ADBDataBlock* adbp, INTEGER address)
```

Trap: `0xA079` executor=True

### GetIndADB  

```c
OSErr GetIndADB(ADBDataBlock* adbp, INTEGER index)
```

Trap: `0xA078` executor=True

### SetADBInfo  

```c
OSErr SetADBInfo(ADBSetInfoBlock* adbp, INTEGER address)
```

Trap: `0xA07A` executor=True

## Structs

- **ADBDataBlock** { devType: SignedByte, origADBAddr: SignedByte, dbServiceRtPtr: ProcPtr, dbDataAreaAddr: Ptr }
- **ADBSetInfoBlock** { siServiceRtPtr: ProcPtr, siDataAreaAddr: Ptr }

## Low Memory Globals

- **KbdLast** @ 0x218 (Byte) — QuickDraw IMV-367 (false);
- **KbdType** @ 0x21E (Byte) — QuickDraw IMV-367 (false);
