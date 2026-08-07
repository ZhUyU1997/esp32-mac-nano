# SoundMgr Interfaces

SoundDvr MPW (true);

Source: `multiversal/defs/SoundMgr.yaml`

- Functions: **56**
- Typedefs: **15**
- Structs: **9**, Unions: **0**
- Enums: **7**
- Function pointers: **2**
- Common blocks: **0**
- Dispatchers: **1**
- Low-memory globals: **1**

## Functions

### Comp3to1  

```c
void Comp3to1(Ptr inp, Ptr outp, LONGINT cnt, Ptr instatep, Ptr outstatep, LONGINT numchannels, LONGINT whichchannel)
```

Trap: — (executor 实现，无 trap) executor=C_

### Comp6to1  

```c
void Comp6to1(Ptr inp, Ptr outp, LONGINT cnt, Ptr instatep, Ptr outstatep, LONGINT numchannels, LONGINT whichchannel)
```

Trap: — (executor 实现，无 trap) executor=C_

### DirectorUnknown3  

```c
LONGINT DirectorUnknown3()
```

Trap: — (executor 实现，无 trap) executor=C_

### DirectorUnknown4  

```c
INTEGER DirectorUnknown4(ResType ?, INTEGER ?, Ptr ?, Ptr ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### Exp1to3  

```c
void Exp1to3(Ptr inp, Ptr outp, LONGINT cnt, Ptr instatep, Ptr outstatep, LONGINT numchannels, LONGINT whichchannel)
```

Trap: — (executor 实现，无 trap) executor=C_

### Exp1to6  

```c
void Exp1to6(Ptr inp, Ptr outp, LONGINT cnt, Ptr instatep, Ptr outstatep, LONGINT numchannels, LONGINT whichchannel)
```

Trap: — (executor 实现，无 trap) executor=C_

### FinaleUnknown1  

```c
void FinaleUnknown1()
```

Trap: — (executor 实现，无 trap) executor=C_

### FinaleUnknown2  

```c
OSErr FinaleUnknown2(ResType ?, LONGINT ?, Ptr ?, Ptr ?)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetCompressionInfo  

```c
OSErr GetCompressionInfo(INTEGER compressionID, OSType format, INTEGER numChannels, INTEGER sampleSize, CompressionInfoPtr cp)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetDefaultOutputVolume  

```c
OSErr GetDefaultOutputVolume(LONGINT* levelp)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetSoundHeaderOffset  

```c
OSErr GetSoundHeaderOffset(Handle sndHandle, LONGINT* offset)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetSoundPreference  

```c
OSErr GetSoundPreference(OSType theType, ConstStringPtr name, Handle settings)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetSysBeepVolume  

```c
OSErr GetSysBeepVolume(LONGINT* levelp)
```

Trap: — (executor 实现，无 trap) executor=C_

### MACEVersion  

```c
NumVersion MACEVersion()
```

Trap: — (executor 实现，无 trap) executor=C_

### SPBBytesToMilliseconds  

```c
OSErr SPBBytesToMilliseconds(LONGINT refnum, LONGINT* bytecountp)
```

Trap: — (executor 实现，无 trap) executor=C_

### SPBCloseDevice  

```c
OSErr SPBCloseDevice(LONGINT inrefnum)
```

Trap: — (executor 实现，无 trap) executor=C_

### SPBGetDeviceInfo  

```c
OSErr SPBGetDeviceInfo(LONGINT refnum, OSType info, Ptr infop)
```

Trap: — (executor 实现，无 trap) executor=C_

### SPBGetIndexedDevice  

```c
OSErr SPBGetIndexedDevice(INTEGER count, ConstStringPtr name, Handle* deviceiconhandlep)
```

Trap: — (executor 实现，无 trap) executor=C_

### SPBGetRecordingStatus  

```c
OSErr SPBGetRecordingStatus(LONGINT refnum, INTEGER* recordingstatus, INTEGER* meterlevel, LONGINT* totalsampstorecord, LONGINT* numsampsrecorded, LONGINT* totalmsecstorecord, LONGINT* numbermsecsrecorded)
```

Trap: — (executor 实现，无 trap) executor=C_

### SPBMillisecondsToBytes  

```c
OSErr SPBMillisecondsToBytes(LONGINT refnum, LONGINT* millip)
```

Trap: — (executor 实现，无 trap) executor=C_

### SPBOpenDevice  

```c
OSErr SPBOpenDevice(ConstStringPtr name, INTEGER permission, LONGINT* inrefnump)
```

Trap: — (executor 实现，无 trap) executor=C_

### SPBPauseRecording  

```c
OSErr SPBPauseRecording(LONGINT refnum)
```

Trap: — (executor 实现，无 trap) executor=C_

### SPBRecord  

```c
OSErr SPBRecord(SPBPtr inparamp, Boolean async)
```

Trap: — (executor 实现，无 trap) executor=C_

### SPBRecordToFile  

```c
OSErr SPBRecordToFile(INTEGER refnum, SPBPtr inparamp, Boolean async)
```

Trap: — (executor 实现，无 trap) executor=C_

### SPBResumeRecording  

```c
OSErr SPBResumeRecording(LONGINT refnum)
```

Trap: — (executor 实现，无 trap) executor=C_

### SPBSetDeviceInfo  

```c
OSErr SPBSetDeviceInfo(LONGINT refnum, OSType info, Ptr infop)
```

Trap: — (executor 实现，无 trap) executor=C_

### SPBSignInDevice  

```c
OSErr SPBSignInDevice(INTEGER refnum, ConstStringPtr name)
```

Trap: — (executor 实现，无 trap) executor=C_

### SPBSignOutDevice  

```c
OSErr SPBSignOutDevice(INTEGER refnum)
```

Trap: — (executor 实现，无 trap) executor=C_

### SPBStopRecording  

```c
OSErr SPBStopRecording(LONGINT refnum)
```

Trap: — (executor 实现，无 trap) executor=C_

### SPBVersion  

```c
NumVersion SPBVersion()
```

Trap: — (executor 实现，无 trap) executor=C_

### SetDefaultOutputVolume  

```c
OSErr SetDefaultOutputVolume(LONGINT level)
```

Trap: — (executor 实现，无 trap) executor=C_

### SetSoundPreference  

```c
OSErr SetSoundPreference(OSType theType, ConstStringPtr name, Handle settings)
```

Trap: — (executor 实现，无 trap) executor=C_

### SetSysBeepVolume  

```c
OSErr SetSysBeepVolume(LONGINT level)
```

Trap: — (executor 实现，无 trap) executor=C_

### SetupAIFFHeader  

```c
OSErr SetupAIFFHeader(INTEGER refnum, INTEGER numchannels, Fixed samplerate, INTEGER samplesize, OSType compression, LONGINT numbytes, LONGINT numframes)
```

Trap: — (executor 实现，无 trap) executor=C_

### SetupSndHeader  

```c
OSErr SetupSndHeader(Handle sndhandle, INTEGER numchannels, Fixed rate, INTEGER size, OSType compresion, INTEGER basefreq, LONGINT numbytes, INTEGER* headerlenp)
```

Trap: — (executor 实现，无 trap) executor=C_

### SndAddModifier  

```c
OSErr SndAddModifier(SndChannelPtr chanp, ProcPtr mod, INTEGER id, LONGINT init)
```

Trap: `0xA802` executor=C_

### SndChannelStatus  

```c
OSErr SndChannelStatus(SndChannelPtr chanp, INTEGER length, SCStatusPtr statusp)
```

Trap: — (executor 实现，无 trap) executor=C_

### SndControl  

```c
OSErr SndControl(INTEGER id, SndCommand* cmdp)
```

Trap: `0xA806` executor=C_

### SndDisposeChannel  

```c
OSErr SndDisposeChannel(SndChannelPtr chanp, Boolean quitnow)
```

Trap: `0xA801` executor=C_

### SndDoCommand  

```c
OSErr SndDoCommand(SndChannelPtr chanp, SndCommand* cmdp, Boolean nowait)
```

Trap: `0xA803` executor=C_

### SndDoImmediate  

```c
OSErr SndDoImmediate(SndChannelPtr chanp, SndCommand* cmdp)
```

Trap: `0xA804` executor=C_

### SndGetInfo  

```c
OSErr SndGetInfo(SndChannelPtr chan, OSType selector, void* infoPtr)
```

Trap: — (executor 实现，无 trap) executor=C_

### SndGetSysBeepState  

```c
void SndGetSysBeepState(INTEGER* statep)
```

Trap: — (executor 实现，无 trap) executor=C_

### SndManagerStatus  

```c
OSErr SndManagerStatus(INTEGER length, SMStatusPtr statusp)
```

Trap: — (executor 实现，无 trap) executor=C_

### SndNewChannel  

```c
OSErr SndNewChannel(SndChannelPtr* chanpp, INTEGER synth, LONGINT init, SndCallbackUPP userroutinep)
```

Trap: `0xA807` executor=C_

### SndPauseFilePlay  

```c
OSErr SndPauseFilePlay(SndChannelPtr chanp)
```

Trap: — (executor 实现，无 trap) executor=C_

### SndPlay  

```c
OSErr SndPlay(SndChannelPtr chanp, Handle sndh, Boolean async)
```

Trap: `0xA805` executor=C_

### SndPlayDoubleBuffer  

```c
OSErr SndPlayDoubleBuffer(SndChannelPtr chanp, SndDoubleBufferHeaderPtr paramp)
```

Trap: — (executor 实现，无 trap) executor=C_

### SndRecord  

```c
OSErr SndRecord(ProcPtr filterp, Point corner, OSType quality, Handle* sndhandlep)
```

Trap: — (executor 实现，无 trap) executor=C_

### SndRecordToFile  

```c
OSErr SndRecordToFile(ProcPtr filterp, Point corner, OSType quality, INTEGER refnum)
```

Trap: — (executor 实现，无 trap) executor=C_

### SndSetInfo  

```c
OSErr SndSetInfo(SndChannelPtr chan, OSType selector, void* infoPtr)
```

Trap: — (executor 实现，无 trap) executor=C_

### SndSetSysBeepState  

```c
OSErr SndSetSysBeepState(INTEGER state)
```

Trap: — (executor 实现，无 trap) executor=C_

### SndSoundManagerVersion  

```c
NumVersion SndSoundManagerVersion()
```

Trap: — (executor 实现，无 trap) executor=C_

### SndStartFilePlay  

```c
OSErr SndStartFilePlay(SndChannelPtr chanp, INTEGER refnum, INTEGER resnum, LONGINT buffersize, Ptr bufferp, AudioSelectionPtr theselectionp, ProcPtr completionp, Boolean async)
```

Trap: — (executor 实现，无 trap) executor=C_

### SndStopFilePlay  

```c
OSErr SndStopFilePlay(SndChannelPtr chanp, Boolean async)
```

Trap: — (executor 实现，无 trap) executor=C_

### UnsignedFixedMulDiv  

```c
UnsignedFixed UnsignedFixedMulDiv(UnsignedFixed value, UnsignedFixed multiplier, UnsignedFixed divisor)
```

Trap: — (executor 实现，无 trap) executor=C_

## Typedefs

- **SndChannelPtr** = SndChannel*
- **SoundHeader** = _SoundHeader
- **SoundHeaderPtr** = _SoundHeader*
- **ExtSoundHeader** = _ExtSoundHeader
- **ExtSoundHeaderPtr** = _ExtSoundHeader*
- **SndDoubleBufferPtr** = SndDoubleBuffer*
- **SndDoubleBufferHeaderPtr** = SndDoubleBufferHeader*
- **SCStatus** = _SCSTATUS
- **SCStatusPtr** = _SCSTATUS*
- **SMStatusPtr** = void*
- **NumVersion** = LONGINT
- **AudioSelectionPtr** = void*
- **SPBPtr** = void*
- **UnsignedFixed** = uint32_t
- **CompressionInfoPtr** = Ptr

## Enums

- **?**
- **?**
- **?**
- **?**
- **?**
- **?**
- **?**

### Enum Values

**anonymous**:

- `stdQLength` = 128

**anonymous**:

- `stdSH` = 0
- `cmpSH` = 254
- `extSH` = 255

**anonymous**:

- `nullCmd` = ?
- `initCmd` = ?
- `freeCmd` = ?
- `quietCmd` = ?
- `flushCmd` = ?
- `waitCmd` = 10
- `pauseCmd` = ?
- `resumeCmd` = ?
- `callBackCmd` = ?
- `syncCmd` = ?
- `emptyCmd` = ?
- `tickleCmd` = 20
- `requestNextCmd` = ?
- `howOftenCmd` = ?
- `wakeUpCmd` = ?
- `availableCmd` = ?
- `noteCmd` = 40
- `restCmd` = ?
- `freqCmd` = ?
- `ampCmd` = ?
- `timbreCmd` = ?
- `waveTableCmd` = 60
- `phaseCmd` = ?
- `soundCmd` = 80
- `bufferCmd` = ?
- `rateCmd` = ?
- `midiDataCmd` = 100

**anonymous**:

- `noteSynth` = 1
- `waveTableSynth` = 3
- `sampledSynth` = 5
- `MIDISynthIn` = 7
- `MIDISynthOut` = 9

**anonymous**:

- `badChannel` = -205
- `badFormat` = -206
- `noHardware` = -200
- `notEnoughHardware` = -201
- `queueFull` = -203
- `resProblem` = -204

**anonymous**:

- `soundactiveoff` = 0
- `soundactive5` = 5
- `soundactiveinplay` = 129
- `soundactivenone` = 255

**anonymous**:

- `dbBufferReady` = 1
- `dbLastBuffer` = 4

## Structs

- **SndCommand** { cmd: INTEGER, param1: INTEGER, param2: LONGINT }
- **SndChannel** {  }
- **SndChannel** { nextChan: SndChannel*, firstMod: Ptr, callBack: SndCallbackUPP, userInfo: LONGINT, wait: LONGINT, cmdInProg: SndCommand, flags: INTEGER, qLength: INTEGER, qHead: INTEGER, qTail: INTEGER, queue: SndCommand[stdQLength] }
- **soundbuffer_t** { offset: LONGINT, nsamples: LONGINT, rate: LONGINT, altbegin: LONGINT, altend: LONGINT, basenote: INTEGER, buf: uint8_t[1] }
- **_SoundHeader** { samplePtr: Ptr, length: LONGINT, sampleRate: Fixed, loopStart: LONGINT, loopEnd: LONGINT, encode: Byte, baseFrequency: Byte, sampleArea: Byte[1] }
- **_ExtSoundHeader** { samplePtr: Ptr, numChannels: LONGINT, sampleRate: Fixed, loopStart: LONGINT, loopEnd: LONGINT, encode: Byte, baseFrequency: Byte, numFrames: LONGINT, AIFFSampleRate: extended80, MarkerChunk: Ptr, instrumentChunks: Ptr, AESRecording: Ptr, sampleSize: INTEGER, futureUse1: INTEGER, futureUse2: LONGINT, futureUse3: LONGINT, futureUse4: LONGINT, sampleArea: Byte[1] }
- **SndDoubleBuffer** { dbNumFrames: LONGINT, dbFlags: LONGINT, dbUserInfo: LONGINT[2], dbSoundData: Byte[1] }
- **SndDoubleBufferHeader** { dbhNumChannels: INTEGER, dbhSampleSize: INTEGER, dbhCompressionID: INTEGER, dbhPacketSize: INTEGER, dbhSampleRate: Fixed, dbhBufferPtr: SndDoubleBufferPtr[2], dbhDoubleBack: SndDoubleBackUPP }
- **_SCSTATUS** { scStartTime: Fixed, scEndTime: Fixed, scCurrentTime: Fixed, scChannelBusy: Boolean, scChannelDisposed: Boolean, scChannelPaused: Boolean, scUnused: Boolean, scChannelAttributes: LONGINT, scCPULoad: LONGINT }

## Function Pointers

- **SndCallbackUPP** (?: SndChannel*, ?: SndCommand*) -> void
- **SndDoubleBackUPP** (?: SndChannelPtr, ?: SndDoubleBufferPtr) -> void

## Dispatchers

- **SoundDispatch**—

## Low Memory Globals

- **SoundActive** @ 0x27E (Byte) — SoundDvr MPW (true);
