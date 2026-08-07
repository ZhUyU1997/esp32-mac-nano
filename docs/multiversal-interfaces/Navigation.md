# Navigation Interfaces

Source: `multiversal/defs/Navigation.yaml`

- Functions: **20**
- Typedefs: **15**
- Structs: **8**, Unions: **1**
- Enums: **4**
- Function pointers: **3**
- Common blocks: **0**
- Dispatchers: **0**
- Low-memory globals: **0**

## Functions

### NavAskDiscardChanges  

```c
OSErr NavAskDiscardChanges(NavDialogOptions * dialogOptions, NavAskDiscardChangesResult * reply, NavEventUPP eventProc, void * callBackUD)
```

Trap: — (executor 实现，无 trap)

### NavAskSaveChanges  

```c
OSErr NavAskSaveChanges(NavDialogOptions * dialogOptions, NavAskSaveChangesAction action, NavAskSaveChangesResult * reply, NavEventUPP eventProc, void * callBackUD)
```

Trap: — (executor 实现，无 trap)

### NavChooseFile  

```c
OSErr NavChooseFile(AEDesc * defaultLocation, NavReplyRecord * reply, NavDialogOptions * dialogOptions, NavEventUPP eventProc, NavPreviewUPP previewProc, NavObjectFilterUPP filterProc, NavTypeListHandle typeList, void * callBackUD)
```

Trap: — (executor 实现，无 trap)

### NavChooseFolder  

```c
OSErr NavChooseFolder(AEDesc * defaultLocation, NavReplyRecord * reply, NavDialogOptions * dialogOptions, NavEventUPP eventProc, NavObjectFilterUPP filterProc, void * callBackUD)
```

Trap: — (executor 实现，无 trap)

### NavChooseObject  

```c
OSErr NavChooseObject(AEDesc * defaultLocation, NavReplyRecord * reply, NavDialogOptions * dialogOptions, NavEventUPP eventProc, NavObjectFilterUPP filterProc, void * callBackUD)
```

Trap: — (executor 实现，无 trap)

### NavChooseVolume  

```c
OSErr NavChooseVolume(AEDesc * defaultLocation, NavReplyRecord * reply, NavDialogOptions * dialogOptions, NavEventUPP eventProc, NavObjectFilterUPP filterProc, void * callBackUD)
```

Trap: — (executor 实现，无 trap)

### NavCompleteSave  

```c
OSErr NavCompleteSave(NavReplyRecord * translateInfo, NavTranslationOptions howToTranslate)
```

Trap: — (executor 实现，无 trap)

### NavCreatePreview  

```c
OSErr NavCreatePreview(AEDesc * theObject, OSType previewDataType, const void * previewData, Size previewDataSize)
```

Trap: — (executor 实现，无 trap)

### NavCustomAskSaveChanges  

```c
OSErr NavCustomAskSaveChanges(NavDialogOptions * dialogOptions, NavAskSaveChangesResult * reply, NavEventUPP eventProc, void * callBackUD)
```

Trap: — (executor 实现，无 trap)

### NavCustomControl  

```c
OSErr NavCustomControl(NavContext context, NavCustomControlMessage selector, void * parms)
```

Trap: — (executor 实现，无 trap)

### NavDisposeReply  

```c
OSErr NavDisposeReply(NavReplyRecord * reply)
```

Trap: — (executor 实现，无 trap)

### NavGetFile  

```c
OSErr NavGetFile(AEDesc * defaultLocation, NavReplyRecord * reply, NavDialogOptions * dialogOptions, NavEventUPP eventProc, NavPreviewUPP previewProc, NavObjectFilterUPP filterProc, NavTypeListHandle typeList, void * callBackUD)
```

Trap: — (executor 实现，无 trap)

### NavLibraryVersion  

```c
uint32_t NavLibraryVersion()
```

Trap: — (executor 实现，无 trap)

### NavLoad  

```c
OSErr NavLoad()
```

Trap: — (executor 实现，无 trap)

### NavNewFolder  

```c
OSErr NavNewFolder(AEDesc * defaultLocation, NavReplyRecord * reply, NavDialogOptions * dialogOptions, NavEventUPP eventProc, void * callBackUD)
```

Trap: — (executor 实现，无 trap)

### NavPutFile  

```c
OSErr NavPutFile(AEDesc * defaultLocation, NavReplyRecord * reply, NavDialogOptions * dialogOptions, NavEventUPP eventProc, OSType fileType, OSType fileCreator, void * callBackUD)
```

Trap: — (executor 实现，无 trap)

### NavServicesAvailable  

```c
Boolean NavServicesAvailable()
```

Trap: — (executor 实现，无 trap)

### NavServicesCanRun  

```c
Boolean NavServicesCanRun()
```

Trap: — (executor 实现，无 trap)

### NavTranslateFile  

```c
OSErr NavTranslateFile(NavReplyRecord * translateInfo, NavTranslationOptions howToTranslate)
```

Trap: — (executor 实现，无 trap)

### NavUnload  

```c
OSErr NavUnload()
```

Trap: — (executor 实现，无 trap)

## Typedefs

- **NavActionState** = uint32_t
- **NavCustomControlMessage** = int32_t
- **NavDialogOptionFlags** = uint32_t
- **NavAskDiscardChangesResult** = uint32_t
- **NavEventCallbackMessage** = uint32_t
- **NavSortKeyField** = uint16_t
- **NavPopupMenuItem** = uint16_t
- **NavFilterModes** = uint32_t
- **NavAskSaveChangesResult** = uint32_t
- **NavAskSaveChangesAction** = uint32_t
- **NavSortOrder** = uint16_t
- **NavTranslationOptions** = uint32_t
- **NavContext** = __navcontext*
- **NavCBRecPtr** = NavCBRec*
- **NavTypeListHandle** = NavTypeList**

## Enums

- **?**
- **?**
- **?**
- **?**

### Enum Values

**anonymous**:

- `kNavCBEvent` = 0
- `kNavCBCustomize` = ?
- `kNavCBStart` = ?
- `kNavCBTerminate` = ?
- `kNavCBAdjustRect` = ?
- `kNavCBNewLocation` = ?
- `kNavCBShowDesktop` = ?
- `kNavCBSelectEntry` = ?
- `kNavCBPopupMenuSelect` = ?
- `kNavCBAccept` = ?
- `kNavCBCancel` = ?
- `kNavCBAdjustPreview` = ?
- `kNavCBOpenSelection = 0x80000000` = ?

**anonymous**:

- `kNavReplyRecordVersion` = 0

**anonymous**:

- `kNavDialogOptionsVersion` = 0

**anonymous**:

- `kNavMenuItemSpecVersion` = 0

## Structs

- **__navcontext** {  }
- **NavReplyRecord** { version: uint16_t, validRecord: Boolean, replacing: Boolean, isStationery: Boolean, translationNeeded: Boolean, selection: AEDescList, keyScript: ScriptCode, fileTranslation: Handle }
- **NavDialogOptions** { version: uint16_t, dialogOptionFlags: NavDialogOptionFlags, location: Point, clientName: Str255, windowTitle: Str255, actionButtonLabel: Str255, cancelButtonLabel: Str255, savedFileName: Str255, message: Str255, preferenceKey: uint32_t, popupExtension: Handle }
- **NavMenuItemSpec** { version: uint16_t, menuCreator: OSType, menuType: OSType, menuItemName: Str255 }
- **NavFileOrFolderInfo** { version: uint16_t, isFolder: Boolean, visible: Boolean, creationDate: uint32_t, modificationDate: uint32_t, fileAndFolder: ? }
- **NavEventData** { eventDataParms: NavEventDataInfo, itemHit: int16_t }
- **NavCBRec** { version: uint16_t, context: NavContext, window: WindowPtr, customRect: Rect, previewRect: Rect, eventData: NavEventData }
- **NavTypeList** { componentSignature: OSType, reserved: int16_t, osTypeCount: int16_t, osType: OSType[1] }

## Unions

- **NavEventDataInfo** { event: EventRecord *, param: void * }

## Function Pointers

- **NavEventUPP** (callBackSelector: NavEventCallbackMessage, callbackParms: NavCBRecPtr, callBackUD: void*) -> void
- **NavPreviewUPP** (callbackParms: NavCBRecPtr, callBackUD: void*) -> void
- **NavObjectFilterUPP** (theItem: AEDesc *, info: void *, callBackUD: void*, filterMode: NavFilterModes) -> void

