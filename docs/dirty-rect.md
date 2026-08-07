# Dirty Rectangle Partial Blit — Design Doc

## Motivation

On ESP32-S3, m68k emulation dominates frame time (~86% of 22ms/frame).
When the mouse moves, the Mac OS redraws the cursor (usually 16×16 to 32×32
pixels), which dirties the screen buffer. The original code does a full
640×480 blit every time the screen is dirty.

With dirty-rectangle tracking, only the changed region is blitted — typically
~1K pixels instead of 307K pixels, reducing PSRAM write bandwidth contention
and freeing up time for the emu core.

## Architecture

Three layers:

### 1. Producer: m68k_write_memory_* (core 1, emu task)

When the 68k writes to the screen buffer address range, the write address is
converted to Mac pixel coordinates (x, y) and the dirty rectangle is expanded:

```
address → offset = address - MACPLUS_SCREENBUF
y = offset / 80           (80 bytes = 640 pixels / 8 bits per byte)
x = (offset % 80) * 8     (8 pixels per byte)
dirty_rect.expand(x, y, x+8, y+1)
```

### 2. State: dirty_x1/y1/x2/y2 in macplus_t (cross-core)

Four `int16_t` fields in `macplus_t`. `dirty_x1 > dirty_x2` means "not dirty".
Producer (core 1) writes, consumer (core 0) reads & clears under VSYNC.
No lock — worst case a one-frame glitch, self-correcting.

### 3. Consumer: mach_s3_blit_mac_cb (core 0, blit worker)

On each VSYNC, reads dirty rect, converts Mac→LCD coordinates (90° CW rotation),
and calls the rect blit function:

```
Mac (mx,my) → LCD (lx,ly):
  lx = my
  ly = lcd_height - mx

Mac rect {mx1,my1,mx2,my2} → LCD rect:
  lcd_x1 = my1,  lcd_y1 = lcd_height - mx2
  lcd_x2 = my2,  lcd_y2 = lcd_height - mx1
```

## Files Changed

| File | Change |
|------|--------|
| `macplus.h` | `vbuf_dirty` → 4× `int16_t` dirty rect fields. New macros: `VBUF_MARK_DIRTY_RECT`, `VBUF_MARK_DIRTY_ALL`, `VBUF_INIT_DIRTY`. Controlled by `#define ENABLE_DIRTY_RECT`. |
| `macplus.c` | `m68k_write_memory_8/16/32` compute Mac (x,y) from 68k address → call `VBUF_MARK_DIRTY_RECT`. `mac_erase_scrn_try_hook` → `VBUF_MARK_DIRTY_ALL`. |
| `frame_blit.h` | New `blit_mac_mono_to_lcd_rgba_rect()` — blits only an LCD sub-rectangle. 4-pixel aligned for efficiency. |
| `main.c` | `mach_s3_blit_mac_cb`: reads dirty rect → converts coords → calls rect blit or full blit if entire screen dirty. |

## A/B Testing

Comment out `#define ENABLE_DIRTY_RECT` in `macplus.h` to fall back to
original full-frame blit. All old code is preserved under `#else`.

## Issues / Limitations

- Code complexity increased significantly in hot paths (m68k_write_memory_*).
- Dirty rect only accurate to 8-pixel horizontal granularity (1bpp byte boundary).
- No lock between producer/consumer — theoretically a race window, practically harmless.
- `is_alt` logic forces full blit on vbuf switch, which is conservative but correct.
- Measurement code in clock.c should be removed before production use.

## Patch

The full patch is saved at:
  /tmp/dirty_rect.patch

Apply with:
  git apply /tmp/dirty_rect.patch
