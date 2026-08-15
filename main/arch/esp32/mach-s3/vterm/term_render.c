/* term_render.c — pixel renderer for libvterm (platform-free).
 *
 * libvterm screen -> IBM VGA 8x16 glyphs (CP437) -> RGB888 pixels.
 * Bit order: MSB = leftmost pixel.
 */
#include <stddef.h>
#include <string.h>

#include "esp_timer.h"

#include "term_render.h"
#include "vga8x16.h"
#include "unicode_glyph.h"
#include "symbol_glyphs.h"
#include "emoji_glyphs.h"

/* ---- CP437: Unicode -> IBM VGA code page ------------------------------ */

/* Box-drawing + block characters (the XT-era interface staples). */
static const struct { uint32_t uni; uint8_t cp; } k_cp437_map[] = {
	{ 0x2500, 0xC4 }, { 0x2501, 0xC4 }, { 0x2502, 0xB3 }, { 0x2503, 0xB3 },
	{ 0x250C, 0xDA }, { 0x2510, 0xBF }, { 0x2514, 0xC0 }, { 0x2518, 0xD9 },
	{ 0x251C, 0xC3 }, { 0x2524, 0xB4 }, { 0x252C, 0xC2 }, { 0x2534, 0xC1 },
	{ 0x253C, 0xC5 }, { 0x2550, 0xCD }, { 0x2551, 0xBA }, { 0x2554, 0xC9 },
	{ 0x2557, 0xBB }, { 0x255A, 0xC8 }, { 0x255D, 0xBC }, { 0x2560, 0xCC },
	{ 0x2563, 0xB9 }, { 0x2566, 0xCB }, { 0x2569, 0xCA }, { 0x256C, 0xCE },
	{ 0x2580, 0xDF }, { 0x2584, 0xDC }, { 0x2588, 0xDB }, { 0x2591, 0xB0 },
	{ 0x2592, 0xB1 }, { 0x2593, 0xB2 }, { 0x258C, 0xDD }, { 0x2590, 0xDE },
	{ 0x25A0, 0xFE }, { 0x2014, 0xC4 },
	/* Latin-1 symbols that exist in CP437 */
	{ 0x00B0, 0xF8 }, { 0x00B1, 0xF1 }, { 0x00B7, 0xFA }, { 0x00B2, 0xFD },
	{ 0x00F7, 0xF6 }, { 0x00BD, 0xAB }, { 0x00BC, 0xAC },
	{ 0x00A1, 0xAD }, { 0x00AB, 0xAE }, { 0x00BB, 0xAF }, { 0x00BF, 0xA8 },
	{ 0x00A2, 0x9B }, { 0x00A3, 0x9C }, { 0x00A5, 0x9D },
	/* Latin-1 accented + Greek + misc: the rest of CP437 (all present in
	 * the VGA font, so map them back instead of the hollow placeholder) */
	{ 0x00C7, 0x80 }, { 0x00FC, 0x81 }, { 0x00E9, 0x82 }, { 0x00E2, 0x83 },
	{ 0x00E4, 0x84 }, { 0x00E0, 0x85 }, { 0x00E5, 0x86 }, { 0x00E7, 0x87 },
	{ 0x00EA, 0x88 }, { 0x00EB, 0x89 }, { 0x00E8, 0x8A }, { 0x00EF, 0x8B },
	{ 0x00EE, 0x8C }, { 0x00EC, 0x8D }, { 0x00C4, 0x8E }, { 0x00C5, 0x8F },
	{ 0x00C9, 0x90 }, { 0x00E6, 0x91 }, { 0x00C6, 0x92 }, { 0x00F4, 0x93 },
	{ 0x00F6, 0x94 }, { 0x00F2, 0x95 }, { 0x00FB, 0x96 }, { 0x00F9, 0x97 },
	{ 0x00FF, 0x98 }, { 0x00D6, 0x99 }, { 0x00DC, 0x9A }, { 0x20A7, 0x9E },
	{ 0x0192, 0x9F }, { 0x00E1, 0xA0 }, { 0x00ED, 0xA1 }, { 0x00F3, 0xA2 },
	{ 0x00FA, 0xA3 }, { 0x00F1, 0xA4 }, { 0x00D1, 0xA5 }, { 0x03B1, 0xE0 },
	{ 0x00DF, 0xE1 }, { 0x0393, 0xE2 }, { 0x03C0, 0xE3 }, { 0x03A3, 0xE4 },
	{ 0x03C3, 0xE5 }, { 0x03C4, 0xE7 }, { 0x03A6, 0xE8 }, { 0x0398, 0xE9 },
	{ 0x03A9, 0xEA }, { 0x03B4, 0xEB }, { 0x03C6, 0xED }, { 0x03B5, 0xEE },
	/* The rest of CP437 0x80-0xFF: double-line box corners, math symbols,
	 * µ and the blank at 0xFF. ANSI art uses these constantly (╡╢╖╕╞╟ and
	 * friends are BBS box-drawing staples); without them they rendered as
	 * hollow placeholders. Values cross-checked against Python's cp437
	 * codec. U+00A0 -> 0xFF is the CP437 blank (VGA glyph 0xFF is empty). */
	{ 0x00AA, 0xA6 }, { 0x00BA, 0xA7 }, { 0x2310, 0xA9 }, { 0x00AC, 0xAA },
	{ 0x2561, 0xB5 }, { 0x2562, 0xB6 }, { 0x2556, 0xB7 }, { 0x2555, 0xB8 },
	{ 0x255C, 0xBD }, { 0x255B, 0xBE }, { 0x255E, 0xC6 }, { 0x255F, 0xC7 },
	{ 0x2567, 0xCF }, { 0x2568, 0xD0 }, { 0x2564, 0xD1 }, { 0x2565, 0xD2 },
	{ 0x2559, 0xD3 }, { 0x2558, 0xD4 }, { 0x2552, 0xD5 }, { 0x2553, 0xD6 },
	{ 0x256B, 0xD7 }, { 0x256A, 0xD8 }, { 0x00B5, 0xE6 }, { 0x221E, 0xEC },
	{ 0x2229, 0xEF }, { 0x2261, 0xF0 }, { 0x2265, 0xF2 }, { 0x2264, 0xF3 },
	{ 0x2320, 0xF4 }, { 0x2321, 0xF5 }, { 0x2248, 0xF7 }, { 0x2219, 0xF9 },
	{ 0x221A, 0xFB }, { 0x207F, 0xFC }, { 0x00A0, 0xFF },
	/* NOTE: U+00D7 (×) has NO CP437 equivalent (0xD7 is Φ); it comes from
	 * the symbol table. */
};

uint8_t term_unicode_to_cp437(uint32_t cp)
{
	if (cp < 0x80)
		return (uint8_t)cp;
	for (size_t i = 0; i < sizeof(k_cp437_map) / sizeof(k_cp437_map[0]); i++) {
		if (k_cp437_map[i].uni == cp)
			return k_cp437_map[i].cp;
	}
	return 0xFF; /* unmapped */
}

/* ---- rendering -------------------------------------------------------- */

static uint32_t color_to_u32(const VTermColor *col)
{
	return ((uint32_t)col->rgb.red << 16) | ((uint32_t)col->rgb.green << 8) | (uint32_t)col->rgb.blue;
}

/* RGB888 -> 6-bit (64 colours) for the ESP32 panel's RGB222 data bus.
 * Channel levels {0,82,165,247} (R/B) and {0,81,162,243} (G); thresholds
 * are the channel midpoints 41/123/206 and 40/121/202. Unused by the host
 * SDL renderer (pixels8 == NULL there). */
static inline uint8_t rgb888_to_64(uint32_t rgb)
{
	uint8_t r = (rgb >> 16) & 0xFF;
	uint8_t g = (rgb >> 8) & 0xFF;
	uint8_t b = rgb & 0xFF;
	uint8_t ri = (r >= 206) ? 3 : (r >= 123) ? 2 : (r >= 41) ? 1 : 0;
	uint8_t gi = (g >= 202) ? 3 : (g >= 121) ? 2 : (g >= 40) ? 1 : 0;
	uint8_t bi = (b >= 206) ? 3 : (b >= 123) ? 2 : (b >= 41) ? 1 : 0;
	return (uint8_t)((ri << 4) | (gi << 2) | bi);
}

/* ---- direct-fb render state (single-threaded) ------------------------- */

static uint8_t s_cell[16][16]; /* transposed cell buffer: [gx][gy] */
static int s_cell_px0, s_cell_py0; /* current cell top-left in terminal px */

/* per-cell cache: the bg and glyph passes read the same cell, and the fb
 * loop reads it once more for the spill check — cache it so a cell's
 * screen fetch + colour resolution happens once, not 2-3 times. */
static VTermScreenCell s_cache_cell;
static bool s_cache_cell_valid;
static int s_cache_cell_row, s_cache_cell_col;
static bool s_cache_style_valid;
static int s_cache_style_row, s_cache_style_col;
static bool s_cache_cursor_here, s_cache_cur_block;
static uint32_t s_cache_on_b, s_cache_off_b;
static uint8_t s_cache_on_q, s_cache_off_q;

/* Write one pixel at absolute (px, py): to the direct rotated fb when
 * fb_out is set (transposed into the current cell's buffer), otherwise to
 * pixels8. q is the pre-quantized 6-bit colour (rgb888_to_64 result). */
static inline void rp_put(term_renderer_t *r, int px, int py, uint8_t q)
{
	if (r->fb_out)
		s_cell[px - s_cell_px0][py - s_cell_py0] = (uint8_t)(q << 2);
	else
		r->pixels8[py * r->win_w + px] = q;
}

/* ---- CJK: Unicode -> 16x16 glyph (binary search, no GB2312 hop) -------- */

/* Returns the glyph for a code point, or NULL. */
static const uint8_t *unicode_glyph(uint32_t cp)
{
	int lo = 0, hi = UNICODE_GLYPH_COUNT - 1;
	while (lo <= hi) {
		int mid = (lo + hi) / 2;
		if (k_unicode_glyphs[mid].uni == cp)
			return k_unicode_glyphs[mid].glyph;
		if (k_unicode_glyphs[mid].uni < cp)
			lo = mid + 1;
		else
			hi = mid - 1;
	}
	return NULL;
}

/* paint a 16x16 glyph starting at cell (row, col): it spans two
 * cell columns (8 px each). Returns true if painted. */
static bool paint_cjk(term_renderer_t *r, int row, int col, uint32_t cp,
                      uint8_t on_q)
{
	const uint8_t *g = unicode_glyph(cp);
	if (!g)
		return false;
	const int px0 = col * TERM_CELL_W;
	const int py0 = row * TERM_CELL_H;
	for (int gy = 0; gy < 16; gy++) {
		uint8_t lo = g[gy * 2], hi = g[gy * 2 + 1];
		for (int gx = 0; gx < 16; gx++) {
			uint8_t byte = (gx < 8) ? lo : hi;
			if ((byte >> (7 - (gx & 7))) & 1)
				rp_put(r, px0 + gx, py0 + gy, on_q);
		}
	}
	return true;
}

/* ---- TUI symbols: Unicode -> glyph (binary search) ---------------------- */

static const uint8_t *symbol_glyph(uint32_t cp, int *width)
{
	int lo = 0, hi = SYMBOL_GLYPH_COUNT - 1;
	while (lo <= hi) {
		int mid = (lo + hi) / 2;
		if (k_symbol_glyphs[mid].uni == cp) {
			*width = k_symbol_glyphs[mid].w;
			return k_symbol_glyphs[mid].glyph;
		}
		if (k_symbol_glyphs[mid].uni < cp)
			lo = mid + 1;
		else
			hi = mid - 1;
	}
	return NULL;
}

/* ---- emoji: Unicode -> 16x16 glyph (binary search) --------------------- */

static const uint8_t *emoji_glyph(uint32_t cp)
{
	int lo = 0, hi = EMOJI_GLYPH_COUNT - 1;
	while (lo <= hi) {
		int mid = (lo + hi) / 2;
		if (k_emoji_glyphs[mid].uni == cp)
			return k_emoji_glyphs[mid].glyph;
		if (k_emoji_glyphs[mid].uni < cp)
			lo = mid + 1;
		else
			hi = mid - 1;
	}
	return NULL;
}

/* paint a full-width 16x16 emoji spanning two cell columns */
static bool paint_emoji(term_renderer_t *r, int row, int col, uint32_t cp,
                        uint8_t on_q)
{
	const uint8_t *g = emoji_glyph(cp);
	if (!g)
		return false;
	const int px0 = col * TERM_CELL_W;
	const int py0 = row * TERM_CELL_H;
	for (int gy = 0; gy < 16; gy++) {
		uint8_t lo = g[gy * 2], hi = g[gy * 2 + 1];
		for (int gx = 0; gx < 16; gx++) {
			uint8_t byte = (gx < 8) ? lo : hi;
			if ((byte >> (7 - (gx & 7))) & 1)
				rp_put(r, px0 + gx, py0 + gy, on_q);
		}
	}
	return true;
}

/* ---- Unicode whitespace: render as blank cells (not placeholders) ------ */

static bool is_space_cp(uint32_t cp)
{
	return cp == 0x20 || cp == 0xA0 ||
	       (cp >= 0x2000 && cp <= 0x200A) || cp == 0x202F || cp == 0x205F ||
	       cp == 0x3000;
}

/* paint one cell (row r, col c) into the pixel buffer */
/* read a cell through the renderer's source (live screen or scrollback). */
static bool get_render_cell(const term_renderer_t *r, int row, int col, VTermScreenCell *cell)
{
	if (s_cache_cell_valid && s_cache_cell_row == row && s_cache_cell_col == col) {
		*cell = s_cache_cell;
		return true;
	}
	bool ok;
	if (r->scroll_offset > 0 && row < r->scroll_offset) {
		int sb_row = r->scroll_offset - 1 - row;
		ok = r->sb_get_cell && r->sb_get_cell(r->sb_user, sb_row, col, cell);
	} else {
		VTermPos p = { .row = row - r->scroll_offset, .col = col };
		ok = vterm_screen_get_cell(r->screen, p, cell);
	}
	if (ok) {
		s_cache_cell = *cell;
		s_cache_cell_valid = true;
		s_cache_cell_row = row;
		s_cache_cell_col = col;
	}
	return ok;
}

/* compute on/off colours (fg/bg, bold, reverse, selection, block cursor).
 * on = glyph colour, off = background. */
static void cell_style(term_renderer_t *r, int row, int col, const VTermScreenCell *cell,
                       bool *cursor_here, bool *cur_block, uint32_t *on_b, uint32_t *off_b,
                       uint8_t *on_q, uint8_t *off_q)
{
	if (s_cache_style_valid && s_cache_style_row == row && s_cache_style_col == col) {
		*cursor_here = s_cache_cursor_here;
		*cur_block = s_cache_cur_block;
		*on_b = s_cache_on_b;
		*off_b = s_cache_off_b;
		*on_q = s_cache_on_q;
		*off_q = s_cache_off_q;
		return;
	}
	VTermColor fg = cell->fg, bg = cell->bg;
	/* bold maps palette 0-7 to 8-15 (xterm default boldColor) */
	if (cell->attrs.bold && VTERM_COLOR_IS_INDEXED(&fg) && fg.indexed.idx < 8)
		fg.indexed.idx += 8;
	vterm_state_convert_color_to_rgb(r->state, &fg);
	vterm_state_convert_color_to_rgb(r->state, &bg);

	/* cursor: reverse-video, blink phase from the host. Only in the live
	 * viewport (scrollback shows no cursor). Shape from DECSCUSR:
	 * 1 block, 2 underline, 3 bar-left. Blink: once an app issued
	 * DECSCUSR we honour its steady/blink bit; before that (plain shell
	 * prompt) the cursor blinks by default, as on VT100s. */
	*cursor_here = r->scroll_offset == 0 && r->cursor_visible &&
		row == r->cursor.row && col == r->cursor.col;
	if (*cursor_here) {
		if (r->cursor_shape == 0)
			r->cursor_shape = VTERM_PROP_CURSORSHAPE_BLOCK;
		bool blink = r->cursor_shape_set ? (r->cursor_blink != 0) : true;
		if (blink && !r->blink_on)
			*cursor_here = false; /* blink dark phase */
	}

	uint32_t on = cell->attrs.reverse ? color_to_u32(&bg) : color_to_u32(&fg);
	uint32_t off = cell->attrs.reverse ? color_to_u32(&fg) : color_to_u32(&bg);

	/* mouse selection: streaming highlight — the top row is anchored at
	 * the TOP point's column and runs to its last non-blank cell, the
	 * bottom row ends at the BOTTOM point's column (both matching
	 * xterm.js), and interior rows are highlighted full width. Block mode
	 * (Alt+drag) selects the plain rectangle. Selection rows are visible
	 * rows; the host translates them by the scroll delta so the highlight
	 * follows the content when the view scrolls. */
	bool in_sel = false;
	if (r->sel_active) {
		int r0 = r->sel_anchor.row < r->sel_cur.row ? r->sel_anchor.row : r->sel_cur.row;
		int r1 = r->sel_anchor.row > r->sel_cur.row ? r->sel_anchor.row : r->sel_cur.row;
		int c_top = (r->sel_anchor.row == r0) ? r->sel_anchor.col : r->sel_cur.col;
		int c_bot = (r->sel_anchor.row == r1) ? r->sel_anchor.col : r->sel_cur.col;
		int c0 = r->sel_anchor.col < r->sel_cur.col ? r->sel_anchor.col : r->sel_cur.col;
		int c1 = r->sel_anchor.col > r->sel_cur.col ? r->sel_anchor.col : r->sel_cur.col;
		if (row >= r0 && row <= r1) {
			if (r->sel_block)
				in_sel = col >= c0 && col <= c1;
			else if (row == r0 && row == r1)
				in_sel = col >= c0 && col <= c1;
			else if (row == r0)
				in_sel = col >= c_top && (r->sel_line_end < 0 || col <= r->sel_line_end);
			else if (row == r1)
				in_sel = col <= c_bot;
			else
				in_sel = true; /* interior rows: full width */
		}
	}
	if (in_sel) {
		/* xterm-style selection: reverse video (swap fg/bg), not a custom
		 * colour. A cell that is already reverse shows normally (double
		 * inversion cancels), matching xterm. */
		uint32_t tmp = on;
		on = off;
		off = tmp;
	}
	/* block cursor: full-cell inversion. Underline/bar shapes are handled
	 * per-pixel in the glyph loop. */
	*cur_block = *cursor_here && r->cursor_shape == VTERM_PROP_CURSORSHAPE_BLOCK;
	*on_b = *cur_block ? off : on;
	*off_b = *cur_block ? on : off;
	*on_q = rgb888_to_64(*on_b);
	*off_q = rgb888_to_64(*off_b);
	s_cache_style_valid = true;
	s_cache_style_row = row;
	s_cache_style_col = col;
	s_cache_cursor_here = *cursor_here;
	s_cache_cur_block = *cur_block;
	s_cache_on_b = *on_b;
	s_cache_off_b = *off_b;
	s_cache_on_q = *on_q;
	s_cache_off_q = *off_q;
}

/* pass 1: background — spans exactly the layout width, like a real
 * terminal.  Wide glyphs (16px render, 1-cell layout) never bleed their
 * background into the next cell. */
static void paint_background(term_renderer_t *r, int row, int col)
{
	VTermScreenCell cell;
	if (!get_render_cell(r, row, col, &cell))
		memset(&cell, 0, sizeof(cell)); /* blank */
	if (cell.chars[0] == (uint32_t)-1 || cell.width == 0)
		return; /* gap/continuation: covered by the anchor cell's width */
	bool cursor_here, cur_block;
	uint32_t on_b, off_b;
	uint8_t on_q, off_q;
	cell_style(r, row, col, &cell, &cursor_here, &cur_block, &on_b, &off_b, &on_q, &off_q);
	int w = (cell.width >= 2) ? 2 : 1;
	const int px0 = col * TERM_CELL_W;
	const int py0 = row * TERM_CELL_H;
	if (r->fb_out) {
		/* uniform background: one memset (transpose is a no-op for a
		 * solid cell), instead of 128 per-pixel rp_put calls */
		memset(s_cell, (int)(off_q << 2), (size_t)w * TERM_CELL_W * TERM_CELL_H);
	} else {
		for (int y = 0; y < TERM_CELL_H; y++)
			for (int x = 0; x < w * TERM_CELL_W; x++)
				rp_put(r, px0 + x, py0 + y, off_q);
	}
}

/* pass 2: glyph — natural width (8px or 16px).  Only on-pixels are
 * written; the background pass already filled the cell. */
static void paint_glyph(term_renderer_t *r, int row, int col)
{
	VTermScreenCell cell;
	if (!get_render_cell(r, row, col, &cell))
		memset(&cell, 0, sizeof(cell)); /* blank */
	if (cell.chars[0] == (uint32_t)-1 || cell.width == 0)
		return;
	bool cursor_here, cur_block;
	uint32_t on_b, off_b;
	uint8_t on_q, off_q;
	cell_style(r, row, col, &cell, &cursor_here, &cur_block, &on_b, &off_b, &on_q, &off_q);

	uint32_t cp = cell.chars[0];
	if (cp == 0)
		cp = ' ';
	const int px0 = col * TERM_CELL_W;
	const int py0 = row * TERM_CELL_H;

	/* Unicode whitespace: blank cell, honouring double-width (U+3000) */
	if (is_space_cp(cp)) {
		int w = (cell.width >= 2) ? 2 : 1;
		/* underline/strike still apply to blank cells */
		if (cell.attrs.underline)
			for (int x = 0; x < w * TERM_CELL_W; x++)
				rp_put(r, px0 + x, py0 + TERM_CELL_H - 1, on_q);
		if (cell.attrs.strike)
			for (int x = 0; x < w * TERM_CELL_W; x++)
				rp_put(r, px0 + x, py0 + TERM_CELL_H / 2, on_q);
		return;
	}

	/* SGR 5 blink dark phase / SGR 8 conceal: no glyph, background stays */
	if ((cell.attrs.blink && !r->blink_on) || cell.attrs.conceal)
		return;

	uint8_t idx = term_unicode_to_cp437(cp);
	if (idx != 0xFF || cp == 0xFF) {
		/* IBM VGA glyphs: MSB = leftmost pixel */
		const uint8_t *glyph = vga8x16[idx];
		bool cursor_underline = cursor_here && !cur_block &&
			r->cursor_shape == VTERM_PROP_CURSORSHAPE_UNDERLINE;
		bool cursor_bar = cursor_here && !cur_block &&
			r->cursor_shape == VTERM_PROP_CURSORSHAPE_BAR_LEFT;
		for (int gy = 0; gy < TERM_CELL_H; gy++) {
			uint8_t line = glyph[gy];
			if (cursor_underline || cursor_bar) {
				/* underline/bar cursor: invert a band of the cell */
				for (int gx = 0; gx < TERM_CELL_W; gx++) {
					bool set = (line >> (7 - gx)) & 1;
					bool cursor_px = (cursor_underline && gy >= TERM_CELL_H - 2) ||
						(cursor_bar && gx < 2);
					if (cursor_px)
						/* inverted: glyph pixels take the background
						 * colour, the rest take the foreground colour */
						rp_put(r, px0 + gx, py0 + gy, set ? off_q : on_q);
					else if (set)
						rp_put(r, px0 + gx, py0 + gy, on_q);
				}
			} else {
				for (int gx = 0; gx < TERM_CELL_W; gx++)
					if ((line >> (7 - gx)) & 1)
						rp_put(r, px0 + gx, py0 + gy, on_q);
			}
		}
		/* SGR 4 underline: bottom row */
		if (cell.attrs.underline) {
			for (int x = 0; x < TERM_CELL_W; x++)
				rp_put(r, px0 + x, py0 + TERM_CELL_H - 1, on_q);
		}
		/* SGR 9 strikethrough: middle row */
		if (cell.attrs.strike) {
			for (int x = 0; x < TERM_CELL_W; x++)
				rp_put(r, px0 + x, py0 + TERM_CELL_H / 2, on_q);
		}
		return;
	}

	if (cell.width >= 2 && paint_cjk(r, row, col, cp, on_q)) {
		/* decorations apply to CJK cells too (16 px wide) */
		if (cell.attrs.underline)
			for (int x = 0; x < 16; x++)
				rp_put(r, px0 + x, py0 + TERM_CELL_H - 1, on_q);
		if (cell.attrs.strike)
			for (int x = 0; x < 16; x++)
				rp_put(r, px0 + x, py0 + TERM_CELL_H / 2, on_q);
		return;
	}
	/* emoji: 16x16, rendered even when layout width is 1 (wide glyph) */
	if (paint_emoji(r, row, col, cp, on_q))
		return;
	/* TUI symbols from unifont: fixed 16px height, natural width — a
	 * 16x16 source renders 16px wide (may spill into the next cell),
	 * an 8x16 source renders 8px */
	int sw = 1;
	const uint8_t *sg = NULL;
	if (cell.width >= 2)
		sg = emoji_glyph(cp);    /* full-width emoji */
	if (!sg)
		sg = symbol_glyph(cp, &sw);
	if (sg) {
		if (sw >= 2) {
			for (int gy = 0; gy < 16; gy++) {
				uint8_t lo = sg[gy * 2], hi = sg[gy * 2 + 1];
				for (int gx = 0; gx < 16; gx++) {
					uint8_t byte = (gx < 8) ? lo : hi;
					if ((byte >> (7 - (gx & 7))) & 1)
						rp_put(r, px0 + gx, py0 + gy, on_q);
				}
			}
		} else {
			for (int gy = 0; gy < TERM_CELL_H; gy++) {
				uint8_t line = sg[gy];
				for (int gx = 0; gx < TERM_CELL_W; gx++)
					if ((line >> (7 - gx)) & 1)
						rp_put(r, px0 + gx, py0 + gy, on_q);
			}
		}
		return;
	}
	/* unmapped: hollow box placeholder spanning the layout width */
	int pw = (cell.width >= 2) ? 2 : 1;
	for (int y = 0; y < TERM_CELL_H; y++)
		for (int x = 0; x < pw * TERM_CELL_W; x++) {
			bool edge = (x == 0 || y == 0 || x == pw * TERM_CELL_W - 1 || y == TERM_CELL_H - 1);
			if (edge)
				rp_put(r, px0 + x, py0 + y, on_q);
		}
}

void term_render_init(term_renderer_t *r, VTerm *vt, uint32_t *pixels)
{
	memset(r, 0, sizeof(*r));
	vterm_get_size(vt, &r->rows, &r->cols);
	r->screen = vterm_obtain_screen(vt);
	r->state = vterm_obtain_state(vt);
	r->pixels = pixels;
	r->win_w = r->cols * TERM_CELL_W;
	r->win_h = r->rows * TERM_CELL_H;
	r->blink_on = true;
}

void term_render_frame(term_renderer_t *r)
{
	s_cache_cell_valid = false;
	s_cache_style_valid = false;
	int r0 = 0, r1 = -1, c_top = 0;
	if (r->sel_active) {
		r0 = r->sel_anchor.row < r->sel_cur.row ? r->sel_anchor.row : r->sel_cur.row;
		r1 = r->sel_anchor.row > r->sel_cur.row ? r->sel_anchor.row : r->sel_cur.row;
		c_top = (r->sel_anchor.row == r0) ? r->sel_anchor.col : r->sel_cur.col;
	}
	for (int row = 0; row < r->rows; row++) {
		/* last non-blank col of the TOP row, for the streaming highlight
		 * (read through get_render_cell so a scrolled view resolves
		 * scrollback content correctly). Interior rows are full width and
		 * the bottom row is a hard range, so they do not need the scan. */
		r->sel_line_end = -1;
		if (r->sel_active && row == r0 && row != r1) {
			for (int col = c_top; col < r->cols; col++) {
				VTermScreenCell cell;
				if (get_render_cell(r, row, col, &cell) && cell.chars[0] != 0 &&
				    cell.chars[0] != (uint32_t)-1)
					r->sel_line_end = col;
			}
		}
		/* pass 1: background spans exactly the layout width; pass 2:
		 * glyphs at natural width (may spill into the next column) */
		for (int col = 0; col < r->cols; col++)
			paint_background(r, row, col);
		for (int col = 0; col < r->cols; col++)
			paint_glyph(r, row, col);
	}
}

/* ---- direct rotated-fb output (on-the-fly, no s_pixels) ---------------- */

static inline uint32_t fb_pack4(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
	return (uint32_t)a | ((uint32_t)b << 8) | ((uint32_t)c << 16) | ((uint32_t)d << 24);
}

/* Write the current cell buffer s_cell[gx][gy] (gx = terminal px offset,
 * gy = terminal py offset) to fb as 16-byte linear runs:
 * fb(dst_x = py0+gy, dst_y = fb_h-1-px0-gx). */
static void flush_cell_fb(term_renderer_t *r, int w)
{
	const int npx = w * TERM_CELL_W;
	for (int gx = 0; gx < npx; gx++) {
		uint32_t *row = (uint32_t *)(r->fb_out +
			(size_t)(r->fb_h - 1 - s_cell_px0 - gx) * r->fb_w + s_cell_py0);
		row[0] = fb_pack4(s_cell[gx][0], s_cell[gx][1], s_cell[gx][2], s_cell[gx][3]);
		row[1] = fb_pack4(s_cell[gx][4], s_cell[gx][5], s_cell[gx][6], s_cell[gx][7]);
		row[2] = fb_pack4(s_cell[gx][8], s_cell[gx][9], s_cell[gx][10], s_cell[gx][11]);
		row[3] = fb_pack4(s_cell[gx][12], s_cell[gx][13], s_cell[gx][14], s_cell[gx][15]);
	}
}

/* True if the cell's glyph renders 16px wide while its layout width is 1
 * (emoji / wide symbol), i.e. it spills into the next cell. The single-pass
 * fb renderer cannot handle this (the neighbour's background would overwrite
 * the spill), so such frames fall back to the s_pixels path. */
static bool cell_spills(term_renderer_t *r, const VTermScreenCell *cell)
{
	if (cell->width >= 2)
		return false;
	uint32_t cp = cell->chars[0];
	if (cp == 0)
		cp = ' ';
	if (is_space_cp(cp))
		return false;
	if ((cell->attrs.blink && !r->blink_on) || cell->attrs.conceal)
		return false;
	uint8_t idx = term_unicode_to_cp437(cp);
	if (idx != 0xFF || cp == 0xFF)
		return false; /* VGA 8px */
	if (emoji_glyph(cp))
		return true;
	int sw = 1;
	const uint8_t *sg = symbol_glyph(cp, &sw);
	return sg && sw >= 2;
}

bool term_render_frame_fb(term_renderer_t *r)
{
	s_cache_cell_valid = false;
	s_cache_style_valid = false;
	const bool partial = r->dirty_r0 <= r->dirty_r1;
	const int row0 = partial ? r->dirty_r0 : 0;
	const int row1 = partial ? r->dirty_r1 : r->rows - 1;
	const int col0 = partial ? r->dirty_c0 : 0;
	const int col1 = partial ? r->dirty_c1 : r->cols - 1;
	int r0 = 0, r1 = -1, c_top = 0;
	if (r->sel_active) {
		r0 = r->sel_anchor.row < r->sel_cur.row ? r->sel_anchor.row : r->sel_cur.row;
		r1 = r->sel_anchor.row > r->sel_cur.row ? r->sel_anchor.row : r->sel_cur.row;
		c_top = (r->sel_anchor.row == r0) ? r->sel_anchor.col : r->sel_cur.col;
	}
	for (int row = row0; row <= row1; row++) {
		r->sel_line_end = -1;
		if (r->sel_active && row == r0 && row != r1) {
			/* top row only: last non-blank col for the streaming highlight
			 * (interior rows are full width, the bottom row is a hard
			 * range; read through get_render_cell so a scrolled view
			 * resolves scrollback content correctly) */
			for (int col = c_top; col < r->cols; col++) {
				VTermScreenCell cell;
				if (get_render_cell(r, row, col, &cell) && cell.chars[0] != 0 &&
				    cell.chars[0] != (uint32_t)-1)
					r->sel_line_end = col;
			}
		}
		for (int col = col0; col <= col1; col++) {
			VTermScreenCell cell;
			if (!get_render_cell(r, row, col, &cell))
				memset(&cell, 0, sizeof(cell));
			if (cell.chars[0] == (uint32_t)-1 || cell.width == 0)
				continue; /* gap/continuation: covered by the anchor cell */
			if (cell_spills(r, &cell))
				return false; /* fall back to s_pixels path */
			s_cell_px0 = col * TERM_CELL_W;
			s_cell_py0 = row * TERM_CELL_H;
			memset(s_cell, 0, sizeof(s_cell));
			paint_background(r, row, col);
			paint_glyph(r, row, col);
			flush_cell_fb(r, (cell.width >= 2) ? 2 : 1);
		}
	}
	return true;
}

/* Read a cell through the renderer's source (live screen or scrollback). */
/* Extract the text of the current selection as UTF-8 (lines joined with
 * \n, trailing spaces stripped). Returns length. */
static size_t utf8_put(uint32_t cp, char *out)
{
	if (cp < 0x80) { out[0] = (char)cp; return 1; }
	if (cp < 0x800) { out[0] = (char)(0xC0 | (cp >> 6)); out[1] = (char)(0x80 | (cp & 0x3F)); return 2; }
	if (cp < 0x10000) { out[0] = (char)(0xE0 | (cp >> 12)); out[1] = (char)(0x80 | ((cp >> 6) & 0x3F)); out[2] = (char)(0x80 | (cp & 0x3F)); return 3; }
	out[0] = (char)(0xF0 | (cp >> 18)); out[1] = (char)(0x80 | ((cp >> 12) & 0x3F)); out[2] = (char)(0x80 | ((cp >> 6) & 0x3F)); out[3] = (char)(0x80 | (cp & 0x3F)); return 4;
}

size_t term_render_selected_text(const term_renderer_t *r, char *out, size_t cap)
{
	size_t o = 0;
	int r0 = r->sel_anchor.row < r->sel_cur.row ? r->sel_anchor.row : r->sel_cur.row;
	int r1 = r->sel_anchor.row > r->sel_cur.row ? r->sel_anchor.row : r->sel_cur.row;
	int c0 = r->sel_anchor.col < r->sel_cur.col ? r->sel_anchor.col : r->sel_cur.col;
	int c1 = r->sel_anchor.col > r->sel_cur.col ? r->sel_anchor.col : r->sel_cur.col;
	int c_top = (r->sel_anchor.row == r0) ? r->sel_anchor.col : r->sel_cur.col;
	int c_bot = (r->sel_anchor.row == r1) ? r->sel_anchor.col : r->sel_cur.col;

	for (int row = r0; row <= r1; row++) {
		/* selection rows are visible rows; rows scrolled out of the
		 * viewport contribute nothing */
		if (row < 0 || row >= r->rows)
			continue;
		/* per-row span matches the highlight geometry */
		int from, to;
		if (r->sel_block || (row == r0 && row == r1)) {
			from = c0;
			to = c1;
		} else if (row == r0) {
			from = c_top;
			to = r->cols - 1;
		} else if (row == r1) {
			from = 0;
			to = c_bot;
		} else {
			from = 0;
			to = r->cols - 1;
		}
		int last = from - 1;
		if (!r->sel_block) {
			for (int col = from; col <= to; col++) {
				VTermScreenCell cell;
				if (get_render_cell(r, row, col, &cell) && cell.chars[0] != 0 &&
				    cell.chars[0] != (uint32_t)-1)
					last = col;
			}
		} else {
			last = to; /* block mode keeps the full column span */
		}
		if (last < from)
			continue; /* empty line */
		for (int col = from; col <= last; col++) {
			VTermScreenCell cell;
			if (!get_render_cell(r, row, col, &cell))
				continue;
			for (int i = 0; i < VTERM_MAX_CHARS_PER_CELL && cell.chars[i]; i++) {
				if (cell.chars[i] == (uint32_t)-1)
					continue;
				if (o + 8 < cap)
					o += utf8_put(cell.chars[i], out + o);
			}
		}
		if (row < r1 && o + 2 < cap)
			out[o++] = '\n';
	}
	out[o] = '\0';
	return o;
}

static bool cell_blank(const term_renderer_t *r, int row, int col)
{
	VTermScreenCell cell;
	VTermPos p = { .row = row, .col = col };
	if (!vterm_screen_get_cell(r->screen, p, &cell))
		return true;
	return cell.chars[0] == 0 || cell.chars[0] == ' ' || cell.chars[0] == (uint32_t)-1;
}
void term_render_select_word(term_renderer_t *r, int row, int col)
{
	int c0 = col, c1 = col;
	while (c0 > 0 && !cell_blank(r, row, c0 - 1))
		c0--;
	while (c1 < r->cols - 1 && !cell_blank(r, row, c1 + 1))
		c1++;
	r->sel_active = true;
	r->sel_anchor.row = row; r->sel_anchor.col = c0;
	r->sel_cur.row = row; r->sel_cur.col = c1;
}

/* xterm triple-click: select the whole line. */
void term_render_select_line(term_renderer_t *r, int row)
{
	r->sel_active = true;
	r->sel_anchor.row = row; r->sel_anchor.col = 0;
	r->sel_cur.row = row; r->sel_cur.col = r->cols - 1;
}
