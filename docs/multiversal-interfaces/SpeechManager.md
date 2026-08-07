# SpeechManager Interfaces

Source: `multiversal/defs/SpeechManager.yaml`

- Functions: **25**
- Typedefs: **2**
- Structs: **9**, Unions: **0**
- Enums: **0**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **0**
- Low-memory globals: **0**

## Functions

### ContinueSpeech  

```c
OSErr ContinueSpeech(SpeechChannel chan)
```

Trap: — (executor 实现，无 trap) executor=C_

### CountVoices  

```c
OSErr CountVoices(int16_t* numVoices)
```

Trap: — (executor 实现，无 trap) executor=C_

### DisposeSpeechChannel  

```c
OSErr DisposeSpeechChannel(SpeechChannel chan)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetIndVoice  

```c
OSErr GetIndVoice(int16_t index, VoiceSpec* voice)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetSpeechInfo  

```c
OSErr GetSpeechInfo(SpeechChannel chan, OSType selector, void* speechInfo)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetSpeechPitch  

```c
OSErr GetSpeechPitch(SpeechChannel chan, Fixed* pitch)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetSpeechRate  

```c
OSErr GetSpeechRate(SpeechChannel chan, Fixed* rate)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetVoiceDescription  

```c
OSErr GetVoiceDescription(const VoiceSpec* voice, VoiceDescription* info, LONGINT infoLength)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetVoiceInfo  

```c
OSErr GetVoiceInfo(const VoiceSpec* voice, OSType selector, void* voiceInfo)
```

Trap: — (executor 实现，无 trap) executor=C_

### MakeVoiceSpec  

```c
OSErr MakeVoiceSpec(OSType creator, OSType id, VoiceSpec* voice)
```

Trap: — (executor 实现，无 trap) executor=C_

### NewSpeechChannel  

```c
OSErr NewSpeechChannel(VoiceSpec* voice, SpeechChannel* chan)
```

Trap: — (executor 实现，无 trap) executor=C_

### PauseSpeechAt  

```c
OSErr PauseSpeechAt(SpeechChannel chan, int32_t whereToPause)
```

Trap: — (executor 实现，无 trap) executor=C_

### SetSpeechInfo  

```c
OSErr SetSpeechInfo(SpeechChannel chan, OSType selector, const void* speechInfo)
```

Trap: — (executor 实现，无 trap) executor=C_

### SetSpeechPitch  

```c
OSErr SetSpeechPitch(SpeechChannel chan, Fixed pitch)
```

Trap: — (executor 实现，无 trap) executor=C_

### SetSpeechRate  

```c
OSErr SetSpeechRate(SpeechChannel chan, Fixed rate)
```

Trap: — (executor 实现，无 trap) executor=C_

### SpeakBuffer  

```c
OSErr SpeakBuffer(SpeechChannel chan, const void* textBuf, ULONGINT textBytes, int32_t controlFlags)
```

Trap: — (executor 实现，无 trap) executor=C_

### SpeakString  

```c
OSErr SpeakString(ConstStringPtr textToBeSpoken)
```

Trap: — (executor 实现，无 trap) executor=C_

### SpeakText  

```c
OSErr SpeakText(SpeechChannel chan, const void* textBuf, ULONGINT textBytes)
```

Trap: — (executor 实现，无 trap) executor=C_

### SpeechBusy  

```c
int16_t SpeechBusy()
```

Trap: — (executor 实现，无 trap) executor=C_

### SpeechBusySystemWide  

```c
int16_t SpeechBusySystemWide()
```

Trap: — (executor 实现，无 trap) executor=C_

### SpeechManagerVersion  

```c
NumVersion SpeechManagerVersion()
```

Trap: — (executor 实现，无 trap) executor=C_

### StopSpeech  

```c
OSErr StopSpeech(SpeechChannel chan)
```

Trap: — (executor 实现，无 trap) executor=C_

### StopSpeechAt  

```c
OSErr StopSpeechAt(SpeechChannel chan, int32_t whereToStop)
```

Trap: — (executor 实现，无 trap) executor=C_

### TextToPhonemes  

```c
OSErr TextToPhonemes(SpeechChannel chan, const void* textBuf, ULONGINT textBytes, Handle phonemeBuf, LONGINT* phonemeBytes)
```

Trap: — (executor 实现，无 trap) executor=C_

### UseDictionary  

```c
OSErr UseDictionary(SpeechChannel chan, Handle dictionary)
```

Trap: — (executor 实现，无 trap) executor=C_

## Typedefs

- **VoiceSpecPtr** = VoiceSpec*
- **SpeechChannel** = SpeechChannelRecord*

## Structs

- **VoiceSpec** { creator: OSType, id: OSType }
- **VoiceFileInfo** { fileSpec: FSSpec, resID: uint16_t }
- **SpeechStatusInfo** { outputBusy: Boolean, outputPaused: Boolean, inputBytesLeft: int32_t, phonemeCode: int16_t }
- **VoiceDescription** { length: int32_t, voice: VoiceSpec, version: int32_t, name: Str63, comment: Str255, gender: int16_t, age: int16_t, script: int16_t, language: int16_t, region: int16_t, reserved: int32_t[4] }
- **SpeechChannelRecord** { data: LONGINT[1] }
- **PhonemeInfo** { opcode: int16_t, phStr: Str15, exampleStr: Str31, hiliteStart: int16_t, hiliteEnd: int16_t }
- **PhonemeDescriptor** { phonemeCount: int16_t, thePhonemes: PhonemeInfo[1] }
- **SpeechXtndData** { synthCreator: OSType, synthData: Byte[2] }
- **DelimiterInfo** { startDelimiter: Byte[2], endDelimiter: Byte[2] }

