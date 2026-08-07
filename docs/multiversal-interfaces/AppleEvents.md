# AppleEvents Interfaces

"internal". TODO: verify if they really exist as entry points.

Source: `multiversal/defs/AppleEvents.yaml`

- Functions: **58**
- Typedefs: **28**
- Structs: **8**, Unions: **1**
- Enums: **18**
- Function pointers: **3**
- Common blocks: **0**
- Dispatchers: **1**
- Low-memory globals: **1**

## Functions

### AECallObjectAccessor  

```c
OSErr AECallObjectAccessor(DescType desiredClass, AEDesc* containerToken, DescType containerClass, DescType keyForm, AEDesc* keyData, AEDesc* theToken)
```

Trap: — (executor 实现，无 trap) executor=C_

### AECoerceDesc  

```c
OSErr AECoerceDesc(AEDesc* desc, DescType result_type, AEDesc* desc_out)
```

Trap: — (executor 实现，无 trap) executor=C_

### AECoercePtr  

```c
OSErr AECoercePtr(DescType data_type, Ptr data, Size data_size, DescType result_type, AEDesc* desc_out)
```

Trap: — (executor 实现，无 trap) executor=C_

### AECountItems  

```c
OSErr AECountItems(AEDescList* list, int32_t* count_out)
```

Trap: — (executor 实现，无 trap) executor=C_

### AECreateAppleEvent  

```c
OSErr AECreateAppleEvent(AEEventClass event_class, AEEventID event_id, AEAddressDesc* target, int16_t return_id, int32_t transaction_id, AppleEvent* evt)
```

Trap: — (executor 实现，无 trap) executor=C_

### AECreateDesc  

```c
OSErr AECreateDesc(DescType type, const void* data, Size data_size, AEDesc* desc_out)
```

Trap: — (executor 实现，无 trap) executor=C_

### AECreateList  

```c
OSErr AECreateList(Ptr list_elt_prefix, Size list_elt_prefix_size, Boolean is_record_p, AEDescList* list_out)
```

Trap: — (executor 实现，无 trap) executor=C_

### AEDeleteItem  

```c
OSErr AEDeleteItem(AEDescList* list, int32_t index)
```

Trap: — (executor 实现，无 trap) executor=C_

### AEDeleteParam  

```c
OSErr AEDeleteParam(AERecord* record, AEKeyword keyword)
```

Trap: — (executor 实现，无 trap) executor=C_

### AEDisposeDesc  

```c
OSErr AEDisposeDesc(AEDesc* desc)
```

Trap: — (executor 实现，无 trap) executor=C_

### AEDisposeToken  

```c
OSErr AEDisposeToken(AEDesc* theToken)
```

Trap: — (executor 实现，无 trap) executor=C_

### AEDuplicateDesc  

```c
OSErr AEDuplicateDesc(AEDesc* src, AEDesc* dst)
```

Trap: — (executor 实现，无 trap) executor=C_

### AEGetArray  

```c
OSErr AEGetArray(AEDescList* list, AEArrayType array_type, AEArrayDataPointer array_ptr, Size max_size, DescType* return_item_type, Size* return_item_size, int32_t* return_item_count)
```

Trap: — (executor 实现，无 trap) executor=C_

### AEGetAttributeDesc  

```c
OSErr AEGetAttributeDesc(AppleEvent* evt, AEKeyword keyword, DescType desired_type, AEDesc* desc_out)
```

Trap: — (executor 实现，无 trap) executor=C_

### AEGetAttributePtr  

```c
OSErr AEGetAttributePtr(AppleEvent* evt, AEKeyword keyword, DescType desired_type, DescType* type_out, void* data, Size max_size, Size* size_out)
```

Trap: — (executor 实现，无 trap) executor=C_

### AEGetCoercionHandler  

```c
OSErr AEGetCoercionHandler(DescType from_type, DescType to_type, AECoerceDescUPP* hdlr_out, int32_t* refcon_out, Boolean* from_type_is_desc_p_out, Boolean system_handler_p)
```

Trap: — (executor 实现，无 trap) executor=C_ — prototypes go here

### AEGetEventHandler  

```c
OSErr AEGetEventHandler(AEEventClass event_class, AEEventID event_id, AEEventHandlerUPP* hdlr, int32_t* refcon, Boolean system_handler_p)
```

Trap: — (executor 实现，无 trap) executor=C_

### AEGetInteractionAllowed  

```c
OSErr AEGetInteractionAllowed(AEInteractionAllowed* return_level)
```

Trap: — (executor 实现，无 trap) executor=C_

### AEGetNthDesc  

```c
OSErr AEGetNthDesc(AEDescList* list, int32_t index, DescType desired_type, AEKeyword* keyword_out, AEDesc* desc_out)
```

Trap: — (executor 实现，无 trap) executor=C_

### AEGetNthPtr  

```c
OSErr AEGetNthPtr(AEDescList* list, int32_t index, DescType desired_type, AEKeyword* keyword_out, DescType* type_out, void* data, int32_t max_size, int32_t* size_out)
```

Trap: — (executor 实现，无 trap) executor=C_

### AEGetObjectAccessor  

```c
OSErr AEGetObjectAccessor(DescType desiredClass, DescType containerType, ProcPtr* theAccessor, LONGINT* accessorRefcon, Boolean isSysHandler)
```

Trap: — (executor 实现，无 trap) executor=C_

### AEGetParamDesc  

```c
OSErr AEGetParamDesc(AERecord* record, AEKeyword keyword, DescType desired_type, AEDesc* desc_out)
```

Trap: — (executor 实现，无 trap) executor=C_

### AEGetParamPtr  

```c
OSErr AEGetParamPtr(AERecord* record, AEKeyword keyword, DescType desired_type, DescType* type_out, Ptr data, Size max_size, Size* size_out)
```

Trap: — (executor 实现，无 trap) executor=C_

### AEGetSpecialHandler  

```c
OSErr AEGetSpecialHandler(AEKeyword function_class, AEEventHandlerUPP* hdlr_out, Boolean system_handler_p)
```

Trap: — (executor 实现，无 trap) executor=C_

### AEGetTheCurrentEvent  

```c
OSErr AEGetTheCurrentEvent(AppleEvent* return_evt)
```

Trap: — (executor 实现，无 trap) executor=C_

### AEInstallCoercionHandler  

```c
OSErr AEInstallCoercionHandler(DescType from_type, DescType to_type, AECoerceDescUPP hdlr, int32_t refcon, Boolean from_type_is_desc_p, Boolean system_handler_p)
```

Trap: — (executor 实现，无 trap) executor=C_

### AEInstallEventHandler  

```c
OSErr AEInstallEventHandler(AEEventClass event_class, AEEventID event_id, AEEventHandlerUPP hdlr, int32_t refcon, Boolean system_handler_p)
```

Trap: — (executor 实现，无 trap) executor=C_

### AEInstallObjectAccessor  

```c
OSErr AEInstallObjectAccessor(DescType desiredClass, DescType containerType, ProcPtr theAccessor, LONGINT refcon, Boolean isSysHandler)
```

Trap: — (executor 实现，无 trap) executor=C_

### AEInstallSpecialHandler  

```c
OSErr AEInstallSpecialHandler(AEKeyword function_class, AEEventHandlerUPP hdlr, Boolean system_handler_p)
```

Trap: — (executor 实现，无 trap) executor=C_

### AEInteractWithUser  

```c
OSErr AEInteractWithUser(int32_t timeout, NMRecPtr nm_req, IdleUPP idle_proc)
```

Trap: — (executor 实现，无 trap) executor=C_

### AEManagerInfo  

```c
OSErr AEManagerInfo(LONGINT* resultp)
```

Trap: — (executor 实现，无 trap) executor=C_

### AEProcessAppleEvent  

```c
OSErr AEProcessAppleEvent(EventRecord* evt)
```

Trap: — (executor 实现，无 trap) executor=C_

### AEPutArray  

```c
OSErr AEPutArray(AEDescList* list, AEArrayType type, AEArrayDataPointer array_data, DescType item_type, Size item_size, int32_t item_count)
```

Trap: — (executor 实现，无 trap) executor=C_

### AEPutAttributeDesc  

```c
OSErr AEPutAttributeDesc(AppleEvent* evt, AEKeyword keyword, AEDesc* desc)
```

Trap: — (executor 实现，无 trap) executor=C_

### AEPutAttributePtr  

```c
OSErr AEPutAttributePtr(AppleEvent* evt, AEKeyword keyword, DescType type, const void* data, Size size)
```

Trap: — (executor 实现，无 trap) executor=C_

### AEPutDesc  

```c
OSErr AEPutDesc(AEDescList* list, int32_t index, AEDesc* desc)
```

Trap: — (executor 实现，无 trap) executor=C_

### AEPutParamDesc  

```c
OSErr AEPutParamDesc(AERecord* record, AEKeyword keyword, AEDesc* desc)
```

Trap: — (executor 实现，无 trap) executor=C_

### AEPutParamPtr  

```c
OSErr AEPutParamPtr(AERecord* record, AEKeyword keyword, DescType type, const void* data, Size data_size)
```

Trap: — (executor 实现，无 trap) executor=C_

### AEPutPtr  

```c
OSErr AEPutPtr(AEDescList* list, int32_t index, DescType type, const void* data, Size data_size)
```

Trap: — (executor 实现，无 trap) executor=C_

### AERemoveCoercionHandler  

```c
OSErr AERemoveCoercionHandler(DescType from_type, DescType to_type, AECoerceDescUPP hdlr, Boolean system_handler_p)
```

Trap: — (executor 实现，无 trap) executor=C_

### AERemoveEventHandler  

```c
OSErr AERemoveEventHandler(AEEventClass event_class, AEEventID event_id, AEEventHandlerUPP hdlr, Boolean system_handler_p)
```

Trap: — (executor 实现，无 trap) executor=C_

### AERemoveObjectAccessor  

```c
OSErr AERemoveObjectAccessor(DescType desiredClass, DescType containerType, ProcPtr theAccessor, Boolean isSysHandler)
```

Trap: — (executor 实现，无 trap) executor=C_

### AERemoveSpecialHandler  

```c
OSErr AERemoveSpecialHandler(AEKeyword function_class, AEEventHandlerUPP hdlr, Boolean system_handler_p)
```

Trap: — (executor 实现，无 trap) executor=C_

### AEResetTimer  

```c
OSErr AEResetTimer(AppleEvent* evt)
```

Trap: — (executor 实现，无 trap) executor=C_

### AEResolve  

```c
OSErr AEResolve(AEDesc* objectSpecifier, INTEGER callbackFlags, AEDesc* theToken)
```

Trap: — (executor 实现，无 trap) executor=C_

### AEResumeTheCurrentEvent  

```c
OSErr AEResumeTheCurrentEvent(AppleEvent* evt, AppleEvent* reply, AEEventHandlerUPP dispatcher, int32_t refcon)
```

Trap: — (executor 实现，无 trap) executor=C_

### AESend  

```c
OSErr AESend(AppleEvent* evt, AppleEvent* reply, AESendMode send_mode, AESendPriority send_priority, int32_t timeout, IdleUPP idle_proc, EventFilterUPP filter_proc)
```

Trap: — (executor 实现，无 trap) executor=C_

### AESetInteractionAllowed  

```c
OSErr AESetInteractionAllowed(AEInteractionAllowed level)
```

Trap: — (executor 实现，无 trap) executor=C_

### AESetObjectCallbacks  

```c
OSErr AESetObjectCallbacks(ProcPtr myCompareProc, ProcPtr myCountProc, ProcPtr myDisposeTokenProc, ProcPtr myGetMarkTokenProc, ProcPtr myMarkProc, ProcPtr myAdjustMarksProc, ProcPtr myGetErrDescProc)
```

Trap: — (executor 实现，无 trap) executor=C_

### AESetTheCurrentEvent  

```c
OSErr AESetTheCurrentEvent(AppleEvent* evt)
```

Trap: — (executor 实现，无 trap) executor=C_

### AESizeOfAttribute  

```c
OSErr AESizeOfAttribute(AppleEvent* evt, AEKeyword keyword, DescType* type_out, Size* size_out)
```

Trap: — (executor 实现，无 trap) executor=C_

### AESizeOfNthItem  

```c
OSErr AESizeOfNthItem(AEDescList* list, int32_t index, DescType* type_out, Size* size_out)
```

Trap: — (executor 实现，无 trap) executor=C_

### AESizeOfParam  

```c
OSErr AESizeOfParam(AERecord* record, AEKeyword keyword, DescType* type_out, Size* size_out)
```

Trap: — (executor 实现，无 trap) executor=C_ — extern OSErr C_AEDeleteParam(AppleEvent *evt, AEKeyword keyword); PASCAL_SUBTRAP(AEDeleteParam, 0xA816, 0x0413, Pack8); The following does not exist. Maybe it should be AEDeleteParam? extern OSErr C_AEDeleteAttribute(AppleEvent *evt, AEKeyword keyword); PASCAL_SUBTRAP_UNKNOWN(AEDeleteAttribute, 0xA816, Pack8);

### AESuspendTheCurrentEvent  

```c
OSErr AESuspendTheCurrentEvent(AppleEvent* evt)
```

Trap: — (executor 实现，无 trap) executor=C_

### _AE_hdlr_delete  

```c
OSErr _AE_hdlr_delete(AE_hdlr_table_h ?, int32_t ?, AE_hdlr_selector_t* ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### _AE_hdlr_install  

```c
OSErr _AE_hdlr_install(AE_hdlr_table_h ?, int32_t ?, AE_hdlr_selector_t* ?, AE_hdlr_t* ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### _AE_hdlr_lookup  

```c
OSErr _AE_hdlr_lookup(AE_hdlr_table_h ?, int32_t ?, AE_hdlr_selector_t* ?, AE_hdlr_t* ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### _AE_hdlr_table_alloc  

```c
OSErr _AE_hdlr_table_alloc(int32_t ?, int32_t ?, int32_t ?, int8_t ?, AE_hdlr_table_h* ?)
```

Trap: — (executor 实现，无 trap) executor=C_ — "internal". TODO: verify if they really exist as entry points.

## Typedefs

- **AEEventClass** = int32_t
- **AEEventID** = int32_t
- **AEKeyword** = int32_t
- **DescType** = ResType
- **descriptor_t** = AEDesc — ### hack, delete
- **key_desc_t** = AEKeyDesc
- **AEAddressDesc** = AEDesc
- **AEDescList** = AEDesc
- **AERecord** = AEDescList
- **AppleEvent** = AERecord
- **AESendMode** = int32_t
- **AESendPriority** = int16_t — #define kAEWantReceipt	???
- **AEEventSource** = uint8_t
- **AEInteractionAllowed** = uint8_t
- **AEArrayType** = uint8_t
- **AEArrayDataPointer** = AEArrayData*
- **IdleUPP** = ProcPtr
- **EventFilterUPP** = ProcPtr
- **AE_hdlr_t** = AE_hdlr — #### internal
- **AE_hdlr_selector_t** = AE_hdlr_selector
- **AE_hdlr_table_elt_t** = AE_hdlr_table_elt
- **AE_hdlr_table_t** = AE_hdlr_table
- **AE_hdlr_table_ptr** = AE_hdlr_table_t*
- **AE_hdlr_table_h** = AE_hdlr_table_ptr*
- **AE_zone_tables_t** = AE_zone_tables — points to a 32byte handle of unknown contents (at least, sometimes)
- **AE_zone_tables_ptr** = AE_zone_tables_t*
- **AE_zone_tables_h** = AE_zone_tables_ptr*
- **AE_info_ptr** = AE_info_t*

## Enums

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
- **?**
- **?**
- **?** — types
- **?**
- **?**

### Enum Values

**anonymous**:

- `_kAEReplyMask` = 3

**anonymous**:

- `kAENoReply` = 1
- `kAEQueueReply` = 2
- `kAEWaitReply` = 3

**anonymous**:

- `_kAEInteractMask` = 48

**anonymous**:

- `kAENeverInteract` = 16
- `kAECanInteract` = 32
- `kAEAlwaysInteract` = 48

**anonymous**:

- `kAECanSwitchLayer` = 64
- `kAEDontReconnect` = 128

**anonymous**:

- `kAEInteractWithSelf` = 0
- `kAEInteractWithLocal` = 1
- `kAEInteractWithAll` = 2

**anonymous**:

- `kAEUnknownSource` = 0
- `kAEDirectCall` = 1
- `kAESameProcess` = 2
- `kAELocalProcess` = 3
- `kAERemoteProcess` = 4

**anonymous**:

- `kAEDataArray` = ?
- `kAEPackedArray` = ?
- `kAEHandleArray` = ?
- `kAEDescArray` = ?
- `kAEKeyDescArray` = ?

**anonymous**:

- `kAutoGenerateReturnID` = -1
- `kAnyTransactionID` = 0

**anonymous**:

- `kAENormalPriority` = 0
- `kAEHighPriority` = 1

**anonymous**:

- `kAEDefaultTimeout` = -1
- `kNoTimeOut` = -2

**anonymous**:

- `invalidConnection` = -609

**anonymous**:

- `errAECoercionFail` = -1700
- `errAEDescNotFound` = -1701
- `errAEWrongDataType` = -1703
- `errAENotAEDesc` = -1704

**anonymous**:

- `errAEEventNotHandled` = -1708
- `errAEUnknownAddressType` = -1716

**anonymous**:

- `errAEHandlerNotFound` = -1717
- `errAEIllegalIndex` = -1719

**anonymous** — types:

- `typeFSS` = 'fss '
- `typeAEList` = 'list'
- `typeAERecord` = 'reco'
- `typeAppleEvent` = 'aevt'
- `typeProcessSerialNumber` = 'psn '
- `typeNull` = 'null'
- `typeApplSignature` = 'sign'
- `typeType` = 'type'
- `typeWildCard` = '****'
- `typeAlias` = 'alis'
- `typeBoolean` = 'bool'
- `typeChar` = 'TEXT'
- `typeSInt16` = 'shor'
- `typeSInt32` = 'long'
- `typeUInt32` = 'magn'
- `typeSInt64` = 'comp'
- `typeIEEE32BitFloatingPoint` = 'sing'
- `typeIEEE64BitFloatingPoint` = 'doub'
- `type128BitFloatingPoint` = 'ldbl'
- `typeDecimalStruct` = 'decm'

**anonymous**:

- `keyAddressAttr` = 'addr'
- `keyEventClassAttr` = 'evcl'
- `keyEventIDAttr` = 'evid'
- `keyProcessSerialNumber` = 'psn '
- `keyDirectObject` = '----'

**anonymous**:

- `kCoreEventClass` = 'aevt'
- `kAEOpenApplication` = 'oapp'
- `kAEOpenDocuments` = 'odoc'
- `kAEPrintDocuments` = 'pdoc'
- `kAEAnswer` = 'ansr'
- `kAEQuitApplication` = 'quit'
- `keySelectProc` = 'selh'

## Structs

- **AEDesc** { descriptorType: DescType, dataHandle: Handle }
- **AEKeyDesc** { descKey: AEKeyword, descContent: AEDesc }
- **AE_hdlr** { fn: void*, refcon: int32_t }
- **AE_hdlr_selector** { sel0: int32_t, sel1: int32_t }
- **AE_hdlr_table_elt** { pad_1: int32_t, selector: AE_hdlr_selector_t, hdlr: AE_hdlr_t, pad_2: int32_t }
- **AE_hdlr_table** { pad_1: int32_t, n_allocated_bytes: int32_t, n_elts: int32_t, pad_2: int32_t[10], elts: AE_hdlr_table_elt_t[0] }
- **AE_zone_tables** { event_hdlr_table: AE_hdlr_table_h, coercion_hdlr_table: AE_hdlr_table_h, special_hdlr_table: AE_hdlr_table_h, pad_1: char[28], unknown_appl_value: char[4], pad_2: char[8], unknown_sys_handle: Handle }
- **AE_info_t** { pad_1: char[340], appl_zone_tables: AE_zone_tables_h, pad_2: char[36], system_zone_tables: AE_zone_tables_h, pad_3: char[212] }

## Unions

- **AEArrayData** { AEDataArray: int16_t[1], AEPackedArray: int8_t[1], AEHandleArray: Handle[1], AEDescArray: AEDesc[1], AEKeyDescArray: AEKeyDesc[1] }

## Function Pointers

- **AEEventHandlerUPP** (evt: const AppleEvent*, reply: AppleEvent*, refcon: int32_t) -> OSErr
- **AECoercePtrUPP** (data_type: DescType, data: Ptr, data_size: Size, to_type: DescType, refcon: int32_t, desc_out: AEDesc*) -> OSErr
- **AECoerceDescUPP** (desc: AEDesc*, to_type: DescType, refcon: int32_t, desc_out: AEDesc*) -> OSErr

## Dispatchers

- **Pack8**—

## Low Memory Globals

- **AE_info** @ 0x2B6 (AE_info_ptr) — AppleEvents AEGizmo (true);
