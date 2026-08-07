<h1 align="center">esp32-mac-nano</h1>

<p align="center">
  <a href="README.md">English</a> | <a href="README.zh.md">中文</a>
</p>

<p align="center">
  <em>把 Macintosh Plus 装进口袋——ESP32-S3 上的 68k 模拟器</em>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/license-GPL--2.0--or--later-blue" alt="License">
  <img src="https://img.shields.io/badge/platform-ESP32--S3-orange" alt="Platform">
  <img src="https://img.shields.io/badge/cpu-68000-important" alt="CPU">
</p>

---

ESP32-S3 上的 Macintosh Plus 模拟器：

- 基于 Musashi 68000 CPU 核心，4 MB RAM，640×480 显示
- 完整模拟 VIA、IWM、SCSI、RTC、键盘与声音
- Web UI：截图、壁纸导出、软盘上传
- 网页 OTA 升级

## 快速上手

### 1. 硬件

- ESP32-S3 开发板（PSRAM 必需）
- LCD 屏（ST7701 480×640）
- SD 卡（可选，用于磁盘镜像存储）
- 物理按键

### 2. 准备 ROM

`macintosh/rom.bin`（Mac Plus v3，`4D1F8172`，128 KB）会被**编译进固件**，缺失时 `idf.py build` 无法完成。ROM 版权归 Apple 所有，**不随本仓库分发**，请自行获取：

- 从你合法持有的 Macintosh Plus 中提取（Mini vMac 的 [CopyRoms](https://www.gryphel.com/c/minivmac/extras/copyroms/index.html) 工具）
- 或从 [archive.org（mac_rom_archive 集合）](https://archive.org/download/mac_rom_archive_-_as_of_8-19-2011/mac_rom_archive_-_as_of_8-19-2011.zip/4D1F8172%20-%20MacPlus%20v3.ROM) 获取（版权归 Apple，请自行确认你所在司法管辖区是否允许）

下载后将文件命名为 `macintosh/rom.bin` 即可。

### 3. 构建并烧录

需要 ESP-IDF v5.5.4：

```bash
idf.py build
idf.py flash
```

## 系统镜像（可选）

运行模拟器需自备 Mac OS 系统盘，见 [docs/images.md](docs/images.md)。

## License

[GPL-2.0-or-later](LICENSE) — 仓库包含 GPL-2.0 的 Musashi CPU 核心及 PCE/umac 派生代码，固件整体按 GPL-2.0 发布。

## 鸣谢

本项目参考/借鉴了以下项目：

- [Musashi](https://github.com/kstenerud/Musashi) — 68000 CPU 模拟核心
- [umac](https://github.com/evansm7/umac) — Macintosh 128K 模拟器，本项目模拟核心的起源
- [Retro68](https://github.com/autc04/Retro68) — 68k Mac OS 交叉编译工具链
- [Mini vMac](https://www.gryphel.com/c/minivmac/) — 桌面 Macintosh 模拟器（CopyRoms ROM 提取工具）
- [pico-mac](https://github.com/evansm7/pico-mac) — RP2040 上的 umac 移植，ROM 处理流程参考
- [minimacplus](https://github.com/spritetm/minimacplus) — ESP32 Mac Plus 模拟器，版权声明与镜像处理参考
- [cydintosh](https://github.com/likeablob/cydintosh) — ESP32 开发板上的 Mac Plus 模拟器
- [Infinite Mac](https://github.com/mihaip/infinite-mac) — 浏览器中的经典 Macintosh，系统盘导出参考
- [mfsjs](https://github.com/minorbug/mfsjs) — 纯浏览器端图片转 MacPaint (PNTG) 并打包 MFS 磁盘镜像的工具（[在线试用](https://minorbug.github.io/mfsjs/index.html)）
