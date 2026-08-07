# EditionMgr Interfaces

Source: `multiversal/defs/EditionMgr.yaml`

- Functions: **30**
- Typedefs: **20**
- Structs: **15**, Unions: **0**
- Enums: **3**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **1**
- Low-memory globals: **0**

## Functions

### AssociateSection  

```c
OSErr AssociateSection(SectionHandle section, FSSpecPtr new_section_doc)
```

Trap: — (executor 实现，无 trap) executor=C_

### CallEditionOpenerProc  

```c
OSErr CallEditionOpenerProc(EditionOpenerVerb selector, EditionOpenerParamBlock* param_block, EditionOpenerUPP opener)
```

Trap: — (executor 实现，无 trap) executor=C_

### CallFormatIOProc  

```c
OSErr CallFormatIOProc(FormatIOVerb selector, FormatIOParamBlock* param_block, FormatIOUPP proc)
```

Trap: — (executor 实现，无 trap) executor=C_

### CloseEdition  

```c
OSErr CloseEdition(EditionRefNum edition, Boolean success_p)
```

Trap: — (executor 实现，无 trap) executor=C_

### CreateEditionContainerFile  

```c
OSErr CreateEditionContainerFile(FSSpecPtr edition_file, OSType creator, ScriptCode edition_file_name_script)
```

Trap: — (executor 实现，无 trap) executor=C_

### DeleteEditionContainerFile  

```c
OSErr DeleteEditionContainerFile(FSSpecPtr edition_file)
```

Trap: — (executor 实现，无 trap) executor=C_

### EditionHasFormat  

```c
OSErr EditionHasFormat(EditionRefNum edition, FormatType format, Size* format_size)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetEditionFormatMark  

```c
OSErr GetEditionFormatMark(EditionRefNum edition, FormatType format, int32_t* currentMark)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetEditionInfo  

```c
OSErr GetEditionInfo(SectionHandle section, EditionInfoPtr edition_info)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetEditionOpenerProc  

```c
OSErr GetEditionOpenerProc(EditionOpenerUPP* opener)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetLastEditionContainerUsed  

```c
OSErr GetLastEditionContainerUsed(EditionContainerSpecPtr container)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetStandardFormats  

```c
OSErr GetStandardFormats(EditionContainerSpecPtr container, FormatType* preview_format, Handle preview, Handle publisher_alias, Handle formats)
```

Trap: — (executor 实现，无 trap) executor=C_

### GoToPublisherSection  

```c
OSErr GoToPublisherSection(EditionContainerSpecPtr container)
```

Trap: — (executor 实现，无 trap) executor=C_

### InitEditionPackVersion  

```c
OSErr InitEditionPackVersion(INTEGER unused)
```

Trap: — (executor 实现，无 trap) executor=C_

### IsRegisteredSection  

```c
OSErr IsRegisteredSection(SectionHandle section)
```

Trap: — (executor 实现，无 trap) executor=C_

### NewPublisherDialog  

```c
OSErr NewPublisherDialog(NewSubscriberReplyPtr reply)
```

Trap: — (executor 实现，无 trap) executor=C_

### NewPublisherExpDialog  

```c
OSErr NewPublisherExpDialog(NewPublisherReplyPtr reply, Point where, int16_t expnasion_ditl_res_id, ExpDialogHookUPP dialog_hook, ExpModalFilterUPP filter_hook, Ptr data)
```

Trap: — (executor 实现，无 trap) executor=C_

### NewSection  

```c
OSErr NewSection(EditionContainerSpecPtr container, FSSpecPtr section_doc, SectionType kind, int32_t section_id, UpdateMode initial_mode, SectionHandle* section_out)
```

Trap: — (executor 实现，无 trap) executor=C_

### NewSubscriberDialog  

```c
OSErr NewSubscriberDialog(NewSubscriberReplyPtr reply)
```

Trap: — (executor 实现，无 trap) executor=C_

### NewSubscriberExpDialog  

```c
OSErr NewSubscriberExpDialog(NewSubscriberReplyPtr reply, Point where, int16_t expnasion_ditl_res_id, ExpDialogHookUPP dialog_hook, ExpModalFilterUPP filter_hook, Ptr data)
```

Trap: — (executor 实现，无 trap) executor=C_

### OpenEdition  

```c
OSErr OpenEdition(SectionHandle subscriber_section, EditionRefNum* ref_num)
```

Trap: — (executor 实现，无 trap) executor=C_

### OpenNewEdition  

```c
OSErr OpenNewEdition(SectionHandle publisher_section, OSType creator, FSSpecPtr publisher_section_doc, EditionRefNum* ref_num)
```

Trap: — (executor 实现，无 trap) executor=C_

### ReadEdition  

```c
OSErr ReadEdition(EditionRefNum edition, FormatType format, Ptr buffer, Size buffer_size)
```

Trap: — (executor 实现，无 trap) executor=C_

### RegisterSection  

```c
OSErr RegisterSection(FSSpecPtr section_doc, SectionHandle section, Boolean* alias_was_updated_p_out)
```

Trap: — (executor 实现，无 trap) executor=C_

### SectionOptionsDialog  

```c
OSErr SectionOptionsDialog(SectionOptionsReply* reply)
```

Trap: — (executor 实现，无 trap) executor=C_

### SectionOptionsExpDialog  

```c
OSErr SectionOptionsExpDialog(SectionOptionsReply* reply, Point where, int16_t expnasion_ditl_res_id, ExpDialogHookUPP dialog_hook, ExpModalFilterUPP filter_hook, Ptr data)
```

Trap: — (executor 实现，无 trap) executor=C_

### SetEditionFormatMark  

```c
OSErr SetEditionFormatMark(EditionRefNum edition, FormatType format, int32_t mark)
```

Trap: — (executor 实现，无 trap) executor=C_

### SetEditionOpenerProc  

```c
OSErr SetEditionOpenerProc(EditionOpenerUPP opener)
```

Trap: — (executor 实现，无 trap) executor=C_

### UnRegisterSection  

```c
OSErr UnRegisterSection(SectionHandle section)
```

Trap: — (executor 实现，无 trap) executor=C_

### WriteEdition  

```c
OSErr WriteEdition(EditionRefNum edition, FormatType format, Ptr buffer, Size buffer_size)
```

Trap: — (executor 实现，无 trap) executor=C_

## Typedefs

- **TimeStamp** = int32_t
- **EditionRefNum** = Handle
- **UpdateMode** = int16_t
- **SectionType** = SignedByte
- **FormatType** = char[4]
- **ExpDialogHookUPP** = ProcPtr
- **ExpModalFilterUPP** = ProcPtr
- **FormatIOUPP** = ProcPtr
- **EditionOpenerUPP** = ProcPtr
- **SectionPtr** = SectionRecord*
- **SectionHandle** = SectionPtr*
- **EditionContainerSpecPtr** = EditionContainerSpec*
- **EditionInfoPtr** = EditionInfoRecord*
- **NewPublisherReplyPtr** = NewPublisherReply*
- **NewSubscriberReplyPtr** = NewSubscriberReply*
- **SectionOptionsReplyPtr** = SectionOptionsReply*
- **EditionOpenerVerb** = uint8_t
- **EditionOpenerParamBlockPtr** = EditionOpenerParamBlock*
- **FormatIOVerb** = uint8_t
- **FormatIOParamBlockPtr** = FormatIOParamBlock*

## Enums

- **?**
- **?**
- **?**

### Enum Values

**anonymous**:

- `eoOpen` = 0
- `eoClose` = 1
- `eoOpenNew` = 2
- `eoCloseNew` = 3
- `eoCanSubscribe` = 4

**anonymous**:

- `ioHasFormat` = 0
- `ioReadFormat` = 1
- `ioNewFormat` = 2
- `ioWtriteFormat` = 3

**anonymous**:

- `flLckedErr` = -45
- `fBusyErr` = -47
- `userCanceledErr` = -128
- `editionMgrInitErr` = -450
- `badSectionErr` = -451
- `notRegisteredSectionErr` = -452
- `badSubPartErr` = -454
- `multiplePubliserWrn` = -460
- `containerNotFoundWrn` = -461
- `notThePublisherWrn` = -463

## Structs

- **SectionRecord** { version: SignedByte, kind: SectionType, mode: UpdateMode, mdDate: TimeStamp, sectionID: int32_t, refCon: int32_t, alias: AliasHandle, subPart: int32_t, nextSection: Handle, controlBlock: Handle, refNum: EditionRefNum }
- **EditionContainerSpec** { theFile: FSSpec, theFileScript: ScriptCode, thePart: int32_t, thePartName: Str31, thePartScript: ScriptCode }
- **EditionContainerSpec** {  }
- **EditionInfoRecord** { crDate: TimeStamp, mdDate: TimeStamp, fdCreator: OSType, fdType: OSType, container: EditionContainerSpec }
- **EditionInfoRecord** {  }
- **NewPublisherReply** { canceled: Boolean, replacing: Boolean, usePart: Boolean, _filler: uint8_t, preview: Handle, previewFormat: FormatType, container: EditionContainerSpec }
- **NewPublisherReply** {  }
- **NewSubscriberReply** { canceled: Boolean, formatsMask: SignedByte, container: EditionContainerSpec }
- **NewSubscriberReply** {  }
- **SectionOptionsReply** { canceled: Boolean, changed: Boolean, sectionH: SectionHandle, action: ResType }
- **SectionOptionsReply** {  }
- **EditionOpenerParamBlock** { info: EditionInfoRecord, sectionH: SectionHandle, document: FSSpecPtr, fdCreator: OSType, ioRefNum: int32_t, ioProc: FormatIOUPP, success: Boolean, formatsMask: SignedByte }
- **EditionOpenerParamBlock** {  }
- **FormatIOParamBlock** { ioRefNum: int32_t, format: FormatType, formatIndex: int32_t, offset: int32_t, buffPtr: Ptr, buffLen: int32_t }
- **FormatIOParamBlock** {  }

## Dispatchers

- **Pack11**—

