# MenuMgr Interfaces

MenuMgr IMV-249 (true);

Source: `multiversal/defs/MenuMgr.yaml`

- Functions: **51**
- Typedefs: **7**
- Structs: **2**, Unions: **0**
- Enums: **3**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **0**
- Low-memory globals: **13**

## Functions

### AppendMenu  

```c
void AppendMenu(MenuHandle mh, ConstStringPtr str)
```

Trap: `0xA933` executor=C_

### AppendResMenu  

```c
void AppendResMenu(MenuHandle mh, ResType restype)
```

Trap: `0xA94D` executor=C_

### CalcMenuSize  

```c
void CalcMenuSize(MenuHandle mh)
```

Trap: `0xA948` executor=C_

### CheckItem  

```c
void CheckItem(MenuHandle mh, INTEGER item, Boolean cflag)
```

Trap: `0xA945` executor=C_

### CheckMenuItem  

```c
void CheckMenuItem(MenuHandle mh, INTEGER item, Boolean cflag)
```

Trap: — (executor 实现，无 trap)

### ClearMenuBar  

```c
void ClearMenuBar()
```

Trap: `0xA934` executor=C_

### CountMItems  

```c
INTEGER CountMItems(MenuHandle mh)
```

Trap: `0xA950` executor=C_

### DeleteMCEntries  

```c
void DeleteMCEntries(INTEGER ?, INTEGER ?)
```

Trap: `0xAA60` executor=C_

### DeleteMenu  

```c
void DeleteMenu(INTEGER mid)
```

Trap: `0xA936` executor=C_

### DeleteMenuItem  

```c
void DeleteMenuItem(MenuHandle mh, INTEGER item)
```

Trap: `0xA952` executor=C_

### DisableItem  

```c
void DisableItem(MenuHandle mh, INTEGER item)
```

Trap: `0xA93A` executor=C_

### DisposeMCInfo  

```c
void DisposeMCInfo(MCTableHandle ?)
```

Trap: `0xAA63` executor=C_

### DisposeMenu  

```c
void DisposeMenu(MenuHandle mh)
```

Trap: `0xA932` executor=C_

### DrawMenuBar  

```c
void DrawMenuBar()
```

Trap: `0xA937` executor=C_

### EnableItem  

```c
void EnableItem(MenuHandle mh, INTEGER item)
```

Trap: `0xA939` executor=C_

### FlashMenuBar  

```c
void FlashMenuBar(INTEGER mid)
```

Trap: `0xA94C` executor=C_

### GetItemCmd  

```c
void GetItemCmd(MenuHandle mh, INTEGER item, CharParameter* cmdp)
```

Trap: `0xA84E` executor=C_

### GetItemIcon  

```c
void GetItemIcon(MenuHandle mh, INTEGER item, INTEGER* iconp)
```

Trap: `0xA93F` executor=C_

### GetItemMark  

```c
void GetItemMark(MenuHandle mh, INTEGER item, INTEGER* markp)
```

Trap: `0xA943` executor=C_

### GetItemStyle  

```c
void GetItemStyle(MenuHandle mh, INTEGER item, INTEGER* stylep)
```

Trap: `0xA941` executor=C_

### GetMBarHeight  

```c
INTEGER GetMBarHeight()
```

Trap: — (executor 实现，无 trap) executor=C_

### GetMCEntry  

```c
MCEntryPtr GetMCEntry(INTEGER ?, INTEGER ?)
```

Trap: `0xAA64` executor=C_

### GetMCInfo  

```c
MCTableHandle GetMCInfo()
```

Trap: `0xAA61` executor=C_

### GetMenu  

```c
MenuHandle GetMenu(INTEGER rid)
```

Trap: `0xA9BF` executor=C_

### GetMenuBar  

```c
Handle GetMenuBar()
```

Trap: `0xA93B` executor=C_

### GetMenuHandle  

```c
MenuHandle GetMenuHandle(INTEGER mid)
```

Trap: `0xA949` executor=C_

### GetMenuItemText  

```c
void GetMenuItemText(MenuHandle mh, INTEGER item, StringPtr str)
```

Trap: `0xA946` executor=C_

### GetNewMBar  

```c
Handle GetNewMBar(INTEGER mbarid)
```

Trap: `0xA9C0` executor=C_

### HiliteMenu  

```c
void HiliteMenu(INTEGER mid)
```

Trap: `0xA938` executor=C_

### InitMenus  

```c
void InitMenus()
```

Trap: `0xA930` executor=C_

### InitProcMenu  

```c
void InitProcMenu(INTEGER mbid)
```

Trap: `0xA808` executor=C_

### InsertMenu  

```c
void InsertMenu(MenuHandle mh, INTEGER before)
```

Trap: `0xA935` executor=C_

### InsertMenuItem  

```c
void InsertMenuItem(MenuHandle mh, ConstStringPtr str, INTEGER after)
```

Trap: `0xA826` executor=C_

### InsertResMenu  

```c
void InsertResMenu(MenuHandle mh, ResType restype, INTEGER after)
```

Trap: `0xA951` executor=C_

### InvalMenuBar  

```c
void InvalMenuBar()
```

Trap: `0xA81D` executor=C_

### MenuChoice  

```c
LONGINT MenuChoice()
```

Trap: `0xAA66` executor=C_

### MenuKey  

```c
LONGINT MenuKey(CharParameter thec)
```

Trap: `0xA93E` executor=C_

### MenuSelect  

```c
LONGINT MenuSelect(Point p)
```

Trap: `0xA93D` executor=C_

### NewMenu  

```c
MenuHandle NewMenu(INTEGER mid, ConstStringPtr str)
```

Trap: `0xA931` executor=C_

### PopUpMenuSelect  

```c
LONGINT PopUpMenuSelect(MenuHandle mh, INTEGER top, INTEGER left, INTEGER item)
```

Trap: `0xA80B` executor=C_

### SetItemCmd  

```c
void SetItemCmd(MenuHandle mh, INTEGER item, CharParameter cmd)
```

Trap: `0xA84F` executor=C_

### SetItemIcon  

```c
void SetItemIcon(MenuHandle mh, INTEGER item, Byte icon)
```

Trap: `0xA940` executor=C_

### SetItemMark  

```c
void SetItemMark(MenuHandle mh, INTEGER item, CharParameter mark)
```

Trap: `0xA944` executor=C_

### SetItemStyle  

```c
void SetItemStyle(MenuHandle mh, INTEGER item, INTEGER style)
```

Trap: `0xA942` executor=C_

### SetMCEntries  

```c
void SetMCEntries(INTEGER ?, MCTablePtr ?)
```

Trap: `0xAA65` executor=C_

### SetMCInfo  

```c
void SetMCInfo(MCTableHandle ?)
```

Trap: `0xAA62` executor=C_

### SetMenuBar  

```c
void SetMenuBar(Handle ml)
```

Trap: `0xA93C` executor=C_

### SetMenuFlash  

```c
void SetMenuFlash(INTEGER i)
```

Trap: `0xA94A` executor=C_

### SetMenuItemText  

```c
void SetMenuItemText(MenuHandle mh, INTEGER item, ConstStringPtr str)
```

Trap: `0xA947` executor=C_

### DisableMenuItem  

```c
void DisableMenuItem(MenuHandle mh, INTEGER item)
```

Trap: — (executor 实现，无 trap) **[carbon]**

### EnableMenuItem  

```c
void EnableMenuItem(MenuHandle mh, INTEGER item)
```

Trap: — (executor 实现，无 trap) **[carbon]**

## Typedefs

- **MenuPtr** = MenuInfo*
- **MenuHandle** = MenuPtr*
- **MCEntryPtr** = MCEntry*
- **MCTable** = MCEntry[1]
- **MCTablePtr** = MCEntry*
- **MCTableHandle** = MCTablePtr*
- **MenuRef** = MenuHandle

## Enums

- **?**
- **?**
- **?**

### Enum Values

**anonymous**:

- `noMark` = 0

**anonymous**:

- `mDrawMsg` = 0
- `mChooseMsg` = 1
- `mSizeMsg` = 2
- `mPopUpRect` = 3

**anonymous**:

- `textMenuProc` = 0

## Structs

- **MenuInfo** { menuID: INTEGER, menuWidth: INTEGER, menuHeight: INTEGER, menuProc: Handle, enableFlags: LONGINT, menuData: Str255 }
- **MCEntry** { mctID: INTEGER, mctItem: INTEGER, mctRGB1: RGBColor, mctRGB2: RGBColor, mctRGB3: RGBColor, mctRGB4: RGBColor, mctReserved: INTEGER }

## Low Memory Globals

- **TopMenuItem** @ 0xA0A (INTEGER) — MenuMgr IMV-249 (true);
- **AtMenuBottom** @ 0xA0C (INTEGER) — MenuMgr IMV-249 (true);
- **MenuList** @ 0xA1C (Handle) — MenuMgr IMI-346 (true);
- **MBarEnable** @ 0xA20 (INTEGER) — MenuMgr IMI-356 (true);
- **MenuFlash** @ 0xA24 (INTEGER) — MenuMgr IMI-361 (true);
- **TheMenu** @ 0xA26 (INTEGER) — MenuMgr IMI-357 (true);
- **MBarHook** @ 0xA2C (ProcPtr) — MenuMgr IMI-356 (true);
- **MenuHook** @ 0xA30 (ProcPtr) — MenuMgr IMI-356 (true);
- **MenuDisable** @ 0xB54 (LONGINT) — MenuMgr IMV-249 (true);
- **MBDFHndl** @ 0xB58 (Handle) — MenuMgr Private.a (true);
- **MBSaveLoc** @ 0xB5C (Handle) — MenuMgr Private.a (true);
- **MBarHeight** @ 0xBAA (INTEGER) — MenuMgr IMV-253 (true);
- **MenuCInfo** @ 0xD50 (MCTableHandle) — QuickDraw IMV-242 (true);
