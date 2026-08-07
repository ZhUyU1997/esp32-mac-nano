# BLE Pause Menu

9-file patch adding Bluetooth HID host management UI to the Pause Menu.

## Overview

Adds a Bluetooth device panel to the Pause Menu (LVGL, 640×480 1-bit display),
enabling users to pair, connect, and manage BLE keyboards/mice without
recompilation. The BLE HID host was previously hardcoded (commented out) with
no runtime control.

## Architecture

```
Pause Menu (settings_ui.c)
├── Left 1/2: Floppy Manager + Quick Settings
└── Right 1/2: Bluetooth Panel (ble_panel.c)
    ├── Switch (init/deinit BLE stack)
    ├── Device list (paired + discovered)
    ├── Scan / Forget / Clear buttons
    └── 500ms refresh timer

BLE Driver Layer
├── input-ble-hid.c/h   — lifecycle (init, deinit, scan, GAP callback)
└── ble_dev_cache.c/h   — device cache + query + forget + clear
```

## Memory Impact

| Metric | Before BLE | BLE ON | Notes |
|--------|-----------|--------|-------|
| DRAM free | ~76KB | ~59KB | -17KB (widgets + Bluedroid BSS) |
| DMA free | ~68KB | ~51KB | -17KB (Controller buffers) |
| ROM (binary) | 1.64MB | 1.71MB | +70KB |
| fps (emulator) | 59 | 59 | no impact |

**Mitigations applied:**

- `CONFIG_BT_GATTS_ENABLE=n` — saves 45KB ROM (101 unused symbols)
- `CONFIG_BT_CTRL_BLE_MAX_ACT=2` — saves ~8KB DMA (was 6)
- `CONFIG_BT_ACL_CONNECTIONS=2` — saves ~4KB DMA (was 4)
- `LV_COLOR_DEPTH_1` — 1-bit display, minimal LVGL buffer
- PSRAM heap preferred for BT allocations (`CONFIG_BT_ALLOCATION_FROM_SPIRAM_FIRST`)
- 128KB Mac RAM pre-allocated before BT init

**Known limitation:** 17KB DRAM drop on pause menu entry is LVGL widget allocation;
each widget instance is <1KB so the total is normal for ~30 widgets. Switching off
BLE in the panel recovers Controller/DMA memory without leaving pause menu.

## Real-time Concerns

BLE HID at 11–30ms connection intervals is tolerant of flash cache misses.
The BLE Controller runs on dedicated hardware with internal SRAM buffers;
Host-side GATT processing and HID report parsing happen in idle time between
emulator frames. No frame drops observed at 59fps with USB HID + BLE HID concurrently.

## Files

| File | Role |
|------|------|
| `ble_panel.c` | UI panel: switch, device list, Scan/Forget/Clear buttons, timer |
| `ble_panel.h` | `ble_panel_create(screen, x, y, w, h)` |
| `settings_ui.c` | Layout: left/right 1/2 split, buttons inside panels, exported `create_action_btn` |
| `settings_ui.h` | `UI_BTN_RADIUS`, `UI_PANEL_RADIUS` macros, `create_action_btn` |
| `input-ble-hid.c` | BLE lifecycle, GAP callback, scan control, state queries |
| `input-ble-hid.h` | `ble_hid_host_init/deinit`, scan/state APIs |
| `ble_dev_cache.c` | Name/discovered cache, paired queries, `ble_forget_device`, `ble_clear_all_bonds` |
| `ble_dev_cache.h` | Cache + query + management API |
| `CMakeLists.txt` | Added `ble_panel.c`, `ble_dev_cache.c` |

## BLE Bond Flow

1. User toggles switch ON → `ble_hid_host_init()` → Bluedroid + Controller init
2. Scan (active, HID UUID 0x1812 filtered) → discovered list in panel
3. Click discovered device → `ble_connect_to_device()` → pair + bond
4. Bond saved to NVS (`ESP_LE_AUTH_REQ_SC_MITM_BOND`, Just Works)
5. Reconnect: Bluedroid reads bond from NVS, encrypts silently
6. Forget: `esp_ble_remove_bond_device()` + disconnect
7. Clear All: iterates bond list, removes each

HID-only filtering: advertising data is checked for 16-bit Service UUID 0x1812
(both complete `0x03` and partial `0x02` AD types) before adding to discovered list.

## Applying the Patch

```bash
cd /path/to/project
git apply docs/ble-pause-menu.patch
idf.py fullclean build
```
