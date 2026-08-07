# SoundDvr Interfaces

SoundDvr IMII-232 (true-b);

Source: `multiversal/defs/SoundDvr.yaml`

- Functions: **5**
- Typedefs: **8**
- Structs: **5**, Unions: **0**
- Enums: **1**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **0**
- Low-memory globals: **5**

## Functions

### GetSoundVol  

```c
void GetSoundVol(INTEGER* volp)
```

Trap: — (executor 实现，无 trap)

### SetSoundVol  

```c
void SetSoundVol(INTEGER vol)
```

Trap: — (executor 实现，无 trap)

### SoundDone  

```c
Boolean SoundDone()
```

Trap: — (executor 实现，无 trap)

### StartSound  

```c
void StartSound(Ptr srec, LONGINT nb, ProcPtr comp)
```

Trap: — (executor 实现，无 trap)

### StopSound  

```c
void StopSound()
```

Trap: — (executor 实现，无 trap)

## Typedefs

- **FreeWave** = Byte[30001]
- **FFSynthPtr** = FFSynthRec*
- **Tones** = Tone[5001]
- **SWSynthPtr** = SWSynthRec*
- **Wave** = Byte[256]
- **WavePtr** = Wave*
- **FTSndRecPtr** = FTSoundRec*
- **FTsynthPtr** = FTSynthRec*

## Enums

- **?**

### Enum Values

**anonymous**:

- `swMode` = -1
- `ftMode` = 1
- `ffMode` = 0

## Structs

- **FFSynthRec** { mode: INTEGER, fcount: Fixed, waveBytes: FreeWave }
- **Tone** { tcount: INTEGER, amplitude: INTEGER, tduration: INTEGER }
- **SWSynthRec** { mode: INTEGER, triplets: Tones }
- **FTSoundRec** { fduration: INTEGER, sound1Rate: Fixed, sound1Phase: LONGINT, sound2Rate: Fixed, sound2Phase: LONGINT, sound3Rate: Fixed, sound3Phase: LONGINT, sound4Rate: Fixed, sound4Phase: LONGINT, sound1Wave: WavePtr, sound2Wave: WavePtr, sound3Wave: WavePtr, sound4Wave: WavePtr }
- **FTSynthRec** { mode: INTEGER, sndRec: FTSndRecPtr }

## Low Memory Globals

- **SdVolume** @ 0x260 (Byte) — SoundDvr IMII-232 (true-b);
- **SoundPtr** @ 0x262 (FTSndRecPtr) — SoundDvr IMII-227 (false);
- **SoundBase** @ 0x266 (Ptr) — SoundDvr IMIII-21 (true-b);
- **SoundLevel** @ 0x27F (Byte) — SoundDvr IMII-234 (false);
- **CurPitch** @ 0x280 (INTEGER) — SoundDvr IMII-226 (true-b);
