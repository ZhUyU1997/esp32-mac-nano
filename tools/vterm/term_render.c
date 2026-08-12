/* term_render.c — pixel renderer for libvterm (platform-free).
 *
 * libvterm screen -> IBM VGA 8x16 glyphs (CP437) -> RGB888 pixels.
 * Bit order: MSB = leftmost pixel.
 */
#include <stddef.h>
#include <string.h>

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
	{ 0x251C, 0xC3 }, { 0x2524, 0xB4 }, { 0x252C, 0xC2 }, { 0x2534, 0xD4 },
	{ 0x253C, 0xC5 }, { 0x2550, 0xCD }, { 0x2551, 0xBA }, { 0x2554, 0xC9 },
	{ 0x2557, 0xBB }, { 0x255A, 0xC8 }, { 0x255D, 0xBC }, { 0x2560, 0xCC },
	{ 0x2563, 0xB9 }, { 0x2566, 0xCA }, { 0x2569, 0xCB }, { 0x256C, 0xCE },
	{ 0x2580, 0xDF }, { 0x2584, 0xDC }, { 0x2588, 0xDB }, { 0x2591, 0xB0 },
	{ 0x2592, 0xB2 }, { 0x2593, 0xB1 }, { 0x25A0, 0xFE }, { 0x2014, 0xC4 },
	/* Latin-1 symbols that exist in CP437 */
	{ 0x00B0, 0xF8 }, { 0x00B1, 0xF1 }, { 0x00B7, 0xFA }, { 0x00B2, 0xFD },
	{ 0x00F7, 0xF6 }, { 0x00BD, 0xAB }, { 0x00BC, 0xAC },
	{ 0x00A1, 0xAD }, { 0x00AB, 0xAE }, { 0x00BB, 0xAF }, { 0x00BF, 0xA8 },
	{ 0x00A2, 0x9B }, { 0x00A3, 0x9C }, { 0x00A5, 0x9D },
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
                      uint32_t on, uint32_t off)
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
			bool set = (byte >> (7 - (gx & 7))) & 1;
			r->pixels[(py0 + gy) * r->win_w + (px0 + gx)] = set ? on : off;
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
                        uint32_t on, uint32_t off)
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
			bool set = (byte >> (7 - (gx & 7))) & 1;
			r->pixels[(py0 + gy) * r->win_w + (px0 + gx)] = set ? on : off;
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
static bool is_wide_glyph_cp(uint32_t cp)
{
	/* 16x16 symbols and emoji render 16px wide even when layout width is
	 * 1; 8x16 glyphs fit their cell exactly */
	if (emoji_glyph(cp))
		return true;
	int sw = 1;
	if (symbol_glyph(cp, &sw) != NULL)
		return sw >= 2;
	return false;
}

/* Unicode whitespace: blank cell, honouring double-width (U+3000).
 * A cell covered by a wide glyph's overflow (wide_occ) is left alone so
 * the 16px glyph stays visible. */
static void paint_blank(term_renderer_t *r, int row, int col, int px0, int py0,
                        int w, uint32_t on_b, uint32_t off_b, bool covered)
{
	if (covered)
		return;
	for (int y = 0; y < TERM_CELL_H; y++)
		for (int x = 0; x < w * TERM_CELL_W; x++)
		r->pixels[(py0 + y) * r->win_w + (px0 + x)] = off_b;
}

/* paint a single cell.  wide_occ[col] is true when a preceding wide glyph
 * (16px render with 1-cell layout) already painted into this column. */
static void paint_cell(term_renderer_t *r, int row, int col, const bool *wide_occ)
{
	VTermScreenCell cell;
	bool have = false;

	if (r->scroll_offset > 0 && row < r->scroll_offset) {
		/* scrollback region: host storage, most recent line on top */
		int sb_row = r->scroll_offset - 1 - row;
		if (r->sb_get_cell && r->sb_get_cell(r->sb_user, sb_row, col, &cell))
			have = true;
	} else if (row >= r->scroll_offset) {
		VTermPos pos = { .row = row - r->scroll_offset, .col = col };
		have = vterm_screen_get_cell(r->screen, pos, &cell);
	}
	if (!have)
		memset(&cell, 0, sizeof(cell)); /* blank */

	VTermColor fg = cell.fg, bg = cell.bg;
	/* bold maps palette 0-7 to 8-15 (xterm default boldColor) */
	if (cell.attrs.bold && VTERM_COLOR_IS_INDEXED(&fg) && fg.indexed.idx < 8)
		fg.indexed.idx += 8;
	vterm_state_convert_color_to_rgb(r->state, &fg);
	vterm_state_convert_color_to_rgb(r->state, &bg);

	/* cursor: reverse-video, blink phase from the host. Only in the live
	 * viewport (scrollback shows no cursor). Shape from DECSCUSR:
	 * 1 block, 2 underline, 3 bar-left. Blink: once an app issued
	 * DECSCUSR we honour its steady/blink bit; before that (plain shell
	 * prompt) the cursor blinks by default, as on VT100s. */
	bool cursor_here = r->scroll_offset == 0 && r->cursor_visible &&
		row == r->cursor.row && col == r->cursor.col;
	if (cursor_here) {
		if (r->cursor_shape == 0)
			r->cursor_shape = VTERM_PROP_CURSORSHAPE_BLOCK;
		bool blink = r->cursor_shape_set ? (r->cursor_blink != 0) : true;
		if (blink && !r->blink_on)
			cursor_here = false; /* blink dark phase */
	}

	uint32_t on = cell.attrs.reverse ? color_to_u32(&bg) : color_to_u32(&fg);
	uint32_t off = cell.attrs.reverse ? color_to_u32(&fg) : color_to_u32(&bg);

	/* mouse selection: streaming highlight — first/last rows are clipped
	 * by the anchor/cur columns, interior rows run to the last non-blank
	 * cell (trailing whitespace is not selected), like xterm. Block mode
	 * (Alt+drag) selects the plain rectangle. */
	bool in_sel = false;
	if (r->sel_active) {
		int r0 = r->sel_anchor.row < r->sel_cur.row ? r->sel_anchor.row : r->sel_cur.row;
		int r1 = r->sel_anchor.row > r->sel_cur.row ? r->sel_anchor.row : r->sel_cur.row;
		int c0 = r->sel_anchor.col < r->sel_cur.col ? r->sel_anchor.col : r->sel_cur.col;
		int c1 = r->sel_anchor.col > r->sel_cur.col ? r->sel_anchor.col : r->sel_cur.col;
		if (row >= r0 && row <= r1) {
			if (r->sel_block)
				in_sel = col >= c0 && col <= c1;
			else if (row == r0 && row == r1)
				in_sel = col >= c0 && col <= c1;
			else if (row == r0)
				in_sel = col >= c0 && (r->sel_line_end < 0 || col <= r->sel_line_end);
			else if (row == r1)
				in_sel = col <= c1;
			else
				in_sel = r->sel_line_end < 0 || col <= r->sel_line_end;
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
	 * per-pixel in the glyph loop below. CJK/placeholder paths get the
	 * block-inverted pair (block behaviour only). */
	bool cur_block = cursor_here && r->cursor_shape == VTERM_PROP_CURSORSHAPE_BLOCK;
	uint32_t on_b = cur_block ? off : on;
	uint32_t off_b = cur_block ? on : off;

	if (cell.width == 0)
		return; /* continuation cell */
	if (cell.chars[0] == (uint32_t)-1)
		return; /* gap behind a double-width char */

	uint32_t cp = cell.chars[0];
	if (cp == 0)
		cp = ' ';

	uint8_t idx = term_unicode_to_cp437(cp);
	const int px0 = col * TERM_CELL_W;
	const int py0 = row * TERM_CELL_H;

	/* Unicode whitespace: blank cell, honouring double-width (U+3000) */
	if (is_space_cp(cp)) {
		int w = (cell.width >= 2) ? 2 : 1;
		paint_blank(r, row, col, px0, py0, w, on_b, off_b, wide_occ[col]);
		/* underline/strike still apply to blank cells */
		if (cell.attrs.underline)
			for (int x = 0; x < w * TERM_CELL_W; x++)
				r->pixels[(py0 + TERM_CELL_H - 1) * r->win_w + (px0 + x)] = on_b;
		if (cell.attrs.strike)
			for (int x = 0; x < w * TERM_CELL_W; x++)
				r->pixels[(py0 + TERM_CELL_H / 2) * r->win_w + (px0 + x)] = on_b;
		return;
	}

	/* SGR 5 blink: hide the glyph on the dark phase of the blink clock */
	if (cell.attrs.blink && !r->blink_on) {
		for (int y = 0; y < TERM_CELL_H; y++)
			for (int x = 0; x < TERM_CELL_W; x++)
				r->pixels[(py0 + y) * r->win_w + (px0 + x)] = off;
		return;
	}
	/* SGR 8 conceal: hide the glyph entirely (password-style fields) */
	if (cell.attrs.conceal) {
		for (int y = 0; y < TERM_CELL_H; y++)
			for (int x = 0; x < TERM_CELL_W; x++)
				r->pixels[(py0 + y) * r->win_w + (px0 + x)] = off;
		return;
	}

	if (idx == 0xFF && cp != 0xFF) {
		/* not in CP437: try CJK (double-width) before falling back */
		if (cell.width >= 2 && paint_cjk(r, row, col, cp, on_b, off_b)) {
			/* decorations apply to CJK cells too (16 px wide) */
			if (cell.attrs.underline)
				for (int x = 0; x < 16; x++)
					r->pixels[(py0 + TERM_CELL_H - 1) * r->win_w + (px0 + x)] = on_b;
			if (cell.attrs.strike)
				for (int x = 0; x < 16; x++)
					r->pixels[(py0 + TERM_CELL_H / 2) * r->win_w + (px0 + x)] = on_b;
			return;
		}
		/* emoji: 16x16, rendered even when layout width is 1 (wide glyph) */
		if (paint_emoji(r, row, col, cp, on_b, off_b))
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
						bool set = (byte >> (7 - (gx & 7))) & 1;
						r->pixels[(py0 + gy) * r->win_w + (px0 + gx)] = set ? on_b : off_b;
					}
				}
			} else {
				for (int gy = 0; gy < TERM_CELL_H; gy++) {
					uint8_t line = sg[gy];
					for (int gx = 0; gx < TERM_CELL_W; gx++) {
						bool set = (line >> (7 - gx)) & 1;
						r->pixels[(py0 + gy) * r->win_w + (px0 + gx)] = set ? on_b : off_b;
					}
				}
			}
			return;
		}
		/* unmapped: draw a hollow box placeholder spanning the cell width */
		int pw = (cell.width >= 2) ? 2 : 1;
		for (int y = 0; y < TERM_CELL_H; y++) {
			for (int x = 0; x < pw * TERM_CELL_W; x++) {
				bool edge = (x == 0 || y == 0 || x == pw * TERM_CELL_W - 1 || y == TERM_CELL_H - 1);
				r->pixels[(py0 + y) * r->win_w + (px0 + x)] = edge ? on_b : off_b;
			}
		}
		return;
	}

	/* IBM VGA glyphs: MSB = leftmost pixel */
	const uint8_t *glyph = vga8x16[idx];
	for (int gy = 0; gy < TERM_CELL_H; gy++) {
		uint8_t line = glyph[gy];
		for (int gx = 0; gx < TERM_CELL_W; gx++) {
			bool set = (line >> (7 - gx)) & 1;
			/* cursor shape masks which pixels invert */
			bool cur = cursor_here;
			if (cur && r->cursor_shape == VTERM_PROP_CURSORSHAPE_UNDERLINE)
				cur = (gy >= TERM_CELL_H - 2);
			else if (cur && r->cursor_shape == VTERM_PROP_CURSORSHAPE_BAR_LEFT)
				cur = (gx < 2);
			r->pixels[(py0 + gy) * r->win_w + (px0 + gx)] = (set != cur) ? on : off;
		}
	}

	/* SGR 4 underline: bottom row */
	if (cell.attrs.underline) {
		for (int x = 0; x < TERM_CELL_W; x++)
			r->pixels[(py0 + TERM_CELL_H - 1) * r->win_w + (px0 + x)] = on;
	}
	/* SGR 9 strikethrough: middle row */
	if (cell.attrs.strike) {
		for (int x = 0; x < TERM_CELL_W; x++)
			r->pixels[(py0 + TERM_CELL_H / 2) * r->win_w + (px0 + x)] = on;
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
	int r0 = 0, r1 = -1, c0 = 0, c1 = 0;
	if (r->sel_active) {
		r0 = r->sel_anchor.row < r->sel_cur.row ? r->sel_anchor.row : r->sel_cur.row;
		r1 = r->sel_anchor.row > r->sel_cur.row ? r->sel_anchor.row : r->sel_cur.row;
		c0 = r->sel_anchor.col < r->sel_cur.col ? r->sel_anchor.col : r->sel_cur.col;
		c1 = r->sel_anchor.col > r->sel_cur.col ? r->sel_anchor.col : r->sel_cur.col;
	}
	for (int row = 0; row < r->rows; row++) {
		/* last non-blank col within the selected span, for streaming highlight */
		r->sel_line_end = -1;
		if (r->sel_active && row >= r0 && row <= r1) {
			int from = (row == r0) ? c0 : 0;
			int to = (row == r1) ? c1 : r->cols - 1;
			for (int col = from; col <= to; col++) {
				VTermScreenCell cell;
				VTermPos p = { .row = row, .col = col };
				if (vterm_screen_get_cell(r->screen, p, &cell) && cell.chars[0] != 0 &&
				    cell.chars[0] != (uint32_t)-1)
					r->sel_line_end = col;
			}
		}
		bool wide_occ[512];
		for (int col = 0; col < r->cols; col++)
			wide_occ[col] = false;
		/* pass 1: mark columns covered by a 1-cell-layout glyph that
		 * renders 16px wide (geometry/ballot boxes/emoji) */
		for (int col = 0; col < r->cols - 1; col++) {
			VTermScreenCell cell;
			VTermPos p = { .row = row, .col = col };
			if (!vterm_screen_get_cell(r->screen, p, &cell))
				continue;
			if (cell.width < 2 && cell.chars[0] && cell.chars[0] != (uint32_t)-1 &&
			    is_wide_glyph_cp(cell.chars[0]))
				wide_occ[col + 1] = true;
		}
		for (int col = 0; col < r->cols; col++)
			paint_cell(r, row, col, wide_occ);
	}
}

/* Read a cell through the renderer's source (live screen or scrollback). */
static bool get_cell_at(const term_renderer_t *r, int row, int col, VTermScreenCell *cell)
{
	if (r->scroll_offset > 0 && row < r->scroll_offset) {
		int sb_row = r->scroll_offset - 1 - row;
		return r->sb_get_cell && r->sb_get_cell(r->sb_user, sb_row, col, cell);
	}
	VTermPos p = { .row = row - r->scroll_offset, .col = col };
	return vterm_screen_get_cell(r->screen, p, cell);
}

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

	for (int row = r0; row <= r1; row++) {
		int last = c0 - 1;
		if (!r->sel_block) {
			for (int col = c0; col <= c1; col++) {
				VTermScreenCell cell;
				if (get_cell_at(r, row, col, &cell) && cell.chars[0] != 0 &&
				    cell.chars[0] != (uint32_t)-1)
					last = col;
			}
		} else {
			last = c1; /* block mode keeps the full column span */
		}
		if (last < c0)
			continue; /* empty line */
		for (int col = c0; col <= last; col++) {
			VTermScreenCell cell;
			if (!get_cell_at(r, row, col, &cell))
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
