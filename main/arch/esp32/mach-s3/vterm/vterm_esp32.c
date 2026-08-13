/*
 * ESP32 vterm adapter: VT100 terminal over telnet.
 *
 * libvterm screen -> on-the-fly renderer: each cell is rasterized into a
 * transposed stack buffer and flushed to the rotated 480x640 ST7701
 * framebuffer as 16-byte runs, with dirty-rect damage tracking. 80x30
 * cells of 8x16 pixels, 64-colour RGB222 output.
 */
#include <string.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "vterm.h"
#include "term_render.h"
#include "framebuffer.h"
#include "blit_worker.h"
#include "input.h"
#include "vterm_hid_map.h"
#include "vterm_telnet.h"

#define VTERM_ROWS 30
#define VTERM_COLS 80
#define VTERM_SB_CAP 200

static const char *TAG = "vterm";

static uint8_t *s_pixels; /* RGB222 (64-colour) intermediate, 80*30*8*16 */
static VTerm *s_vt;
static term_renderer_t s_renderer;
static mach_s3_blit_worker_t *s_blit_worker;
static framebuffer_t *s_lcd;
static uint8_t *s_fb;   /* direct rotated fb output */
static int s_fb_w, s_fb_h;

/* ---- scrollback ring --------------------------------------------------- */
static VTermScreenCell *s_sb;   /* VTERM_SB_CAP rows x VTERM_COLS cells */
static int s_sb_count;          /* valid rows stored */
static int s_sb_head;           /* ring index of the newest row */
static bool s_dirty;            /* damage pending */
static bool s_cursor_dirty;     /* mouse moved: re-blit only, no re-render */
static bool s_sync_update;      /* DECSET 2026 synchronized output in progress */
static int s_mouse_x, s_mouse_y; /* physical landscape px (640x480) */
static int s_mouse_mode;         /* VTERM_PROP_MOUSE value (0 = off) */
static bool s_sel_dragging;      /* left button held for text selection */

/* mouse pointer save/restore (the fb is rendered on the fly, so a moving
 * pointer must be restored from a saved 16x16 patch instead of re-blitting
 * the whole frame from s_pixels) */
static uint8_t s_ptr_save[16][16];
static bool s_ptr_saved;
static int s_ptr_x, s_ptr_y;

/* damage bbox for dirty-rect rendering (cell coords) */
static bool s_dmg_full; /* full-frame render requested */
static bool s_dmg_has;  /* partial bbox accumulated */
static int s_dmg_r0, s_dmg_r1, s_dmg_c0, s_dmg_c1;

/* libvterm mouse/keyboard output -> host */
static void vterm_output_cb(const char *s, size_t len, void *user)
{
	(void)user;
	(void)vterm_telnet_send((const uint8_t *)s, len);
}

/* ---- screen callbacks -------------------------------------------------- */

static void dmg_mark_full(void)
{
	s_dmg_full = true;
}

static void dmg_add(int r0, int r1, int c0, int c1)
{
	if (r0 < 0)
		r0 = 0;
	if (r1 >= VTERM_ROWS)
		r1 = VTERM_ROWS - 1;
	if (c0 < 0)
		c0 = 0;
	if (c1 >= VTERM_COLS)
		c1 = VTERM_COLS - 1;
	if (r0 > r1 || c0 > c1)
		return;
	if (s_dmg_full)
		return;
	if (!s_dmg_has) {
		s_dmg_has = true;
		s_dmg_r0 = r0;
		s_dmg_r1 = r1;
		s_dmg_c0 = c0;
		s_dmg_c1 = c1;
	} else {
		if (r0 < s_dmg_r0)
			s_dmg_r0 = r0;
		if (r1 > s_dmg_r1)
			s_dmg_r1 = r1;
		if (c0 < s_dmg_c0)
			s_dmg_c0 = c0;
		if (c1 > s_dmg_c1)
			s_dmg_c1 = c1;
	}
}

static int vterm_cb_damage(VTermRect rect, void *user)
{
	(void)user;
	s_dirty = true;
	dmg_add(rect.start_row, rect.end_row - 1, rect.start_col, rect.end_col - 1);
	return 1;
}

static int vterm_cb_moverect(VTermRect dest, VTermRect src, void *user)
{
	(void)dest;
	(void)src;
	(void)user;
	/* let libvterm damage the scrolled region (dirty-rect re-renders it) */
	return 0;
}

static int vterm_cb_movecursor(VTermPos pos, VTermPos oldpos, int visible, void *user)
{
	(void)user;
	s_renderer.cursor = pos;
	s_renderer.cursor_visible = visible != 0;
	s_dirty = true;
	/* re-render old and new cursor cells (block cursor is reverse video) */
	dmg_add(oldpos.row, oldpos.row, oldpos.col - 1, oldpos.col + 1);
	dmg_add(pos.row, pos.row, pos.col - 1, pos.col + 1);
	return 1;
}

static int vterm_cb_settermprop(VTermProp prop, VTermValue *val, void *user)
{
	(void)user;
	switch (prop) {
	case VTERM_PROP_CURSORVISIBLE:
		s_renderer.cursor_visible = val->boolean;
		s_dirty = true;
		dmg_add(s_renderer.cursor.row, s_renderer.cursor.row,
		        s_renderer.cursor.col - 1, s_renderer.cursor.col + 1);
		break;
	case VTERM_PROP_CURSORBLINK:
		s_renderer.cursor_blink = val->boolean;
		s_dirty = true;
		dmg_add(s_renderer.cursor.row, s_renderer.cursor.row,
		        s_renderer.cursor.col - 1, s_renderer.cursor.col + 1);
		break;
	case VTERM_PROP_CURSORSHAPE:
		s_renderer.cursor_shape = val->number;
		s_renderer.cursor_shape_set = true;
		s_dirty = true;
		dmg_add(s_renderer.cursor.row, s_renderer.cursor.row,
		        s_renderer.cursor.col - 1, s_renderer.cursor.col + 1);
		break;
	case VTERM_PROP_SYNCOUTPUT:
		/* DECSET 2026: defer rendering until the update ends so
		 * fullscreen apps (vim/htop) repaint as one frame */
		s_sync_update = val->boolean;
		break;
	case VTERM_PROP_MOUSE:
		s_mouse_mode = val->number;
		break;
	default:
		break;
	}
	return 1;
}

static int vterm_cb_bell(void *user)
{
	(void)user;
	return 1;
}

/* ---- scrollback: libvterm hands scrolled-out lines to sb_pushline ---- */

static int vterm_cb_sb_pushline(int cols, const VTermScreenCell *cells, void *user)
{
	(void)user;
	if (!s_sb)
		return 1; /* scrollback disabled (alloc failed) */
	if (cols > VTERM_COLS)
		cols = VTERM_COLS;
	if (s_sb_count < VTERM_SB_CAP)
		s_sb_count++;
	s_sb_head = (s_sb_head + 1) % VTERM_SB_CAP;
	memcpy(&s_sb[(size_t)s_sb_head * VTERM_COLS], cells,
	       (size_t)cols * sizeof(VTermScreenCell));
	for (int c = cols; c < VTERM_COLS; c++)
		memset(&s_sb[(size_t)s_sb_head * VTERM_COLS + c], 0, sizeof(VTermScreenCell));
	return 1;
}

static int vterm_cb_sb_popline(int cols, VTermScreenCell *cells, void *user)
{
	(void)user;
	if (!s_sb)
		return 0;
	if (s_sb_count == 0)
		return 0;
	int old_idx = (s_sb_head - (s_sb_count - 1) + VTERM_SB_CAP) % VTERM_SB_CAP;
	int n = cols < VTERM_COLS ? cols : VTERM_COLS;
	memcpy(cells, &s_sb[(size_t)old_idx * VTERM_COLS], (size_t)n * sizeof(VTermScreenCell));
	for (int c = n; c < cols; c++)
		memset(&cells[c], 0, sizeof(VTermScreenCell));
	s_sb_count--;
	return 1;
}

static int vterm_cb_sb_clear(void *user)
{
	(void)user;
	if (!s_sb)
		return 1;
	s_sb_count = 0;
	s_sb_head = 0;
	return 1;
}

/* row 0 = most recently scrolled-out line (matches term_render's contract) */
static int vterm_sb_get_cell(void *user, int row, int col, VTermScreenCell *cell)
{
	(void)user;
	if (!s_sb || row < 0 || row >= s_sb_count)
		return 0;
	int idx = (s_sb_head - row + VTERM_SB_CAP) % VTERM_SB_CAP;
	if (col < 0 || col >= VTERM_COLS) {
		memset(cell, 0, sizeof(*cell));
		return 1;
	}
	*cell = s_sb[(size_t)idx * VTERM_COLS + col];
	return 1;
}

static const VTermScreenCallbacks s_callbacks = {
	.damage = vterm_cb_damage,
	.moverect = vterm_cb_moverect,
	.movecursor = vterm_cb_movecursor,
	.settermprop = vterm_cb_settermprop,
	.bell = vterm_cb_bell,
	.sb_pushline = vterm_cb_sb_pushline,
	.sb_popline = vterm_cb_sb_popline,
	.sb_clear = vterm_cb_sb_clear,
};

/* ---- mouse pointer sprite (16x16 A1 arrow, same as the lvgl pause UI) ---
 * White fill plus a black shadow (the black sprite is offset +1,+1). */

static const uint8_t k_cursor_a1_data[] = {
        0xC0, 0x00, 0xE0, 0x00, 0xF0, 0x00, 0xF8, 0x00, 0xFC, 0x00, 0xFE, 0x00, 0xFF, 0x00, 0xFF, 0x80,
        0xFF, 0xC0, 0xFF, 0xE0, 0xFE, 0x00, 0xEF, 0x00, 0xCF, 0x00, 0x87, 0x80, 0x07, 0x80, 0x03, 0x80,
};
static const uint8_t k_cursor_a1_data_black[] = {
        0x00, 0x00, 0x40, 0x00, 0x60, 0x00, 0x70, 0x00, 0x78, 0x00, 0x7C, 0x00, 0x7E, 0x00, 0x7F, 0x00,
        0x7F, 0x80, 0x7C, 0x00, 0x6C, 0x00, 0x46, 0x00, 0x06, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00,
};

/* Overlay the pointer directly in the rotated framebuffer: terminal
 * (px,py) -> fb (dst_x=py, dst_y=H-1-px). Panel byte = 6-bit colour << 2,
 * so white = 63<<2 and black = 0. */
static void vterm_draw_cursor(framebuffer_t *lcd, int cx, int cy)
{
	uint8_t *fb = (uint8_t *)framebuffer_get_framebuffer(lcd);
	const int H = (int)lcd->height;
	for (int y = 0; y < 16; y++) {
		int dst_x = cy + y;
		if (dst_x < 0 || dst_x >= (int)lcd->width)
			continue;
		uint8_t wlo = k_cursor_a1_data[y * 2], whi = k_cursor_a1_data[y * 2 + 1];
		uint8_t blo = k_cursor_a1_data_black[y * 2], bhi = k_cursor_a1_data_black[y * 2 + 1];
		for (int x = 0; x < 16; x++) {
			int dst_y = H - 1 - (cx + x);
			if (dst_y < 0 || dst_y >= H)
				continue;
			uint8_t wb = (x < 8) ? wlo : whi;
			uint8_t bb = (x < 8) ? blo : bhi;
			if ((wb >> (7 - (x & 7))) & 1)
				fb[(size_t)dst_y * lcd->width + dst_x] = (uint8_t)(63 << 2);
			else if ((bb >> (7 - (x & 7))) & 1)
				fb[(size_t)dst_y * lcd->width + dst_x] = 0;
		}
	}
}

/* Restore the fb patch under the last pointer position. */
static void vterm_ptr_restore(framebuffer_t *lcd)
{
	if (!s_ptr_saved)
		return;
	uint8_t *fb = (uint8_t *)framebuffer_get_framebuffer(lcd);
	const int H = (int)lcd->height;
	for (int y = 0; y < 16; y++) {
		int dst_x = s_ptr_y + y;
		if (dst_x < 0 || dst_x >= (int)lcd->width)
			continue;
		for (int x = 0; x < 16; x++) {
			int dst_y = H - 1 - (s_ptr_x + x);
			if (dst_y < 0 || dst_y >= H)
				continue;
			fb[(size_t)dst_y * lcd->width + dst_x] = s_ptr_save[y][x];
		}
	}
	s_ptr_saved = false;
}

/* Save the fb patch under (cx, cy) and draw the pointer on top. */
static void vterm_ptr_save_and_draw(framebuffer_t *lcd, int cx, int cy)
{
	uint8_t *fb = (uint8_t *)framebuffer_get_framebuffer(lcd);
	const int H = (int)lcd->height;
	for (int y = 0; y < 16; y++) {
		int dst_x = cy + y;
		for (int x = 0; x < 16; x++) {
			int dst_y = H - 1 - (cx + x);
			if (dst_x < 0 || dst_x >= (int)lcd->width || dst_y < 0 || dst_y >= H) {
				s_ptr_save[y][x] = 0;
				continue;
			}
			s_ptr_save[y][x] = fb[(size_t)dst_y * lcd->width + dst_x];
		}
	}
	s_ptr_x = cx;
	s_ptr_y = cy;
	s_ptr_saved = true;
	vterm_draw_cursor(lcd, cx, cy);
}

/* ---- blit job: runs on the blit worker after vsync ---------------------- */

static void vterm_frame_blit(framebuffer_t *lcd, void *user_ctx)
{
	(void)user_ctx;
	uint8_t *fb = (uint8_t *)framebuffer_get_framebuffer(lcd);
	assert(fb != NULL);

	/* rotate 90 degrees like the mac blit (panel mounted in landscape):
	 * fb(dst_x, dst_y) <- s_pixels(col = dst_x, row = h-1-dst_y). s_pixels
	 * holds the 6-bit colour in bits 5..0; shift to bits 7..2 so the panel
	 * data lines (data_gpio_nums[2..7]) see the colour.
	 * Transpose in 16x16 blocks through a stack buffer: both the s_pixels
	 * reads and the fb writes are linear runs. The naive per-pixel version
	 * strides 640 bytes on every read and thrashes the 32KB data cache —
	 * that was the measured 329 ms bottleneck (#1 word-packing didn't help
	 * because the blit is read-bound, not write-bound). */
	const int src_w = VTERM_COLS * TERM_CELL_W; /* 640 */
	const int src_h = VTERM_ROWS * TERM_CELL_H; /* 480 */
	const int fb_w = (int)lcd->width;           /* 480 */
	const int fb_h = (int)lcd->height;          /* 640 */
	uint8_t blk[16][16];
	for (int by = 0; by < src_h; by += 16) {
		for (int bx = 0; bx < src_w; bx += 16) {
			/* read the 16x16 block linearly, transposing into blk */
			for (int y = 0; y < 16; y++)
				for (int x = 0; x < 16; x++)
					blk[x][y] = s_pixels[(size_t)(by + y) * src_w + (bx + x)];
			/* write it out: fb(dst_x = by+y, dst_y = 639-bx-x) as
			 * 16-byte runs, 4 pixels per uint32 store */
			for (int x = 0; x < 16; x++) {
				uint32_t *row = (uint32_t *)(fb + (size_t)(fb_h - 1 - bx - x) * fb_w + by);
				row[0] = (uint32_t)(blk[x][0] << 2) | ((uint32_t)(blk[x][1] << 2) << 8) |
				         ((uint32_t)(blk[x][2] << 2) << 16) | ((uint32_t)(blk[x][3] << 2) << 24);
				row[1] = (uint32_t)(blk[x][4] << 2) | ((uint32_t)(blk[x][5] << 2) << 8) |
				         ((uint32_t)(blk[x][6] << 2) << 16) | ((uint32_t)(blk[x][7] << 2) << 24);
				row[2] = (uint32_t)(blk[x][8] << 2) | ((uint32_t)(blk[x][9] << 2) << 8) |
				         ((uint32_t)(blk[x][10] << 2) << 16) | ((uint32_t)(blk[x][11] << 2) << 24);
				row[3] = (uint32_t)(blk[x][12] << 2) | ((uint32_t)(blk[x][13] << 2) << 8) |
				         ((uint32_t)(blk[x][14] << 2) << 16) | ((uint32_t)(blk[x][15] << 2) << 24);
			}
		}
	}
	/* mouse pointer on top */
	vterm_draw_cursor(lcd, s_mouse_x, s_mouse_y);
}

/* ---- render helper ------------------------------------------------------ */

static void vterm_render_if_needed(void)
{
	if (s_dirty && !s_sync_update) {
		s_dirty = false;
		s_cursor_dirty = false;
		s_renderer.fb_out = s_fb;
		s_renderer.fb_w = s_fb_w;
		s_renderer.fb_h = s_fb_h;
		if (s_dmg_full || !s_dmg_has) {
			s_renderer.dirty_r0 = 0;
			s_renderer.dirty_r1 = -1; /* full frame */
		} else {
			s_renderer.dirty_r0 = s_dmg_r0;
			s_renderer.dirty_r1 = s_dmg_r1;
			s_renderer.dirty_c0 = s_dmg_c0;
			s_renderer.dirty_c1 = s_dmg_c1;
		}
		s_dmg_full = false;
		s_dmg_has = false;
		vterm_ptr_restore(s_lcd); /* remove pointer: a partial render may
		                           * not repaint its region */
		int r0 = s_renderer.dirty_r0;
		int r1 = s_renderer.dirty_r1 < 0 ? VTERM_ROWS : s_renderer.dirty_r1;
		bool fb_ok = term_render_frame_fb(&s_renderer);
		if (!fb_ok) {
			/* wide-glyph spill: fall back to s_pixels + full blit */
			s_renderer.fb_out = NULL;
			term_render_frame(&s_renderer);
		}
		s_renderer.fb_out = NULL;
		if (fb_ok) {
			/* frame in fb: overlay the pointer (the pre-render restore
			 * already cleared the old patch) */
			vterm_ptr_save_and_draw(s_lcd, s_mouse_x, s_mouse_y);
		} else {
			s_ptr_saved = false;
			mach_s3_blit_worker_submit_async(s_blit_worker, vterm_frame_blit, NULL);
		}
	} else if (s_cursor_dirty) {
		/* mouse moved: restore + redraw the pointer, no re-render */
		s_cursor_dirty = false;
		vterm_ptr_restore(s_lcd);
		vterm_ptr_save_and_draw(s_lcd, s_mouse_x, s_mouse_y);
	}
}

/* ---- mouse -------------------------------------------------------------- */

static int vterm_mouse_btn(input_mouse_button_t b)
{
	switch (b) {
	case INPUT_MOUSE_BTN_LEFT:   return 1;
	case INPUT_MOUSE_BTN_MIDDLE: return 2;
	case INPUT_MOUSE_BTN_RIGHT:  return 3;
	default:                     return 0;
	}
}

/* Handle one mouse event; sets s_dirty (re-render) or s_cursor_dirty
 * (re-blit only) as appropriate. */
static void vterm_mouse_event(const input_evt_t *evt)
{
	int row, col;

	if (evt->kind == INPUT_EVT_MOUSE_MOVE_REL) {
		/* USB mouse deltas are already in physical landscape space, which
		 * matches the unrotated 640x480 terminal layout */
		s_mouse_x += evt->u.mouse_move_rel.dx;
		s_mouse_y += evt->u.mouse_move_rel.dy;
	} else if (evt->kind == INPUT_EVT_MOUSE_MOVE_ABS) {
		/* absolute sources give framebuffer (480x640) coords; rotate like
		 * the lvgl indev: x <- fy, y <- (w-1) - fx */
		s_mouse_x = evt->u.mouse_move_abs.y;
		s_mouse_y = (VTERM_ROWS * TERM_CELL_H - 1) - (int)evt->u.mouse_move_abs.x;
	}

	if (s_mouse_x < 0) s_mouse_x = 0;
	if (s_mouse_x > VTERM_COLS * TERM_CELL_W - 1) s_mouse_x = VTERM_COLS * TERM_CELL_W - 1;
	if (s_mouse_y < 0) s_mouse_y = 0;
	if (s_mouse_y > VTERM_ROWS * TERM_CELL_H - 1) s_mouse_y = VTERM_ROWS * TERM_CELL_H - 1;
	col = s_mouse_x / TERM_CELL_W;
	row = s_mouse_y / TERM_CELL_H;

	switch (evt->kind) {
	case INPUT_EVT_MOUSE_MOVE_REL:
	case INPUT_EVT_MOUSE_MOVE_ABS:
		if (s_mouse_mode)
			vterm_mouse_move(s_vt, row, col, VTERM_MOD_NONE);
		else if (s_sel_dragging) {
			s_renderer.sel_active = true;
			s_renderer.sel_cur.row = row;
			s_renderer.sel_cur.col = col;
			s_dirty = true; /* selection highlight changes */
			dmg_mark_full();
		}
		s_cursor_dirty = true; /* pointer moved */
		break;

	case INPUT_EVT_MOUSE_DOWN: {
		int b = vterm_mouse_btn(evt->u.mouse_button.button);
		if (s_mouse_mode) {
			if (b)
				vterm_mouse_button(s_vt, b, true, VTERM_MOD_NONE);
		} else if (evt->u.mouse_button.button == INPUT_MOUSE_BTN_LEFT) {
			s_renderer.sel_active = false;
			s_renderer.sel_anchor.row = row;
			s_renderer.sel_anchor.col = col;
			s_renderer.sel_cur = s_renderer.sel_anchor;
			s_sel_dragging = true;
		}
		s_dirty = true;
		dmg_mark_full();
		break;
	}

	case INPUT_EVT_MOUSE_UP: {
		int b = vterm_mouse_btn(evt->u.mouse_button.button);
		if (s_mouse_mode) {
			if (b)
				vterm_mouse_button(s_vt, b, false, VTERM_MOD_NONE);
		} else if (evt->u.mouse_button.button == INPUT_MOUSE_BTN_LEFT && s_sel_dragging) {
			s_sel_dragging = false;
		}
		s_dirty = true;
		dmg_mark_full();
		break;
	}

	case INPUT_EVT_MOUSE_WHEEL: {
		int steps = evt->u.mouse_wheel.steps;
		if (s_mouse_mode) {
			/* wheel up = button 4, wheel down = button 5 (xterm) */
			int b = steps > 0 ? 4 : 5;
			vterm_mouse_button(s_vt, b, true, VTERM_MOD_NONE);
			vterm_mouse_button(s_vt, b, false, VTERM_MOD_NONE);
		} else {
			s_renderer.scroll_offset += steps;
			if (s_renderer.scroll_offset < 0)
				s_renderer.scroll_offset = 0;
			if (s_renderer.scroll_offset > s_sb_count)
				s_renderer.scroll_offset = s_sb_count;
		}
		s_dirty = true;
		dmg_mark_full();
		break;
	}

	default:
		break;
	}
}

/* ---- render loop ------------------------------------------------------- */

void vterm_esp32_selftest(framebuffer_t *lcd, mach_s3_blit_worker_t *blit_worker)
{
	s_lcd = lcd;
	s_fb = (uint8_t *)framebuffer_get_framebuffer(lcd);
	s_fb_w = (int)lcd->width;
	s_fb_h = (int)lcd->height;

	/* allocate once and reuse across mode entries: re-entering the vterm
	 * mode must not leak/re-malloc the pixel buffer (PSRAM can fragment
	 * after the Mac allocates) */
	if (!s_pixels) {
		s_pixels = heap_caps_malloc((size_t)VTERM_ROWS * VTERM_COLS * TERM_CELL_W *
		                                TERM_CELL_H * sizeof(uint8_t),
		                            MALLOC_CAP_SPIRAM);
		assert(s_pixels != NULL);
	}
	if (!s_sb) {
		s_sb = heap_caps_malloc((size_t)VTERM_SB_CAP * VTERM_COLS * sizeof(VTermScreenCell),
		                        MALLOC_CAP_SPIRAM);
		if (!s_sb)
			ESP_LOGW(TAG, "scrollback alloc failed (%d rows): scrollback disabled",
			         VTERM_SB_CAP);
	}
	s_dirty = false;
	s_cursor_dirty = false;
	s_sync_update = false;
	if (!s_vt) {
		s_vt = vterm_new(VTERM_ROWS, VTERM_COLS);
		vterm_set_utf8(s_vt, 1);
		vterm_output_set_callback(s_vt, vterm_output_cb, NULL);
		VTermScreen *scr0 = vterm_obtain_screen(s_vt);
		vterm_screen_enable_altscreen(scr0, 1);
		vterm_screen_enable_reflow(scr0, true);
		vterm_screen_reset(scr0, 1);
		vterm_screen_set_callbacks(scr0, &s_callbacks, NULL);
		term_render_init(&s_renderer, s_vt, NULL);
		s_renderer.pixels8 = s_pixels;
		s_renderer.sb_get_cell = vterm_sb_get_cell;
		s_renderer.sb_user = NULL;
		s_renderer.cursor_visible = true;
		s_renderer.blink_on = true;
		/* LNM (DECSET 20): a bare LF also returns to column 0, so output
		 * that sends only \n (no \r) still starts each line at the left */
		vterm_input_write(s_vt, "\033[20h", 6);
	}
	VTermScreen *scr = vterm_obtain_screen(s_vt);

	/* clean screen: the telnet host output is the terminal content */
	vterm_input_write(s_vt, "\033[2J\033[H", 7);
	vterm_screen_flush_damage(scr);

	/* a fresh session starts with an empty scrollback, no selection, and a
	 * centred mouse pointer */
	s_sb_count = 0;
	s_sb_head = 0;
	s_renderer.scroll_offset = 0;
	s_renderer.sel_active = false;
	s_sel_dragging = false;
	s_mouse_x = (VTERM_COLS * TERM_CELL_W) / 2;
	s_mouse_y = (VTERM_ROWS * TERM_CELL_H) / 2;
	term_render_frame(&s_renderer);

	/* write after a vsync, exactly like the blit worker would (the
	 * submit_and_wait round trip misbehaves in this context — returns
	 * false despite a valid worker) */
	if (!framebuffer_wait_vsync(lcd, 100)) {
		ESP_LOGW(TAG, "vsync wait timeout, writing anyway");
	}
	vterm_frame_blit(lcd, NULL);
}

/* ---- interactive loop: input -> VT100 -> render -> blit ----------------
 * Runs until F10/F12 is pressed. */

bool vterm_esp32_enter(framebuffer_t *lcd, mach_s3_blit_worker_t *blit_worker)
{
	char seq[8];
	input_evt_t evt;
	s_blit_worker = blit_worker;

	/* (re)init, then interactive */
	vterm_esp32_selftest(lcd, blit_worker);
	VTermScreen *scr = vterm_obtain_screen(s_vt);
	ESP_LOGI(TAG, "vterm mode: type on the USB keyboard, F10/F12 to exit");

	while (true) {
		if (input_pop(&evt)) {
			if (evt.kind == INPUT_EVT_KEY) {
				if (vterm_hid_mod_event(evt.u.key.code, evt.u.key.value)) {
					continue; /* modifier tracked, no output */
				}
				if (evt.u.key.value) { /* key press */
					if (evt.u.key.code == INPUT_KEY_F10 ||
					    evt.u.key.code == INPUT_KEY_F12) {
						return true;
					}
					/* scrollback: Shift+PageUp/Down walk the saved lines;
					 * any other key returns to the live view first */
					bool scroll_key = false;
					if (evt.u.key.code == INPUT_KEY_PAGEUP && vterm_hid_shift_down()) {
						if (s_renderer.scroll_offset < s_sb_count)
							s_renderer.scroll_offset++;
						s_dirty = true;
						dmg_mark_full();
						scroll_key = true;
					} else if (evt.u.key.code == INPUT_KEY_PAGEDOWN && vterm_hid_shift_down()) {
						if (s_renderer.scroll_offset > 0)
							s_renderer.scroll_offset--;
						s_dirty = true;
						dmg_mark_full();
						scroll_key = true;
					} else if (s_renderer.scroll_offset > 0) {
						s_renderer.scroll_offset = 0;
						s_dirty = true;
						dmg_mark_full();
					}
					/* typing invalidates the active mouse selection */
					if (s_renderer.sel_active) {
						s_renderer.sel_active = false;
						s_dirty = true;
						dmg_mark_full();
					}
					if (!scroll_key) {
						size_t n = vterm_hid_map(evt.u.key.code, seq, sizeof(seq));
						if (n) {
							/* connected: forward to host (host drives the display);
							 * otherwise local echo */
							if (!vterm_telnet_send((uint8_t *)seq, n)) {
								vterm_input_write(s_vt, seq, n);
								vterm_screen_flush_damage(scr);
							}
						}
					}
				}
			} else if (evt.kind == INPUT_EVT_MOUSE_MOVE_REL ||
			           evt.kind == INPUT_EVT_MOUSE_MOVE_ABS ||
			           evt.kind == INPUT_EVT_MOUSE_DOWN ||
			           evt.kind == INPUT_EVT_MOUSE_UP ||
			           evt.kind == INPUT_EVT_MOUSE_WHEEL) {
				vterm_mouse_event(&evt);
			}
		} else {
			/* drain remote telnet bytes into the terminal. The first pop
			 * blocks on the ring semaphore (up to the idle timeout), which
			 * replaces the per-iteration delay; once data arrives, drain the
			 * whole ring before rendering once so a big burst (pi -c replays
			 * ~850 KB of session history) is not throttled to one 1 KB chunk
			 * per loop. */
			uint8_t tbuf[1024];
			size_t tlen = 0;
			vterm_telnet_pop(tbuf, &tlen, sizeof(tbuf), pdMS_TO_TICKS(10));
			while (tlen) {
				vterm_input_write(s_vt, (char *)tbuf, tlen);
				vterm_screen_flush_damage(scr);
				tlen = 0;
				vterm_telnet_pop(tbuf, &tlen, sizeof(tbuf), 0);
			}
		}

		/* the text cursor stays steady (no blink) */

		vterm_render_if_needed();
	}
	return false;
}
