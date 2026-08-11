# Project Layout

Multi-platform Macintosh Plus emulator. Layers: `arch/` for platform bring-up, `core/` for emulation, `driver/` + `include/` for reusable code.

Current platforms:
- `arch/esp32/mach-s3` — ESP32-S3 (ESP-IDF / FreeRTOS)
- `arch/x64/mach-sdl` — host (SDL)

## Commands

```bash
source ~/.espressif/tools/activate_idf_v5.5.4.sh 1>/dev/null 2>&1; \
  cd /path/to/project && idf.py build 2>&1 | tail -100
# Note: use `;` not `&&` after source — activate exits 1, so && skips the build.
make flash         # idf.py flash
make upgrade       # build + SD card upgrade file (firmware only)
make upgrade-full  # build + upgrade file (firmware + hd.img)
make install       # build Mac apps + copy hd_v1.img → hd.img
make mac-all       # build all Mac 68k apps (Retro68)
make mac-clean     # clean Mac app build
```

## UI Strings & Fonts

UI text is centralized in `main/arch/esp32/mach-s3/ui/ui_strings.json` (source of truth, 56 entries, Chinese). Generated artifacts are committed, the generator scripts are the only way to edit them:

```bash
node scripts/gen-ui-strings.js                 # ui_strings.json → include/ui_strings.h
node scripts/gen-mono-opposans-font.js [14|18] # extract non-ASCII chars from JSON → lv_font_conv → ui/font/mono_opposans_<size>.c
```

- 18px is the main menu font (bpp=1); 14px is the auxiliary font for the Recover/Update button + version label (bpp=2 AA, small sizes look grainy at bpp=1).
- Source TTF lives at `fonts/OPPOSans-R.ttf` — git-ignored (`fonts/` in `.gitignore`, contains personal metadata, never commit it). Override the path with `OPPOSANS_TTF`.
- Font glyphs are auto-extracted from `ui_strings.json` (non-ASCII chars only) — adding UI text then re-running the scripts regenerates the fonts with no missing glyphs.

## Architecture

| Layer | Directory | Rules |
|-------|-----------|-------|
| core | `main/core/macplus/` | Emulation core — Musashi 68000, MMU, VIA, IWM, SCSI, RTC, keyboard, sound. **No ESP-IDF/FreeRTOS headers.** |
| core | `main/core/kernel/` | Class/object system, driver/device registration, framebuffer, input |
| core | `main/core/dtree/` | JSON device-tree parsing + driver probing |
| arch | `main/arch/<arch>/mach-*/` | Board bring-up: `main.c`, `driver/`, `dtree/mach-*.json` |
| driver | `main/driver/` | Cross-platform driver adapters: `block`, `gpio`, `input`, `sound`, `video`. **No platform headers.** |
| include | `main/include/` | Public headers — `device.h`, `driver.h`, `sound.h`, `framebuffer.h`, `dt.h`, `fast_attr.h` |
| util | `main/util/` | Generic utilities, no board dependencies |

## Key Conventions

- **C only** — OOP via `class()`/`class_impl()`/`new()`/`delete()` macros.
- **Naming**: `snake_case`, `k_` prefix for locals, `_t` suffix for typedefs. Driver files: `fb-*.c`, `snd-*.c`, `block-*.c`.
- **`FAST_FUNC_ATTR` / `FAST_DATA_ATTR`**: IRAM/DRAM placement on ESP32.
- **ROM patches**: `target_compile_definitions` in `CMakeLists.txt`.
- **Sound**: `mac_sound_vbl()` (60 Hz) → 370×16-bit mono → ring buffer → I2S DAC.
- **Tasks**: emulation on core 1, sound on core 0. LVGL settings UI, `mac_set_pause()` / `mac_get_pause()`.
- **Commits**: [Conventional Commits](https://www.conventionalcommits.org/) — `type: description`, lowercase, imperative. Types: `feat`, `fix`, `chore`, `docs`, `refactor`, `test`, `style`.

## Driver / Device Model

Device-tree JSON → `probe()` → `driver_t` → typed `device_t` (e.g. `framebuffer_t`).

### Device-Tree Node

Files: `main/arch/<arch>/mach-*/dtree/mach-*.json`.

```json
{ "block-file:0": { "name": "hd-img", "path": "/sdcard/hd.img" },
  "fb-st7701":    { "name": "lcd", "h_res": 480, "v_res": 640 },
  "key-gpio-polled": { "name": "keys", "poll-interval-ms": 20 } }
```

Key format: `<driver>[:<id>][@<addr>]`. Properties: `name` (instance name), `status` (`"okay"` or `"disabled"`).

### Driver Skeleton

```c
static device_t *mydev_probe(driver_t *drv, dtnode_t *n) {
    const int id = dt_read_id(n);
    return register_device(dt_read_string(n, "name", "mydev"), drv);
}
impl(mydev, driver_t) { .name = "mydev", .probe = mydev_probe };
```

### Typed Device

```c
// main/include/<subsystem>.h
class(mytype_t, device_t) { int field; };
device_t *register_<subsystem>(mytype_t *obj, driver_t *drv, const dtnode_t *n);
mytype_t *<subsystem>_lookup(const char *name);
```

See `framebuffer.h` + `fb-st7701.c` for a concrete example.
