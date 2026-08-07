# SD Card Upgrade

This project supports upgrading firmware (and optionally hard disk image) via SD card.  
Place `upgrade.bin` on the SD card root, insert it, and reboot — the device detects it automatically and shows a progress bar.

---

## Quick Start

```bash
# 1. Build firmware
idf.py build

# 2. Generate upgrade file (firmware only)
make upgrade
# or with hd image:
make upgrade-full

# 3. Copy to SD card
cp upgrade.bin /media/sdcard/

# 4. Insert SD card into device and reboot
```

The device will boot, detect `upgrade.bin`, display a progress screen, and reboot automatically after completion.

---

## File: `upgrade.bin` Format

| Offset | Size | Field |
|--------|------|-------|
| 0      | 4    | Magic `ESUP` |
| 4      | 2    | Version (1) |
| 6      | 2    | Entry count |
| 8      | 2    | Reserved (0) |
| 10     | N×16 | Entry table |
| 10+N×16| —    | Payload data |

Each entry (16 bytes):

| Offset | Size | Field |
|--------|------|-------|
| 0      | 4    | `data_offset` from file start |
| 4      | 4    | `data_size` in bytes |
| 8      | 8    | Target partition name (null-padded) |

Supported partition names: `factory` (firmware → written to inactive OTA slot), `hd`.

### Build Tool

```bash
# firmware only
python3 tools/make_upgrade.py \
    --firmware build/esp32-mac-nano.bin \
    --output upgrade.bin

# firmware + hd image
python3 tools/make_upgrade.py \
    --firmware build/esp32-mac-nano.bin \
    --hd macintosh/hd.img \
    --output upgrade.bin
```

---

## Partition Layout (16 MB Flash)

| Partition | Type | Size | Description |
|-----------|------|------|-------------|
| nvs       | data  | 16K  | Settings (floppy path, backlight, etc.) |
| otadata   | data  | 8K   | OTA boot selection & rollback state |
| phy_init  | data  | 4K   | PHY calibration |
| factory   | app   | 2M   | Golden firmware image (fallback) |
| ota_0     | app   | 2M   | OTA slot A |
| ota_1     | app   | 2M   | OTA slot B |
| hd        | 0x40  | 8M   | Mac hard disk image |

Remaining: ~2 MB free for future use.

---

## Upgrade Flow

```
Boot
  ├─ upgrade_mark_app_valid()       ← confirm current firmware healthy
  ├─ mount SD card
  ├─ upgrade.bin found? ──┤
  │                       ├─ yes: show LVGL progress bar
  │                       │       ├─ read ESUP header & entries
  │                       │       ├─ firmware entry → esp_ota_write(inactive slot)
  │                       │       │                     └─ esp_ota_set_boot_partition()
  │                       │       ├─ hd entry → esp_partition_write(hd)
  │                       │       ├─ delete upgrade.bin
  │                       │       └─ esp_restart()
  │                       │
  │                       └─ no:  normal Mac emulator boot
  └─ launch Mac emulator
```

### OTA Slot Alternation

With dual OTA (ota_0 + ota_1), upgrades alternate automatically:

```
1st upgrade:  running factory    → write ota_0,  boot ota_0
2nd upgrade:  running ota_0      → write ota_1,  boot ota_1
3rd upgrade:  running ota_1      → write ota_0,  boot ota_0
...
```

### Rollback (CONFIG_BOOTLOADER_APP_TEST)

When enabled, new firmware starts in `PENDING_VERIFY` state.  
If the device crashes or watchdog resets before `upgrade_mark_app_valid()` is called,
the bootloader automatically reverts to the previous partition.

On failure, the `upgrade.bin` file is **left on the SD card** so you can retry.

---

## First-Time Flashing

After partition table changes (adding otadata, resizing slots), you must erase the
entire flash before the first boot:

```bash
idf.py erase-flash flash
# or
make flash   # with appropriate port configured
```

---

## Makefile Targets

```bash
make upgrade        # generate upgrade.bin (firmware only)
make upgrade-full   # generate upgrade.bin (firmware + hd.img)
make flash          # flash via idf.py
```
