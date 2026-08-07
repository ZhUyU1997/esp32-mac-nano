# StartMgr Interfaces

StartMgr IMV-348 (true-b);

Source: `multiversal/defs/StartMgr.yaml`

- Functions: **0**
- Typedefs: **3**
- Structs: **2**, Unions: **1**
- Enums: **0**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **0**
- Low-memory globals: **4**

## Typedefs

- **DefStartPtr** = DefStartRec*
- **DefVideoPtr** = DefVideoRec*
- **DefOSPtr** = DefOSRec*

## Structs

- **DefVideoRec** { sdSlot: SignedByte, sdSResource: SignedByte }
- **DefOSRec** { sdReserved: SignedByte, sdOSType: SignedByte }

## Unions

- **DefStartRec** { slotDev: ?, scsiDev: ? }

## Low Memory Globals

- **CPUFlag** @ 0x12F (Byte) — StartMgr IMV-348 (true-b);
- **TimeDBRA** @ 0xD00 (INTEGER) — StartMgr IMV (false);
- **TimeSCCDB** @ 0xD02 (INTEGER) — StartMgr IMV (false);
- **TimeSCSIDB** @ 0xDA6 (INTEGER) — StartMgr IMV (false);
