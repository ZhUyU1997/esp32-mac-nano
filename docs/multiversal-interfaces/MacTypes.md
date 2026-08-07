# MacTypes Interfaces

very important not to use this as char

Source: `multiversal/defs/MacTypes.yaml`

- Functions: **0**
- Typedefs: **42**
- Structs: **4**, Unions: **1**
- Enums: **2**
- Function pointers: **1**
- Common blocks: **0**
- Dispatchers: **0**
- Low-memory globals: **0**

## Typedefs

- **INTEGER** = int16_t
- **LONGINT** = int32_t
- **ULONGINT** = uint32_t
- **Boolean** = int8_t
- **CharParameter** = int16_t — very important not to use this as char
- **SignedByte** = int8_t
- **Byte** = uint8_t
- **Ptr** = char*
- **Handle** = Ptr*
- **Boolean** = int8_t
- **SInt8** = int8_t
- **UInt8** = uint8_t
- **SInt16** = int16_t
- **UInt16** = uint16_t
- **SInt32** = int32_t
- **UInt32** = uint32_t
- **Str15** = Byte[16]
- **Str31** = Byte[32]
- **Str32** = Byte[33]
- **Str63** = Byte[64]
- **Str255** = Byte[256]
- **StringPtr** = Byte*
- **ConstStringPtr** = const uint8_t*
- **ConstStr255Param** = ConstStringPtr
- **ConstStr63Param** = ConstStringPtr
- **ConstStr31Param** = ConstStringPtr
- **ConstStr16Param** = ConstStringPtr
- **StringHandle** = StringPtr*
- **UniversalProcPtr** = RoutineDescriptor*
- **Fixed** = LONGINT
- **Fract** = LONGINT
- **SmallFract** = uint16_t — SmallFract represnts values between 0 and 65535
- **Extended** = double
- **Size** = LONGINT
- **OSErr** = INTEGER
- **OSType** = LONGINT
- **ResType** = LONGINT
- **QElemPtr** = QElem*
- **QHdrPtr** = QHdr*
- **RectPtr** = Rect*
- **ScriptCode** = INTEGER — from IntlUtil.h
- **LangCode** = INTEGER

## Enums

- **?**
- **?**

### Enum Values

**anonymous**:

- `MaxSmallFract` = 65535

**anonymous**:

- `noErr` = 0
- `paramErr` = -50

## Structs

- **RoutineDescriptor** {  }
- **QHdr** { qFlags: INTEGER, qHead: QElemPtr, qTail: QElemPtr }
- **Rect** { top: INTEGER, left: INTEGER, bottom: INTEGER, right: INTEGER }
- **Point** { v: INTEGER, h: INTEGER }

## Unions

- **QElem** {  }

## Function Pointers

- **VoidUPP** () -> void

