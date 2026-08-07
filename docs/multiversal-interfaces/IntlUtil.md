# IntlUtil Interfaces

Source: `multiversal/defs/IntlUtil.yaml`

- Functions: **21**
- Typedefs: **7**
- Structs: **2**, Unions: **0**
- Enums: **6**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **1**
- Low-memory globals: **0**

## Functions

### ClearIntlResourceCache  

```c
void ClearIntlResourceCache()
```

Trap: — (executor 实现，无 trap) executor=C_

### GetIntlResource  

```c
Handle GetIntlResource(INTEGER id)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetIntlResourceTable  

```c
void GetIntlResourceTable(ScriptCode script, INTEGER tablecode, Handle* itlhandlep, LONGINT* offsetp, LONGINT* lengthp)
```

Trap: — (executor 实现，无 trap) executor=C_

### IUCompString  

```c
INTEGER IUCompString(ConstStringPtr str1, ConstStringPtr str2)
```

Trap: — (executor 实现，无 trap)

### IUDatePString  

```c
void IUDatePString(LONGINT date, DateForm form, StringPtr p, Handle h)
```

Trap: — (executor 实现，无 trap) executor=C_

### IUDateString  

```c
void IUDateString(LONGINT date, DateForm form, StringPtr p)
```

Trap: — (executor 实现，无 trap) executor=C_

### IUEqualString  

```c
INTEGER IUEqualString(ConstStringPtr str1, ConstStringPtr str2)
```

Trap: — (executor 实现，无 trap)

### IULDateString  

```c
void IULDateString(LongDateTime* datetimep, DateForm longflag, Str255 result, Handle intlhand)
```

Trap: — (executor 实现，无 trap) executor=C_

### IULTimeString  

```c
void IULTimeString(LongDateTime* datetimep, Boolean wantseconds, Str255 result, Handle intlhand)
```

Trap: — (executor 实现，无 trap) executor=C_

### IULangOrder  

```c
INTEGER IULangOrder(LangCode l1, LangCode l2)
```

Trap: — (executor 实现，无 trap) executor=C_

### IUMagIDPString  

```c
INTEGER IUMagIDPString(Ptr ptra, Ptr ptrb, INTEGER lena, INTEGER lenb, Handle itl2hand)
```

Trap: — (executor 实现，无 trap) executor=C_

### IUMagIDString  

```c
INTEGER IUMagIDString(Ptr ptr1, Ptr ptr2, INTEGER len1, INTEGER len2)
```

Trap: — (executor 实现，无 trap) executor=C_

### IUMagPString  

```c
INTEGER IUMagPString(Ptr ptra, Ptr ptrb, INTEGER lena, INTEGER lenb, Handle itl2hand)
```

Trap: — (executor 实现，无 trap) executor=C_

### IUMagString  

```c
INTEGER IUMagString(Ptr ptr1, Ptr ptr2, INTEGER len1, INTEGER len2)
```

Trap: — (executor 实现，无 trap) executor=C_

### IUMystery  

```c
void IUMystery(Ptr arg1, Ptr arg2, INTEGER arg3, INTEGER arg4)
```

Trap: — (executor 实现，无 trap) executor=C_

### IUScriptOrder  

```c
INTEGER IUScriptOrder(ScriptCode script1, ScriptCode script2)
```

Trap: — (executor 实现，无 trap) executor=C_

### IUTextOrder  

```c
INTEGER IUTextOrder(Ptr ptra, Ptr ptrb, INTEGER lena, INTEGER lenb, ScriptCode scripta, ScriptCode bscript, LangCode langa, LangCode langb)
```

Trap: — (executor 实现，无 trap) executor=C_

### IUTimePString  

```c
void IUTimePString(LONGINT date, Boolean secs, StringPtr p, Handle h)
```

Trap: — (executor 实现，无 trap) executor=C_

### IUTimeString  

```c
void IUTimeString(LONGINT date, Boolean secs, StringPtr p)
```

Trap: — (executor 实现，无 trap) executor=C_

### IsMetric  

```c
Boolean IsMetric()
```

Trap: — (executor 实现，无 trap) executor=C_

### SetIntlResource  

```c
void SetIntlResource(INTEGER rn, INTEGER id, Handle newh)
```

Trap: — (executor 实现，无 trap) executor=C_

## Typedefs

- **Intl0Ptr** = Intl0Rec*
- **Intl0Hndl** = Intl0Ptr*
- **STRING15** = Byte[16]
- **Intl1Ptr** = Intl1Rec*
- **Intl1Hndl** = Intl1Ptr*
- **LongDateTime** = comp
- **DateForm** = SignedByte

## Enums

- **?**
- **?**
- **?**
- **?**
- **?**
- **?**

### Enum Values

**anonymous**:

- `currSymLead` = 16
- `currNegSym` = 32
- `currTrailingZ` = 64
- `currLeadingZ` = 128

**anonymous**:

- `mdy` = 0
- `dmy` = 1
- `ymd` = 2

**anonymous**:

- `dayLdingZ` = 32
- `mntLdingZ` = 64
- `century` = 128

**anonymous**:

- `secLeadingZ` = 32
- `minLeadingZ` = 64
- `hrLeadingZ` = 128

**anonymous**:

- `verUS` = 0
- `verFrance` = 1
- `verBritain` = 2
- `verGermany` = 3
- `verItaly` = 4
- `verNetherlands` = 5
- `verBelgiumLux` = 6
- `verSweden` = 7
- `verSpain` = 8
- `verDenmark` = 9
- `verPortugal` = 10
- `verFrCanada` = 11
- `verNorway` = 12
- `verIsreal` = 13
- `verJapan` = 14
- `verAustralia` = 15
- `verArabia` = 16
- `verFinland` = 17
- `verFrSwiss` = 18
- `verGrSwiss` = 19
- `verGreece` = 20
- `verIceland` = 21
- `verMalta` = 22
- `verCyprus` = 23
- `verTurkey` = 24
- `verYugoslavia` = 25

**anonymous**:

- `shortDate` = 0
- `longDate` = 1
- `abbrevDate` = 2

## Structs

- **Intl0Rec** { decimalPt: Byte, thousSep: Byte, listSep: Byte, currSym1: Byte, currSym2: Byte, currSym3: Byte, currFmt: Byte, dateOrder: Byte, shrtDateFmt: Byte, dateSep: Byte, timeCycle: Byte, timeFmt: Byte, mornStr: LONGINT, eveStr: LONGINT, timeSep: Byte, time1Suff: Byte, time2Suff: Byte, time3Suff: Byte, time4Suff: Byte, time5Suff: Byte, time6Suff: Byte, time7Suff: Byte, time8Suff: Byte, metricSys: Byte, intl0Vers: INTEGER }
- **Intl1Rec** { days: STRING15[7], months: STRING15[12], suppressDay: Byte, lngDateFmt: Byte, dayLeading0: Byte, abbrLen: Byte, st0: LONGINT, st1: LONGINT, st2: LONGINT, st3: LONGINT, st4: LONGINT, intl1Vers: INTEGER, localRtn: INTEGER }

## Dispatchers

- **Pack6**—

