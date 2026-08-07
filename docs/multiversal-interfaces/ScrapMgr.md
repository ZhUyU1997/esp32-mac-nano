# ScrapMgr Interfaces

ScrapMgr IMI-457 (true);

Source: `multiversal/defs/ScrapMgr.yaml`

- Functions: **6**
- Typedefs: **1**
- Structs: **1**, Unions: **0**
- Enums: **1**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **0**
- Low-memory globals: **5**

## Functions

### GetScrap  

```c
LONGINT GetScrap(Handle h, ResType rest, LONGINT* off)
```

Trap: `0xA9FD` executor=C_

### InfoScrap  

```c
PScrapStuff InfoScrap()
```

Trap: `0xA9F9` executor=C_

### LoadScrap  

```c
LONGINT LoadScrap()
```

Trap: `0xA9FB` executor=C_

### PutScrap  

```c
LONGINT PutScrap(LONGINT len, ResType rest, Ptr p)
```

Trap: `0xA9FE` executor=C_

### UnloadScrap  

```c
LONGINT UnloadScrap()
```

Trap: `0xA9FA` executor=C_

### ZeroScrap  

```c
LONGINT ZeroScrap()
```

Trap: `0xA9FC` executor=C_

## Typedefs

- **PScrapStuff** = ScrapStuff*

## Enums

- **?**

### Enum Values

**anonymous**:

- `noScrapErr` = -100
- `noTypeErr` = -102

## Structs

- **ScrapStuff** { scrapSize: LONGINT, scrapHandle: Handle, scrapCount: INTEGER, scrapState: INTEGER, scrapName: StringPtr }

## Low Memory Globals

- **ScrapSize** @ 0x960 (LONGINT) — ScrapMgr IMI-457 (true);
- **ScrapHandle** @ 0x964 (Handle) — ScrapMgr IMI-457 (true);
- **ScrapCount** @ 0x968 (INTEGER) — ScrapMgr IMI-457 (true);
- **ScrapState** @ 0x96A (INTEGER) — ScrapMgr IMI-457 (true);
- **ScrapName** @ 0x96C (StringPtr) — ScrapMgr IMI-457 (true);
