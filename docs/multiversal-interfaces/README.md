# Multiversal Interfaces — API Reference

由 [Retro68](https://github.com/autc04/Retro68) 子模块 `multiversal`
（[autc04/multiversal](https://github.com/autc04/multiversal)）的
`defs/*.yaml` 自动生成。本目录共 **63** 个 API 域，
**1476** 个函数、**604** 个类型、**280** 个枚举、**248** 个低内存全局变量。

## 这是什么

Multiversal Interfaces 是经典 Mac OS（Carbon 之前）Toolbox API 的开源重实现，
用 YAML 定义、由 Ruby 生成器转为 C/C++ 头文件（Retro68 编译器使用）。
来源是 Executor 2000 的干净室实现头文件（宽松许可证，可自由再分发）。

**覆盖范围**：System 7.0 及以前。**不包含**：Carbon、MacTCP、OpenTransport、
Navigation Services（仅少量定义）、System 7 之后引入的 API。

> 也可使用 Apple Universal Interfaces（需自备，来自 MPW/CodeWarrior），
> 覆盖更全（含 Carbon）。两者二选一，构建时自动检测。

## 使用

Retro68 编译时 `-I` 已指向生成的头文件。直接 `#include <Quickdraw.h>` 等即可。
函数带 `trap` 的（如 `CopyBits` = `0xA8EC`）由 Retro68 生成 trap 调用胶水；
`executor: C_` 表示函数体在模拟器/运行时实现。

## API 域索引

| 域 | 函数 | 类型 | 枚举 | 低内存 |
|---|---|---|---|---|
| [ADB](ADB.md) | 6 | 2 | 0 | 2 |
| [AliasMgr](AliasMgr.md) | 9 | 3 | 1 | 0 |
| [AppleEvents](AppleEvents.md) | 58 | 37 | 18 | 1 |
| [AppleTalk](AppleTalk.md) | 0 | 0 | 0 | 3 |
| [BinaryDecimal](BinaryDecimal.md) | 2 | 0 | 0 | 0 |
| [CQuickDraw](CQuickDraw.md) | 138 | 38 | 10 | 4 |
| [CodeFragments](CodeFragments.md) | 7 | 13 | 5 | 0 |
| [CommTool](CommTool.md) | 6 | 5 | 6 | 0 |
| [Components](Components.md) | 0 | 4 | 0 | 0 |
| [ControlMgr](ControlMgr.md) | 31 | 9 | 9 | 1 |
| [DeskMgr](DeskMgr.md) | 7 | 0 | 2 | 1 |
| [DeviceMgr](DeviceMgr.md) | 9 | 10 | 7 | 12 |
| [DialogMgr](DialogMgr.md) | 43 | 15 | 7 | 6 |
| [Disk](Disk.md) | 3 | 1 | 1 | 0 |
| [DiskInit](DiskInit.md) | 6 | 0 | 0 | 0 |
| [Displays](Displays.md) | 1 | 0 | 0 | 0 |
| [EditionMgr](EditionMgr.md) | 30 | 35 | 3 | 0 |
| [EventMgr](EventMgr.md) | 0 | 1 | 6 | 1 |
| [FileMgr](FileMgr.md) | 122 | 34 | 7 | 11 |
| [Finder](Finder.md) | 16 | 3 | 0 | 0 |
| [FontMgr](FontMgr.md) | 16 | 11 | 6 | 16 |
| [Gestalt](Gestalt.md) | 3 | 0 | 17 | 0 |
| [HelpMgr](HelpMgr.md) | 21 | 3 | 1 | 0 |
| [Iconutil](Iconutil.md) | 35 | 8 | 7 | 0 |
| [IntlUtil](IntlUtil.md) | 21 | 9 | 6 | 0 |
| [ListMgr](ListMgr.md) | 26 | 7 | 4 | 0 |
| [LowMem](LowMem.md) | 0 | 0 | 0 | 21 |
| [MPW](MPW.md) | 0 | 7 | 0 | 1 |
| [MacTypes](MacTypes.md) | 0 | 47 | 2 | 0 |
| [MemoryMgr](MemoryMgr.md) | 55 | 3 | 3 | 25 |
| [MenuMgr](MenuMgr.md) | 51 | 9 | 3 | 13 |
| [MixedMode](MixedMode.md) | 7 | 10 | 11 | 0 |
| [Navigation](Navigation.md) | 20 | 24 | 4 | 0 |
| [NotifyMgr](NotifyMgr.md) | 2 | 2 | 0 | 0 |
| [OSEvent](OSEvent.md) | 14 | 7 | 4 | 8 |
| [OSUtil](OSUtil.md) | 50 | 7 | 10 | 20 |
| [PEFBinaryFormat](PEFBinaryFormat.md) | 9 | 9 | 10 | 0 |
| [PPC](PPC.md) | 0 | 8 | 0 | 0 |
| [Package](Package.md) | 2 | 0 | 1 | 1 |
| [PrintMgr](PrintMgr.md) | 23 | 13 | 8 | 1 |
| [ProcessMgr](ProcessMgr.md) | 9 | 7 | 7 | 0 |
| [QuickDraw](QuickDraw.md) | 166 | 50 | 13 | 29 |
| [QuickTime](QuickTime.md) | 21 | 3 | 0 | 0 |
| [ResourceMgr](ResourceMgr.md) | 48 | 0 | 7 | 11 |
| [SANE](SANE.md) | 48 | 9 | 7 | 0 |
| [ScrapMgr](ScrapMgr.md) | 6 | 2 | 1 | 5 |
| [ScriptMgr](ScriptMgr.md) | 48 | 20 | 14 | 1 |
| [SegmentLdr](SegmentLdr.md) | 6 | 1 | 2 | 7 |
| [Serial](Serial.md) | 9 | 3 | 11 | 0 |
| [ShutDown](ShutDown.md) | 4 | 0 | 1 | 0 |
| [SoundDvr](SoundDvr.md) | 5 | 13 | 1 | 5 |
| [SoundMgr](SoundMgr.md) | 56 | 24 | 7 | 1 |
| [SpeechManager](SpeechManager.md) | 25 | 11 | 0 | 0 |
| [StartMgr](StartMgr.md) | 0 | 6 | 0 | 4 |
| [StdFilePkg](StdFilePkg.md) | 8 | 3 | 8 | 2 |
| [SysErr](SysErr.md) | 1 | 0 | 1 | 4 |
| [TextEdit](TextEdit.md) | 48 | 27 | 7 | 4 |
| [TimeMgr](TimeMgr.md) | 4 | 1 | 0 | 0 |
| [ToolboxEvent](ToolboxEvent.md) | 12 | 0 | 0 | 7 |
| [ToolboxUtil](ToolboxUtil.md) | 41 | 3 | 2 | 0 |
| [VDriver](VDriver.md) | 0 | 12 | 0 | 0 |
| [VRetraceMgr](VRetraceMgr.md) | 5 | 2 | 1 | 2 |
| [WindowMgr](WindowMgr.md) | 57 | 13 | 11 | 18 |

## 相关

- 生成脚本：`tools/gen-multiversal-docs.py`
- 上游仓库：`ref/Retro68/multiversal`（submodule，commit `ac0a295`）
