# PPC Interfaces

Source: `multiversal/defs/PPC.yaml`

- Functions: **0**
- Typedefs: **3**
- Structs: **5**, Unions: **0**
- Enums: **0**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **0**
- Low-memory globals: **0**

## Typedefs

- **PPCPortKinds** = int16_t
- **PPCLocationKind** = int16_t
- **PPCPortPtr** = PPCPortRec*

## Structs

- **EntityName** { objStr: Str32, typeStr: Str32, zoneStr: Str32 }
- **PPCXTIAddress** { fAddressType: int16_t, fAddress: uint8_t[96] }
- **PPCAddrRec** { Reserved: uint8_t[3], xtiAddrLen: uint8_t, xtiAddr: PPCXTIAddress }
- **LocationNameRec** { locationKindSelector: PPCLocationKind, u: ? }
- **PPCPortRec** { nameScript: ScriptCode, name: Str32, portKindsSelector: PPCPortKinds, u: ? }

