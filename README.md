# esp32-mac-nano

[English](README.md) | [中文](README.zh.md)

Macintosh Plus emulator for ESP32-S3:

- Musashi 68000 CPU core, 4 MB RAM, 640×480 display
- Full emulation of VIA, IWM, SCSI, RTC, keyboard and sound
- Web UI: screenshots, HD wallpaper export, floppy upload
- Web-based OTA updates

## Quick Start

### 1. Hardware

- ESP32-S3 board (PSRAM required)
- LCD display (ST7701 480×640)
- SD card (optional, for disk image storage)
- Physical buttons

### 2. Prepare the ROM

`macintosh/rom.bin` (Mac Plus v3, `4D1F8172`, 128 KB) is **compiled into the firmware**; without it `idf.py build` will fail. The ROM is copyrighted by Apple and **is not distributed with this repository**. Obtain it yourself:

- Dump it from a Macintosh Plus you legally own (e.g. Mini vMac's [CopyRoms](https://www.gryphel.com/c/minivmac/extras/copyroms/index.html) tool)
- Or download it from [archive.org (mac_rom_archive collection)](https://archive.org/download/mac_rom_archive_-_as_of_8-19-2011/mac_rom_archive_-_as_of_8-19-2011.zip/4D1F8172%20-%20MacPlus%20v3.ROM) (copyrighted by Apple — please verify it is allowed in your jurisdiction)

Name the file `macintosh/rom.bin`.

### 3. Build and Flash

Requires ESP-IDF v5.5.4:

```bash
idf.py build
idf.py flash
```

## Disk Images (optional)

You need to provide a Mac OS system disk to run the emulator — see [docs/images.md](docs/images.md).

## License

[GPL-2.0-or-later](LICENSE) — the repository includes the GPL-2.0 Musashi CPU core and PCE/umac-derived code; the firmware is distributed under GPL-2.0.

## Acknowledgements

This project references/borrows from:

- [Musashi](https://github.com/kstenerud/Musashi) — 68000 CPU emulation core
- [umac](https://github.com/evansm7/umac) — Macintosh 128K emulator, origin of this project's emulation core
- [Retro68](https://github.com/autc04/Retro68) — 68k Mac OS cross-compilation toolchain
- [Mini vMac](https://www.gryphel.com/c/minivmac/) — desktop Macintosh emulator (CopyRoms ROM extraction tool)
- [pico-mac](https://github.com/evansm7/pico-mac) — umac port for RP2040, ROM handling reference
- [minimacplus](https://github.com/spritetm/minimacplus) — ESP32 Mac Plus emulator, copyright & disk image handling reference
- [cydintosh](https://github.com/likeablob/cydintosh) — Mac Plus emulator for ESP32 development boards
- [Infinite Mac](https://github.com/mihaip/infinite-mac) — classic Macintosh in the browser, system disk export reference
- [mfsjs](https://github.com/minorbug/mfsjs) — browser-based image-to-MacPaint (PNTG) converter with MFS disk packaging ([try it online](https://minorbug.github.io/mfsjs/index.html))
