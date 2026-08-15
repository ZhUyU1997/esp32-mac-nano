/* vterm-test — automated pixel-level regression tests for the terminal
 * rendering pipeline (libvterm + term_render + IBM VGA glyphs).
 *
 * Each case feeds a byte stream into a fresh libvterm instance, renders
 * one frame, then asserts on the pixel buffer. Covers the bugs found
 * during manual bring-up:
 *   1. mirrored glyphs (font bit order)           — 'b' must not look like 'd'
 *   2. CP437 box-drawing mapping                  — U+2500 -> glyph 0xC4
 *   3. alternate screen restore (htop ghosting)   — DECSET 1049 round-trip
 *   4. cursor block rendering                     — reverse video cell
 *   5. SGR colours                                — 16-colour palette
 *   6. scrolling                                  — content leaves the top
 *   7. grid size                                  — 80x30
 *
 * Run: xmake run vterm-test
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vterm.h"
#include "term_render.h"
#include "vga8x16.h"
#include "xterm_seqs.h"
#include "symbol_glyphs.h"
#include "emoji_glyphs.h"
#include "cp437.h"
#include "sauce.h"

/* UTF-8 encode one code point (host-side helper) */
static int utf8_encode_cp(uint32_t cp, char *out)
{
	if (cp < 0x80) { out[0] = (char)cp; return 1; }
	if (cp < 0x800) { out[0] = (char)(0xC0 | (cp >> 6)); out[1] = (char)(0x80 | (cp & 0x3F)); return 2; }
	if (cp < 0x10000) { out[0] = (char)(0xE0 | (cp >> 12)); out[1] = (char)(0x80 | ((cp >> 6) & 0x3F)); out[2] = (char)(0x80 | (cp & 0x3F)); return 3; }
	out[0] = (char)(0xF0 | (cp >> 18)); out[1] = (char)(0x80 | ((cp >> 12) & 0x3F)); out[2] = (char)(0x80 | ((cp >> 6) & 0x3F)); out[3] = (char)(0x80 | (cp & 0x3F)); return 4;
}

static int s_failures;
static int s_passes;

/* ---- helpers ---------------------------------------------------------- */

/* Reconstruct the glyph of a rendered cell from pixels, then find which
 * CP437 character it matches. Returns -1 on no match. */
static int cell_char(term_renderer_t *r, int row, int col)
{
	uint8_t lines[TERM_CELL_H];
	int x0 = col * TERM_CELL_W;
	int y0 = row * TERM_CELL_H;
	for (int gy = 0; gy < TERM_CELL_H; gy++) {
		uint8_t v = 0;
		for (int gx = 0; gx < TERM_CELL_W; gx++) {
			uint32_t p = r->pixels[(y0 + gy) * r->win_w + (x0 + gx)];
			uint8_t r8 = (uint8_t)(p >> 16);
			if (r8 > 128)
				v |= 1u << (7 - gx);
		}
		lines[gy] = v;
	}
	for (int i = 0; i < 256; i++) {
		if (memcmp(vga8x16[i], lines, TERM_CELL_H) == 0)
			return i;
	}
	return -1;
}

/* Pixel colour at the centre of a cell. */
static uint32_t cell_pixel(term_renderer_t *r, int row, int col)
{
	int x = col * TERM_CELL_W + TERM_CELL_W / 2;
	int y = row * TERM_CELL_H + TERM_CELL_H / 2;
	return r->pixels[y * r->win_w + x];
}

static int cell_bright(term_renderer_t *r, int row, int col)
{
	int x0 = col * TERM_CELL_W;
	int y0 = row * TERM_CELL_H;
	int n = 0;
	for (int y = 0; y < TERM_CELL_H; y++)
		for (int x = 0; x < TERM_CELL_W; x++)
			if ((r->pixels[(y0 + y) * r->win_w + (x0 + x)] >> 16) > 128)
				n++;
	return n;
}

/* ---- assertion macros -------------------------------------------------- */

static void report(const char *name, bool ok, const char *what, int got, int want)
{
	if (ok) {
		s_passes++;
		printf("PASS  %s\n", name);
	} else {
		s_failures++;
		printf("FAIL  %s: %s got=%d want=%d\n", name, what, got, want);
	}
}

#define CHECK_EQ(name, expr, want, what)             \
	do {                                             \
		int _got = (int)(expr);                      \
		report(name, _got == (int)(want), what, _got, (int)(want)); \
	} while (0)

/* ---- test harness ------------------------------------------------------ */

typedef struct {
	term_renderer_t r;
	VTerm *vt;
	uint32_t *px;
	/* scrollback storage: rows scrolled out of the live screen */
	VTermScreenCell sb[8][80];
	int sb_count;
	bool track_visible;
	bool sync_h, sync_l; /* DECSET 2026 seen */
	/* libvterm stores a pointer to this, so it must outlive the ctor */
	VTermScreenCallbacks cbs;
} tctx_t;

static int t_damage(VTermRect rect, void *user)
{
	(void)rect;
	(void)user;
	return 1;
}

static int t_settermprop(VTermProp prop, VTermValue *val, void *user)
{
	tctx_t *t = user;
	/* only track visibility when the specific test asks for it, otherwise
	 * the default-on cursor would invert cells in unrelated tests */
	if (prop == VTERM_PROP_CURSORVISIBLE && t->track_visible)
		t->r.cursor_visible = val->boolean;
	if (prop == VTERM_PROP_SYNCOUTPUT) {
		if (val->boolean)
			t->sync_h = true;
		else
			t->sync_l = true;
	}
	return 1;
}

static int t_sb_pushline(int cols, const VTermScreenCell *cells, void *user)
{
	tctx_t *t = user;
	if (t->sb_count < 8 && cols <= 80) {
		/* newest scrolled-out line goes to sb[0] (top of scrollback view) */
		for (int i = t->sb_count; i > 0; i--)
			memcpy(t->sb[i], t->sb[i - 1], (size_t)cols * sizeof(VTermScreenCell));
		memcpy(t->sb[0], cells, (size_t)cols * sizeof(VTermScreenCell));
		t->sb_count++;
	}
	return 1;
}

static int t_sb_popline(int cols, VTermScreenCell *cells, void *user)
{
	tctx_t *t = user;
	if (t->sb_count == 0)
		return 0;
	/* oldest stored row is sb[sb_count-1] (sb[0] = newest) */
	int n = cols < 80 ? cols : 80;
	memcpy(cells, t->sb[t->sb_count - 1], (size_t)n * sizeof(VTermScreenCell));
	t->sb_count--;
	return 1;
}

static int t_sb_get_cell(void *user, int row, int col, VTermScreenCell *cell)
{
	tctx_t *t = user;
	if (row < 0 || row >= t->sb_count)
		return 0;
	*cell = t->sb[row][col];
	return 1;
}

static void tctx_new(tctx_t *t, int rows, int cols)
{
	memset(t, 0, sizeof(*t));
	t->vt = vterm_new(rows, cols);
	vterm_set_utf8(t->vt, 1);
	VTermScreen *scr = vterm_obtain_screen(t->vt);
	/* mirror the host tool setup: altscreen buffer must be allocated */
	vterm_screen_enable_altscreen(scr, 1);
	vterm_screen_reset(scr, 1);
	t->cbs.damage = t_damage;
	t->cbs.sb_pushline = t_sb_pushline;
	t->cbs.sb_popline = t_sb_popline;
	t->cbs.settermprop = t_settermprop;
	vterm_screen_set_callbacks(scr, &t->cbs, t);
	t->px = calloc((size_t)rows * cols * TERM_CELL_W * TERM_CELL_H, sizeof(uint32_t));
	term_render_init(&t->r, t->vt, t->px);
	t->r.sb_get_cell = t_sb_get_cell;
	t->r.sb_user = t;
}

static void tctx_feed(tctx_t *t, const char *bytes)
{
	vterm_input_write(t->vt, bytes, strlen(bytes));
	vterm_screen_flush_damage(t->r.screen);
	term_render_frame(&t->r);
}

static void tctx_free(tctx_t *t)
{
	free(t->px);
	vterm_free(t->vt);
}

/* ---- tests ------------------------------------------------------------- */

static void test_glyph_orientation(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	tctx_feed(&t, "bEd");
	/* cursor sits after the text; move it home so it does not cover cell 0 */
	vterm_input_write(t.vt, "\033[H", 3);
	vterm_screen_flush_damage(t.r.screen);
	term_render_frame(&t.r);
	CHECK_EQ("glyph 'b' not mirrored (must not be 'd')", cell_char(&t.r, 0, 0), 'b', "cell(0,0) char");
	CHECK_EQ("glyph 'E' not mirrored (must not be U+018E)", cell_char(&t.r, 0, 1), 'E', "cell(0,1) char");
	CHECK_EQ("glyph 'd' renders as 'd'", cell_char(&t.r, 0, 2), 'd', "cell(0,2) char");
	tctx_free(&t);
}

static void test_cp437_box_mapping(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	/* U+2500 horizontal line, U+2502 vertical line, U+2510 top-right corner */
	tctx_feed(&t, "\xe2\x94\x80\xe2\x94\x82\xe2\x94\x90");
	vterm_input_write(t.vt, "\033[H", 3);
	vterm_screen_flush_damage(t.r.screen);
	term_render_frame(&t.r);
	CHECK_EQ("U+2500 maps to CP437 0xC4", cell_char(&t.r, 0, 0), 0xC4, "cell(0,0) char");
	CHECK_EQ("U+2502 maps to CP437 0xB3", cell_char(&t.r, 0, 1), 0xB3, "cell(0,1) char");
	CHECK_EQ("U+2510 maps to CP437 0xBF", cell_char(&t.r, 0, 2), 0xBF, "cell(0,2) char");

	/* cross-checked against the Unicode Consortium CP437 table: ┴/╦/╩ and
	 * the ▒/▓ shades were misassigned (swapped slots) in the map */
	tctx_feed(&t, "\xe2\x94\xb4\xe2\x95\xa6\xe2\x95\xa9\xe2\x96\x92\xe2\x96\x93"); /* ┴ ╦ ╩ ▒ ▓ */
	vterm_input_write(t.vt, "\033[1;1H", 6);
	vterm_screen_flush_damage(t.r.screen);
	term_render_frame(&t.r);
	CHECK_EQ("U+2534 ┴ maps to CP437 0xC1", cell_char(&t.r, 0, 0), 0xC1, "cell(0,0) char");
	CHECK_EQ("U+2566 ╦ maps to CP437 0xCB", cell_char(&t.r, 0, 1), 0xCB, "cell(0,1) char");
	CHECK_EQ("U+2569 ╩ maps to CP437 0xCA", cell_char(&t.r, 0, 2), 0xCA, "cell(0,2) char");
	CHECK_EQ("U+2592 ▒ maps to CP437 0xB1", cell_char(&t.r, 0, 3), 0xB1, "cell(0,3) char");
	CHECK_EQ("U+2593 ▓ maps to CP437 0xB2", cell_char(&t.r, 0, 4), 0xB2, "cell(0,4) char");

	/* the rest of CP437: Latin-1 accented + Greek + ß/ƒ/₧ (were hollow
	 * placeholders because the VGA font has them but the map did not) */
	tctx_feed(&t, "\xc3\xa9\xcf\x80\xce\xa9\xc3\x9f\xc6\x92");  /* é π Ω ß ƒ */
	vterm_input_write(t.vt, "\033[1;1H", 6);
	vterm_screen_flush_damage(t.r.screen);
	term_render_frame(&t.r);
	CHECK_EQ("U+00E9 maps to CP437 0x82", cell_char(&t.r, 0, 0), 0x82, "cell(0,0) char");
	CHECK_EQ("U+03C0 maps to CP437 0xE3", cell_char(&t.r, 0, 1), 0xE3, "cell(0,1) char");
	CHECK_EQ("U+03A9 maps to CP437 0xEA", cell_char(&t.r, 0, 2), 0xEA, "cell(0,2) char");
	CHECK_EQ("U+00DF maps to CP437 0xE1", cell_char(&t.r, 0, 3), 0xE1, "cell(0,3) char");
	CHECK_EQ("U+0192 maps to CP437 0x9F", cell_char(&t.r, 0, 4), 0x9F, "cell(0,4) char");
	/* U+00FF ÿ: was rendered as the CP437 0xFF glyph by the cp==0xFF
	 * special case; now maps to the real ÿ glyph at 0x98 */
	vterm_input_write(t.vt, "\033[1;6H", 6); /* move to (0,5) first */
	tctx_feed(&t, "\xc3\xbf");
	vterm_screen_flush_damage(t.r.screen);
	term_render_frame(&t.r);
	CHECK_EQ("U+00FF maps to CP437 0x98", cell_char(&t.r, 0, 5), 0x98, "cell(0,5) char");
	tctx_free(&t);
}

static void test_altscreen_restore(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	tctx_feed(&t, "MAIN-SCREEN");
	/* enter altscreen, write, leave: primary screen must be restored */
	tctx_feed(&t, "\033[?1049hALT-ONLY\033[?1049l");
	CHECK_EQ("altscreen restore: main content back", cell_char(&t.r, 0, 0), 'M', "cell(0,0) char");
	CHECK_EQ("altscreen restore: no ghost, col4 is '-'", cell_char(&t.r, 0, 4), '-', "cell(0,4) char");
	CHECK_EQ("altscreen restore: col5 is 'S'", cell_char(&t.r, 0, 5), 'S', "cell(0,5) char");
	tctx_free(&t);
}

static void test_cursor_block(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	tctx_feed(&t, "abc");
	/* cursor on a blank cell (col 3): reverse video -> cell fully bright */
	vterm_input_write(t.vt, "\033[1;4H", 5);
	vterm_screen_flush_damage(t.r.screen);
	t.r.cursor.row = 0;
	t.r.cursor.col = 3;
	t.r.cursor_visible = true;
	t.r.cursor_shape_set = true;
	t.r.cursor_blink = 1;
	t.r.blink_on = true;
	term_render_frame(&t.r);
	CHECK_EQ("cursor block: cell fully bright", cell_bright(&t.r, 0, 3), TERM_CELL_W * TERM_CELL_H, "bright px");
	/* blink off: cursor hidden (blinking mode) */
	t.r.blink_on = false;
	term_render_frame(&t.r);
	CHECK_EQ("cursor blink off: cell dark again", cell_bright(&t.r, 0, 3), 0, "bright px");
	/* steady mode (vim CSI 2 q): cursor stays visible in dark phase */
	t.r.cursor_blink = 0;
	term_render_frame(&t.r);
	CHECK_EQ("steady cursor: visible even when blink phase dark",
	         cell_bright(&t.r, 0, 3), TERM_CELL_W * TERM_CELL_H, "bright px");
	/* pre-DECSCUSR (plain shell): blinks by default */
	t.r.cursor_shape_set = false;
	t.r.blink_on = false;
	term_render_frame(&t.r);
	CHECK_EQ("default cursor: blinks before any DECSCUSR", cell_bright(&t.r, 0, 3), 0, "bright px");
	tctx_free(&t);
}

/* count bright pixels of a given colour family in a cell */
static int cell_color_count(term_renderer_t *r, int row, int col, int want)
{
	int x0 = col * TERM_CELL_W;
	int y0 = row * TERM_CELL_H;
	int n = 0;
	for (int y = 0; y < TERM_CELL_H; y++) {
		for (int x = 0; x < TERM_CELL_W; x++) {
			uint32_t p = r->pixels[(y0 + y) * r->win_w + (x0 + x)];
			uint8_t rr = (uint8_t)(p >> 16), gg = (uint8_t)(p >> 8), bb = (uint8_t)p;
			bool hit = false;
			switch (want) {
			case 0: hit = (rr > 150 && gg < 80 && bb < 80); break;   /* red */
			case 1: hit = (gg > 150 && rr < 80 && bb < 80); break;   /* green */
			case 2: hit = (rr > 200 && gg > 200 && bb > 200); break; /* white */
			}
			if (hit)
				n++;
		}
	}
	return n;
}

static void test_sgr_colours(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	tctx_feed(&t, "\033[31mR\033[32mG\033[0mW");
	CHECK_EQ("SGR 31: red glyph pixels", cell_color_count(&t.r, 0, 0, 0) > 4, 1, "red px");
	CHECK_EQ("SGR 32: green glyph pixels", cell_color_count(&t.r, 0, 1, 1) > 4, 1, "green px");
	CHECK_EQ("SGR reset: white glyph pixels", cell_color_count(&t.r, 0, 2, 2) > 4, 1, "white px");
	tctx_free(&t);
}

static void test_scroll(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	char buf[2048];
	size_t off = 0;
	for (int i = 1; i <= 35; i++)
		off += (size_t)snprintf(buf + off, sizeof(buf) - off, "LINE%02d\r\n", i);
	buf[off] = '\0';
	tctx_feed(&t, buf);
	/* 35 lines through a 30-row screen: lines 6..35 visible, 1..5 scrolled
	 * out. The trailing \n of LINE35 scrolls the last row empty, so the
	 * last content row is 28. */
	CHECK_EQ("scroll: row0 starts at LINE06", cell_char(&t.r, 0, 0), 'L', "cell(0,0)");
	CHECK_EQ("scroll: row28 is LINE35", cell_char(&t.r, 28, 0), 'L', "cell(28,0)");
	CHECK_EQ("scroll: row28 tag '35'", cell_char(&t.r, 28, 4), '3', "cell(28,4)");
	tctx_free(&t);
}

static void test_query_responses(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	char out[128];

	/* CPR: CSI 6 n -> ESC[<row>;<col>R */
	tctx_feed(&t, "\033[6n");
	size_t n = vterm_output_read(t.vt, out, sizeof(out) - 1);
	out[n] = '\0';
	CHECK_EQ("CPR answers ESC[1;1R", strcmp(out, "\033[1;1R") == 0, 1, "cpr reply");

	/* DA: CSI c -> ESC[?1;2c (VT100) */
	tctx_feed(&t, "\033[c");
	n = vterm_output_read(t.vt, out, sizeof(out) - 1);
	out[n] = '\0';
	CHECK_EQ("DA answers ESC[?1;2c", strcmp(out, "\033[?1;2c") == 0, 1, "da reply");

	/* DECSCUSR 3 q: underline cursor, must be accepted without error */
	tctx_feed(&t, "\033[3 q");
	tctx_feed(&t, "OK");
	CHECK_EQ("DECSCUSR accepted, text still renders", cell_char(&t.r, 0, 0), 'O', "cell(0,0)");
	tctx_free(&t);
}

static void test_grid_size(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	CHECK_EQ("grid: 80 cols -> win_w 640", t.r.win_w, 640, "win_w");
	CHECK_EQ("grid: 30 rows -> win_h 480", t.r.win_h, 480, "win_h");
	tctx_free(&t);
}

static void test_xterm_corpus(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);

	/* Feed every escape sequence exercised by the xterm.js test suite.
	 * Assert the parser survives the whole corpus and the terminal is
	 * still fully usable afterwards. */
	for (int i = 0; i < XTERM_SEQS_COUNT; i++) {
		const char *seq = k_xterm_seqs[i];
		vterm_input_write(t.vt, seq, strlen(seq));
		vterm_screen_flush_damage(t.r.screen);
		term_render_frame(&t.r);
	}

	/* clear + write: screen must still work after the entire corpus */
	vterm_input_write(t.vt, "\033[2J\033[HHELLO", 11);
	vterm_screen_flush_damage(t.r.screen);
	term_render_frame(&t.r);
	CHECK_EQ("corpus: terminal usable after 628 sequences",
	         cell_char(&t.r, 0, 0), 'H', "cell(0,0)");
	CHECK_EQ("corpus: second char intact",
	         cell_char(&t.r, 0, 1), 'E', "cell(0,1)");
	tctx_free(&t);
}

/* ---- behavioural tests ported from xterm.js InputHandler.test.ts ------ */

static void cur_pos(tctx_t *t, int *row, int *col)
{
	VTermPos p;
	vterm_state_get_cursorpos(t->r.state, &p);
	*row = p.row;
	*col = p.col;
}

static void test_cursor_moves(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	int r, c;

	tctx_feed(&t, "\033[10;20H");          /* CUP */
	cur_pos(&t, &r, &c);
	CHECK_EQ("CUP 10;20 -> (9,19)", r * 100 + c, 9 * 100 + 19, "row*100+col");

	tctx_feed(&t, "\033[3C");              /* CUF */
	cur_pos(&t, &r, &c);
	CHECK_EQ("CUF 3 -> col 22", c, 22, "col");

	tctx_feed(&t, "\033[2A");              /* CUU */
	cur_pos(&t, &r, &c);
	CHECK_EQ("CUU 2 -> row 7", r, 7, "row");

	tctx_feed(&t, "\033[5G");              /* CHA */
	cur_pos(&t, &r, &c);
	CHECK_EQ("CHA 5 -> col 4", c, 4, "col");

	tctx_feed(&t, "\033[12d");             /* VPA */
	cur_pos(&t, &r, &c);
	CHECK_EQ("VPA 12 -> row 11", r, 11, "row");
	tctx_free(&t);
}

/* ANSI.SYS save/restore cursor (ESC[s / ESC[u) — the positioning core of
 * classic ANSI art. libvterm used to misroute ESC[s to DECSLRM (which
 * resets the cursor to the home corner) and ignore ESC[u, garbling every
 * piece that relies on save/restore. */
static void test_ansi_save_restore_cursor(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	int r, c;

	tctx_feed(&t, "\033[10;20H");          /* CUP to (9,19) */
	tctx_feed(&t, "\033[s");               /* save cursor */
	tctx_feed(&t, "\033[5;5H");            /* move away */
	cur_pos(&t, &r, &c);
	CHECK_EQ("ESC[s: cursor still moves after save", r * 100 + c, 4 * 100 + 4, "pos");
	tctx_feed(&t, "\033[u");               /* restore cursor */
	cur_pos(&t, &r, &c);
	CHECK_EQ("ESC[u: cursor restored to saved pos", r * 100 + c, 9 * 100 + 19, "pos");

	/* the latest save wins (single saved slot, ANSI.SYS behaviour) */
	tctx_feed(&t, "\033[s");               /* save (9,19) */
	tctx_feed(&t, "\033[20;30H");          /* move */
	tctx_feed(&t, "\033[s");               /* save (19,29) */
	tctx_feed(&t, "\033[3;3H");            /* move */
	tctx_feed(&t, "\033[u");               /* restore -> (19,29) */
	cur_pos(&t, &r, &c);
	CHECK_EQ("ESC[u: latest save wins", r * 100 + c, 19 * 100 + 29, "pos");

	/* CSI ? s is XTSAVE (save private modes), NOT DECSLRM: it must not
	 * reset the cursor to the home corner */
	tctx_feed(&t, "\033[10;10H");
	tctx_feed(&t, "\033[?s");
	cur_pos(&t, &r, &c);
	CHECK_EQ("ESC[?s: not DECSLRM, cursor stays", r * 100 + c, 9 * 100 + 9, "pos");

	/* the parameterised CSI Pl;Pr s IS DECSLRM (margins reset cursor) */
	tctx_feed(&t, "\033[20;20H");
	tctx_feed(&t, "\033[1;40s");
	cur_pos(&t, &r, &c);
	CHECK_EQ("ESC[1;40s: DECSLRM resets cursor home", r * 100 + c, 0 * 100 + 0, "pos");
	tctx_free(&t);
}

static void test_erase(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	tctx_feed(&t, "ABCDEFGHIJ\033[1;5H");

	tctx_feed(&t, "\033[K");               /* EL0: cursor to end, inclusive */
	CHECK_EQ("EL0: col3 'D' kept", cell_char(&t.r, 0, 3), 'D', "cell(0,3)");
	CHECK_EQ("EL0: col4 (cursor) erased", cell_char(&t.r, 0, 4), 0, "cell(0,4)");

	tctx_feed(&t, "\033[1K");              /* EL1: start to cursor, inclusive */
	CHECK_EQ("EL1: col0 erased", cell_char(&t.r, 0, 0), 0, "cell(0,0)");
	CHECK_EQ("EL1: col3 erased too", cell_char(&t.r, 0, 3), 0, "cell(0,3)");

	tctx_feed(&t, "\033[2K");              /* EL2: whole line */
	CHECK_EQ("EL2: col4 erased", cell_char(&t.r, 0, 4), 0, "cell(0,4)");

	tctx_feed(&t, "SECOND-LINE\033[1;1H\033[2J"); /* ED2: whole screen */
	CHECK_EQ("ED2: screen cleared", cell_char(&t.r, 0, 0), 0, "cell(0,0)");
	tctx_free(&t);
}

static void test_insert_delete(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	tctx_feed(&t, "ABCDE\033[1;3H");       /* cursor at col 2 */

	tctx_feed(&t, "\033[2P");              /* DCH: delete 2 chars at cursor */
	CHECK_EQ("DCH: col1 'B' untouched", cell_char(&t.r, 0, 1), 'B', "cell(0,1)");
	CHECK_EQ("DCH: col2 'E' shifted left", cell_char(&t.r, 0, 2), 'E', "cell(0,2)");

	tctx_feed(&t, "\033[2@");              /* ICH: insert 2 blanks at cursor */
	CHECK_EQ("ICH: col1 'B' untouched", cell_char(&t.r, 0, 1), 'B', "cell(0,1)");
	CHECK_EQ("ICH: col2 blank", cell_char(&t.r, 0, 2), 0, "cell(0,2)");
	tctx_free(&t);
}

static void test_scroll_margins(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	/* DECSTBM 2..4, home, then insert line: only margin rows move */
	tctx_feed(&t, "\033[5;10r\033[2;2H");
	tctx_feed(&t, "LINE-A\r\nLINE-B\r\nLINE-C\r\n");
	tctx_feed(&t, "\033[1;1H\033[L");      /* IL at top (outside margin): no-op */
	int r, c;
	cur_pos(&t, &r, &c);
	CHECK_EQ("IL outside margin: cursor stays row0", r, 0, "row");
	tctx_free(&t);
}

static void test_sgr_attrs(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	tctx_feed(&t, "\033[1mB\033[4mU\033[7mR\033[0mN");
	VTermScreenCell cell;
	VTermPos p = { 0, 0 };
	vterm_screen_get_cell(t.r.screen, p, &cell);
	CHECK_EQ("SGR 1 bold", cell.attrs.bold, 1, "bold");
	p.col = 1;
	vterm_screen_get_cell(t.r.screen, p, &cell);
	CHECK_EQ("SGR 4 underline", cell.attrs.underline, 1, "underline");
	p.col = 2;
	vterm_screen_get_cell(t.r.screen, p, &cell);
	CHECK_EQ("SGR 7 reverse", cell.attrs.reverse, 1, "reverse");
	p.col = 3;
	vterm_screen_get_cell(t.r.screen, p, &cell);
	CHECK_EQ("SGR 0 resets attrs", cell.attrs.bold + cell.attrs.reverse, 0, "attrs");
	tctx_free(&t);
}

static void test_charset_line_drawing(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	/* DEC Special Graphics: ESC ( 0, then 'j' = U+2518 box corner */
	tctx_feed(&t, "\033(0j\033(B");
	CHECK_EQ("G0 line drawing: 'j' -> U+2518 -> CP437 0xD9",
	         cell_char(&t.r, 0, 0), 0xD9, "cell(0,0)");
	tctx_free(&t);
}

static void test_rep_and_wide(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	/* REP: CSI b repeats the preceding character */
	tctx_feed(&t, "A\033[3b");
	CHECK_EQ("REP 3: AAAA", cell_char(&t.r, 0, 0), 'A', "cell(0,0)");
	CHECK_EQ("REP 3: 4th A", cell_char(&t.r, 0, 3), 'A', "cell(0,3)");

	/* wide char: CJK occupies 2 cells */
	tctx_feed(&t, "\033[2;1H\xe4\xb8\xad");   /* U+4E2D on row 1 */
	VTermScreenCell cell;
	VTermPos p = { 1, 0 };
	vterm_screen_get_cell(t.r.screen, p, &cell);
	CHECK_EQ("wide char: cell0 width 2", cell.width, 2, "width");
	p.col = 1;
	vterm_screen_get_cell(t.r.screen, p, &cell);
	CHECK_EQ("wide char: gap cell marker", cell.chars[0], (uint32_t)-1, "chars[0]");
	tctx_free(&t);
}

static void test_combining_chars(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	/* 'e' + U+0301 combining acute accent */
	tctx_feed(&t, "e\xcc\x81");
	VTermScreenCell cell;
	VTermPos p = { 0, 0 };
	vterm_screen_get_cell(t.r.screen, p, &cell);
	CHECK_EQ("combining: base 'e'", cell.chars[0], 'e', "chars[0]");
	CHECK_EQ("combining: accent joined", cell.chars[1], 0x301, "chars[1]");
	tctx_free(&t);
}

static void test_reset(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	tctx_feed(&t, "\033[3;10r\033[4;4H\033[7mX");
	tctx_feed(&t, "\033c");              /* RIS: full reset */
	int r, c;
	cur_pos(&t, &r, &c);
	CHECK_EQ("RIS: cursor home", r + c, 0, "row+col");
	VTermScreenCell cell;
	VTermPos p = { 0, 0 };
	vterm_screen_get_cell(t.r.screen, p, &cell);
	CHECK_EQ("RIS: attrs cleared", cell.attrs.reverse, 0, "reverse");
	tctx_free(&t);
}

/* char of a stored scrollback cell (no pixel round-trip) */
static int cell_char_from_cells(const VTermScreenCell *cells, int col)
{
	uint32_t cp = cells[col].chars[0];
	if (cp == 0 || cp == (uint32_t)-1)
		return 0;
	return (int)term_unicode_to_cp437(cp);
}

static void test_scrollback(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	char buf[2048];
	size_t off = 0;
	for (int i = 1; i <= 35; i++)
		off += (size_t)snprintf(buf + off, sizeof(buf) - off, "LINE%02d\r\n", i);
	buf[off] = '\0';
	tctx_feed(&t, buf);

	/* 35 lines through 30 rows: trailing \n of LINE35 scrolls once more,
	 * so 6 lines are captured (LINE01..06). */
	CHECK_EQ("scrollback: 6 lines captured", t.sb_count, 6, "sb_count");
	/* sb[0] = most recent scrolled-out line = LINE06 */
	CHECK_EQ("scrollback: sb[0] is LINE06", cell_char_from_cells(t.sb[0], 5), '6', "sb[0] col5");

	/* scroll back 2 lines: row0 shows the 2nd-most-recent line (LINE05) */
	t.r.scroll_offset = 2;
	term_render_frame(&t.r);
	CHECK_EQ("scrollback view: row0 is LINE05", cell_char(&t.r, 0, 0), 'L', "row0 col0");
	CHECK_EQ("scrollback view: row0 tag '05'", cell_char(&t.r, 0, 5), '5', "row0 col5");
	/* live viewport shifted: row2 of frame = screen row0 = LINE07 */
	CHECK_EQ("scrollback view: row2 is LINE07", cell_char(&t.r, 2, 5), '7', "row2 col5");

	/* back to live: normal view, cursor visible */
	t.r.scroll_offset = 0;
	term_render_frame(&t.r);
	CHECK_EQ("live view: row0 is LINE06", cell_char(&t.r, 0, 0), 'L', "row0 col0");
	tctx_free(&t);
}

/* ---- full-coverage behavioural tests (xterm.js InputHandler classes) --- */

static void test_column_ops(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	/* DECIC: CSI Ps ' } — insert columns at cursor */
	tctx_feed(&t, "ABCDE\033[1;3H\033[2'}");
	CHECK_EQ("DECIC: col1 'B' kept", cell_char(&t.r, 0, 1), 'B', "cell(0,1)");
	CHECK_EQ("DECIC: col2 blank", cell_char(&t.r, 0, 2), 0, "cell(0,2)");
	CHECK_EQ("DECIC: col4 'C' shifted", cell_char(&t.r, 0, 4), 'C', "cell(0,4)");

	/* DECDC: CSI Ps ' ~ — delete columns at cursor */
	tctx_feed(&t, "\033[2'~");
	CHECK_EQ("DECDC: col2 'C' shifted left", cell_char(&t.r, 0, 2), 'C', "cell(0,2)");

	/* SL (CSI SP @) is an xterm extension libvterm does not implement:
	 * it must be ignored without breaking the screen. */
	tctx_feed(&t, "\033[2;1HXYZ\033[2 @");
	CHECK_EQ("SL (unsupported): ignored, text intact", cell_char(&t.r, 1, 0), 'X', "cell(1,0)");
	tctx_free(&t);
}

static void test_line_ops(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	/* SU: CSI Ps S — scroll up */
	tctx_feed(&t, "A1\r\nA2\r\nA3\r\n\033[1;1H\033[1S");
	CHECK_EQ("SU: A2 moved to row0", cell_char(&t.r, 0, 0), 'A', "cell(0,0)");
	CHECK_EQ("SU: row1 now A3", cell_char(&t.r, 1, 0), 'A', "cell(1,0)");

	/* SD: CSI Ps T — scroll down */
	tctx_feed(&t, "\033[1T");
	CHECK_EQ("SD: top row blank", cell_char(&t.r, 0, 0), 0, "cell(0,0)");

	/* IL within margins: CSI Ps L */
	tctx_feed(&t, "\033[3;5r\033[3;1H\033[1L");
	CHECK_EQ("IL in margins: row3 (margin top) blank", cell_char(&t.r, 2, 0), 0, "cell(2,0)");
	tctx_free(&t);
}

static void test_modes(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	int r, c;

	/* DECOM origin mode: CUP relative to margins */
	tctx_feed(&t, "\033[5;10r\033[?6h\033[2;2H");
	cur_pos(&t, &r, &c);
	CHECK_EQ("DECOM: CUP 2;2 -> row 5 (margin top)", r, 5, "row");

	/* IRM insert mode: typing inserts instead of overwrites */
	tctx_feed(&t, "\033[?6l\033[1;1HAB\033[1;2H\033[4hC");
	CHECK_EQ("IRM: C inserted before B", cell_char(&t.r, 0, 1), 'C', "cell(0,1)");
	CHECK_EQ("IRM: B shifted right", cell_char(&t.r, 0, 2), 'B', "cell(0,2)");

	/* LNM newline mode: LF moves to col 0 too */
	tctx_feed(&t, "\033[?6l\033[4h\033[20h\033[2;5HX\n");
	cur_pos(&t, &r, &c);
	CHECK_EQ("LNM: after LF row+1 col0", r * 100 + c, 2 * 100 + 0, "pos");
	tctx_free(&t);
}

static void test_wrap_boundary(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	/* cursor never advances beyond last column: typing at col 79 wraps */
	char buf[128];
	memset(buf, 'a', 80);
	buf[80] = '\0';
	tctx_feed(&t, buf);
	tctx_feed(&t, "b");
	CHECK_EQ("wrap: col79 still 'a'", cell_char(&t.r, 0, 79), 'a', "cell(0,79)");
	CHECK_EQ("wrap: next row starts 'b'", cell_char(&t.r, 1, 0), 'b', "cell(1,0)");

	/* DECAWM off: no wrap, cursor stays at col 79 */
	tctx_feed(&t, "\033[?7l\033[3;79H\033[79Cx");
	int r, c;
	cur_pos(&t, &r, &c);
	CHECK_EQ("DECAWM off: cursor clamps at col79", c, 79, "col");
	tctx_free(&t);
}

static void test_soft_hyphen(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	/* U+00AD soft hyphen: libvterm substitutes U+FFFD (renders as the
	 * placeholder glyph); it occupies a cell. */
	tctx_feed(&t, "AB\xc2\xad" "C");
	CHECK_EQ("soft hyphen: cell0 'A'", cell_char(&t.r, 0, 0), 'A', "cell(0,0)");
	CHECK_EQ("soft hyphen: cell1 'B'", cell_char(&t.r, 0, 1), 'B', "cell(0,1)");
	CHECK_EQ("soft hyphen: U+00AD -> placeholder", cell_char(&t.r, 0, 2), -1, "cell(0,2)");
	CHECK_EQ("soft hyphen: C at col3", cell_char(&t.r, 0, 3), 'C', "cell(0,3)");
	tctx_free(&t);
}

static void test_charset_g1(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	/* ESC ) 0 sets G1 to line drawing, SO (0x0E) switches to G1 */
	tctx_feed(&t, "\033)0\x0ej\x0f");
	CHECK_EQ("G1 line drawing: 'j' -> box corner 0xD9",
	         cell_char(&t.r, 0, 0), 0xD9, "cell(0,0)");
	tctx_free(&t);
}

static void test_save_restore_cursor(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	/* ESC 7 / ESC 8 (DECSC/DECRC) */
	tctx_feed(&t, "\033[5;10H\0337\033[20;70H\0338");
	int r, c;
	cur_pos(&t, &r, &c);
	CHECK_EQ("DECSC/DECRC: cursor restored to (4,9)", r * 100 + c, 4 * 100 + 9, "pos");
	tctx_free(&t);
}

static void test_sgr_256_rgb(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	/* 256-colour and 24-bit colour must reach the pixels */
	tctx_feed(&t, "\033[38;5;196mX\033[38;2;10;200;30mY");
	int red = cell_color_count(&t.r, 0, 0, 0);
	int green = 0;
	{
		int x0 = 1 * TERM_CELL_W;
		int y0 = 0;
		for (int y = 0; y < TERM_CELL_H; y++)
			for (int x = 0; x < TERM_CELL_W; x++) {
				uint32_t p = t.r.pixels[(y0 + y) * t.r.win_w + (x0 + x)];
				uint8_t gg = (uint8_t)(p >> 8), rr = (uint8_t)(p >> 16), bb = (uint8_t)p;
				if (gg > 150 && rr < 80 && bb < 80)
					green++;
			}
	}
	CHECK_EQ("SGR 38;5;196 -> red pixels", red > 4, 1, "red px");
	CHECK_EQ("SGR 38;2;10;200;30 -> green pixels", green > 4, 1, "green px");
	tctx_free(&t);
}

static void test_erase_scrollback_cmd(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	/* ED3 (CSI 3 J) erases scrollback: libvterm must not crash and the
	 * screen must stay usable (xterm extension; accept no-op) */
	tctx_feed(&t, "\033[3JHELLO");
	CHECK_EQ("ED3 accepted, screen usable", cell_char(&t.r, 0, 0), 'H', "cell(0,0)");
	tctx_free(&t);
}

static void test_chinese(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	/* UTF-8 中文 '你好' -> 16x16 GB2312 glyphs, each spanning 2 cells */
	tctx_feed(&t, "\xe4\xbd\xa0\xe5\xa5\xbd");   /* U+4F60 U+597D */
	int bright0 = cell_bright(&t.r, 0, 0);
	int bright1 = cell_bright(&t.r, 0, 2);
	CHECK_EQ("CJK: '你' renders (not placeholder)", bright0 > 30, 1, "bright px cell0");
	CHECK_EQ("CJK: '好' renders", bright1 > 30, 1, "bright px cell2");
	/* right half of the glyph lives in the continuation cell (col1/col3) */
	CHECK_EQ("CJK: '你' right half in col1", cell_bright(&t.r, 0, 1) > 10, 1, "bright px cell1");
	CHECK_EQ("CJK: col4 empty after 你好", cell_bright(&t.r, 0, 4), 0, "bright px cell4");
	/* '中' U+4E2D */
	tctx_feed(&t, "\033[2;1H\xe4\xb8\xad");
	CHECK_EQ("CJK: '中' renders", cell_bright(&t.r, 1, 0) > 30, 1, "bright px");
	tctx_free(&t);
}

static void test_cursor_shape(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	tctx_feed(&t, "abc");
	vterm_input_write(t.vt, "\033[1;2H", 5);
	vterm_screen_flush_damage(t.r.screen);
	t.r.cursor.row = 0;
	t.r.cursor.col = 1;
	t.r.cursor_visible = true;
	t.r.blink_on = true;
	t.r.cursor_shape_set = true;
	t.r.cursor_blink = 1;

	/* underline shape (DECSCUSR 4): only the bottom rows invert */
	t.r.cursor_shape = VTERM_PROP_CURSORSHAPE_UNDERLINE;
	term_render_frame(&t.r);
	int x0 = 1 * TERM_CELL_W, y0 = 0;
	int bottom = 0, top = 0;
	for (int y = 0; y < TERM_CELL_H; y++)
		for (int x = 0; x < TERM_CELL_W; x++) {
			uint32_t p = t.r.pixels[(y0 + y) * t.r.win_w + (x0 + x)];
			if ((p >> 16) > 128) {
				if (y >= TERM_CELL_H - 2)
					bottom++;
				else
					top++;
			}
		}
	CHECK_EQ("underline cursor: bright pixels at bottom", bottom > 10, 1, "bottom px");
	/* top rows keep only the glyph pixels (36 for 'b'), no full inversion */
	CHECK_EQ("underline cursor: top is glyph-only, not inverted", top < 60, 1, "top px");

	/* bar shape (DECSCUSR 6): left columns invert */
	t.r.cursor_shape = VTERM_PROP_CURSORSHAPE_BAR_LEFT;
	term_render_frame(&t.r);
	int left = 0, right = 0;
	for (int y = 0; y < TERM_CELL_H; y++)
		for (int x = 0; x < TERM_CELL_W; x++) {
			uint32_t p = t.r.pixels[(y0 + y) * t.r.win_w + (x0 + x)];
			if ((p >> 16) > 128) {
				if (x < 2)
					left++;
				else
					right++;
			}
		}
	CHECK_EQ("bar cursor: bright pixels on left", left > 15, 1, "left px");
	CHECK_EQ("bar cursor: right is glyph-only, not inverted", right < 60, 1, "right px");
	tctx_free(&t);
}

static void test_keyboard(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	char out[128];
	size_t n;

	/* arrow keys: normal mode ESC[A, DECCKM (?1 h) mode ESC OA */
	vterm_keyboard_key(t.vt, VTERM_KEY_UP, VTERM_MOD_NONE);
	n = vterm_output_read(t.vt, out, sizeof(out) - 1);
	out[n] = '\0';
	CHECK_EQ("keyboard UP -> ESC[A", strcmp(out, "\033[A") == 0, 1, "bytes");

	tctx_feed(&t, "\033[?1h");
	vterm_keyboard_key(t.vt, VTERM_KEY_UP, VTERM_MOD_NONE);
	n = vterm_output_read(t.vt, out, sizeof(out) - 1);
	out[n] = '\0';
	CHECK_EQ("keyboard UP + DECCKM -> ESC OA", strcmp(out, "\033OA") == 0, 1, "bytes");
	tctx_feed(&t, "\033[?1l");

	/* Ctrl+A -> 0x01 */
	vterm_keyboard_unichar(t.vt, 'a', VTERM_MOD_CTRL);
	n = vterm_output_read(t.vt, out, sizeof(out) - 1);
	CHECK_EQ("keyboard Ctrl+A -> 0x01", n == 1 && (unsigned char)out[0] == 0x01, 1, "bytes");

	/* Alt+letter -> ESC prefix */
	vterm_keyboard_unichar(t.vt, 'x', VTERM_MOD_ALT);
	n = vterm_output_read(t.vt, out, sizeof(out) - 1);
	out[n] = '\0';
	CHECK_EQ("keyboard Alt+x -> ESC x", strcmp(out, "\033x") == 0, 1, "bytes");

	/* Enter / F1 / Backspace */
	vterm_keyboard_key(t.vt, VTERM_KEY_ENTER, VTERM_MOD_NONE);
	n = vterm_output_read(t.vt, out, sizeof(out) - 1);
	out[n] = '\0';
	CHECK_EQ("keyboard ENTER -> CR", strcmp(out, "\r") == 0, 1, "bytes");

	vterm_keyboard_key(t.vt, VTERM_KEY_FUNCTION(1), VTERM_MOD_NONE);
	n = vterm_output_read(t.vt, out, sizeof(out) - 1);
	out[n] = '\0';
	CHECK_EQ("keyboard F1 -> ESC OP", strcmp(out, "\033OP") == 0, 1, "bytes");

	vterm_keyboard_key(t.vt, VTERM_KEY_BACKSPACE, VTERM_MOD_NONE);
	n = vterm_output_read(t.vt, out, sizeof(out) - 1);
	out[n] = '\0';
	CHECK_EQ("keyboard BACKSPACE -> DEL 0x7f", strcmp(out, "\x7f") == 0, 1, "bytes");
	tctx_free(&t);
}

static void test_cursor_visible_prop(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	/* steady block cursor on a blank cell at (0,0) */
	t.track_visible = true;
	t.r.cursor.row = 0;
	t.r.cursor.col = 0;
	t.r.cursor_visible = true;
	t.r.cursor_shape_set = true;
	t.r.cursor_blink = 0;
	t.r.blink_on = true;
	term_render_frame(&t.r);
	CHECK_EQ("cursor visible by default", cell_bright(&t.r, 0, 0), 128, "bright px");

	/* vim hides the cursor while redrawing (DECSET 25 l) */
	tctx_feed(&t, "\033[?25l");
	term_render_frame(&t.r);
	CHECK_EQ("?25l hides cursor", cell_bright(&t.r, 0, 0), 0, "bright px");

	/* ...then shows it again (DECSET 25 h): the host redraws */
	tctx_feed(&t, "\033[?25h");
	term_render_frame(&t.r);
	CHECK_EQ("?25h shows cursor again", cell_bright(&t.r, 0, 0), 128, "bright px");
	tctx_free(&t);
}

static void test_sgr_decorations(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);

	/* SGR 4 underline on a blank cell: bottom row bright, middle dark */
	tctx_feed(&t, "\033[4mU \033[0m");
	uint32_t bot = t.r.pixels[(0 * TERM_CELL_H + TERM_CELL_H - 1) * t.r.win_w + 8];
	uint32_t mid = t.r.pixels[(0 * TERM_CELL_H + TERM_CELL_H / 2) * t.r.win_w + 8];
	CHECK_EQ("SGR 4: underline row drawn", (bot >> 16) > 200, 1, "bottom bright");
	CHECK_EQ("SGR 4: no underline in middle", (mid >> 16) < 100, 1, "middle dark");

	/* SGR 9 strikethrough: middle row */
	tctx_feed(&t, "\033[2;1H\033[9mS");
	mid = t.r.pixels[(1 * TERM_CELL_H + TERM_CELL_H / 2) * t.r.win_w + 0];
	CHECK_EQ("SGR 9: strike row drawn", (mid >> 16) > 200, 1, "middle bright");

	/* SGR 5 blink: dark phase hides the glyph */
	tctx_feed(&t, "\033[3;1H\033[5mB");
	t.r.blink_on = true;
	term_render_frame(&t.r);
	CHECK_EQ("SGR 5: visible on bright phase", cell_bright(&t.r, 2, 0) > 10, 1, "glyph px");
	t.r.blink_on = false;
	term_render_frame(&t.r);
	CHECK_EQ("SGR 5: hidden on dark phase", cell_bright(&t.r, 2, 0), 0, "glyph px");
	tctx_free(&t);
}

static void test_reverse_video_mode(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	tctx_feed(&t, "TEXT");
	/* DECSCNM (?5h) flips the whole screen: background becomes bright */
	uint32_t before = t.r.pixels[0 * t.r.win_w + 0];
	tctx_feed(&t, "\033[?5h");
	uint32_t after = t.r.pixels[0 * t.r.win_w + 0];
	CHECK_EQ("DECSCNM: bg flips bright", (before >> 16) < 100 && (after >> 16) > 200, 1, "brightness");
	tctx_feed(&t, "\033[?5l");
	tctx_free(&t);
}

static void test_mouse_and_focus(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	char out[128];
	size_t n;

	/* mouse mode 1000: vterm_mouse_button generates an SGR or classic sequence */
	tctx_feed(&t, "\033[?1000h");
	vterm_mouse_button(t.vt, 1, true, VTERM_MOD_NONE);
	n = vterm_output_read(t.vt, out, sizeof(out) - 1);
	CHECK_EQ("mouse 1000: press emits ESC[M...", n >= 6 && out[0] == 0x1b && out[2] == 'M', 1, "bytes");

	/* wheel = button 4: X10 encoding, press then release */
	vterm_mouse_button(t.vt, 4, true, VTERM_MOD_NONE);
	n = vterm_output_read(t.vt, out, sizeof(out) - 1);
	CHECK_EQ("mouse 1000: wheel press b=0x60",
	         n == 6 && (unsigned char)out[3] == 0x60 && (unsigned char)out[4] == 0x21,
	         1, "wheel press");
	vterm_mouse_button(t.vt, 4, false, VTERM_MOD_NONE);
	n = vterm_output_read(t.vt, out, sizeof(out) - 1);
	CHECK_EQ("mouse 1000: wheel release b=0x23",
	         n == 6 && (unsigned char)out[3] == 0x23, 1, "wheel release");

	/* focus report 1004: vterm_state_focus_in emits ESC[I */
	tctx_feed(&t, "\033[?1004h");
	vterm_state_focus_in(t.r.state);
	n = vterm_output_read(t.vt, out, sizeof(out) - 1);
	out[n] = '\0';
	CHECK_EQ("focus 1004: focus_in emits ESC[I", strcmp(out, "\033[I") == 0, 1, "bytes");
	tctx_free(&t);
}

static void test_conceal(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	/* SGR 8 conceal: glyph hidden, cell stays background */
	tctx_feed(&t, "\033[8mHI\033[0mX");
	CHECK_EQ("conceal: cell0 no glyph", cell_bright(&t.r, 0, 0), 0, "bright px");
	/* cell after reset is normal */
	CHECK_EQ("conceal: reset restores glyph", cell_bright(&t.r, 0, 2) > 10, 1, "bright px");
	tctx_free(&t);
}

/* OSC 52 selection sink used by test_osc52 */
static char s_sel_got[64];
static size_t s_sel_got_len;
static bool s_sel_fired;
static VTermSelectionMask s_sel_mask;
static bool s_sel_query_fired;

static int sel_test_set(VTermSelectionMask mask, VTermStringFragment frag, void *user)
{
	(void)user;
	s_sel_mask = mask;
	if (frag.initial)
		s_sel_got_len = 0;
	if (frag.str && s_sel_got_len + frag.len < sizeof(s_sel_got)) {
		memcpy(s_sel_got + s_sel_got_len, frag.str, frag.len);
		s_sel_got_len += frag.len;
	}
	if (frag.final) {
		s_sel_got[s_sel_got_len] = '\0';
		s_sel_fired = true;
	}
	return 1;
}

static int sel_test_query(VTermSelectionMask mask, void *user)
{
	(void)user;
	s_sel_query_fired = true;
	s_sel_mask = mask;
	return 1;
}

static void test_osc52(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	VTermSelectionCallbacks scb = { .set = sel_test_set, .query = sel_test_query };
	char buf[256];
	vterm_state_set_selection_callbacks(t.r.state, &scb, NULL, buf, sizeof(buf));
	char out[128];
	size_t n;

	/* 1. set: libvterm decodes base64 before the callback */
	s_sel_fired = false;
	tctx_feed(&t, "\033]52;c;aGVsbG8=\a");
	CHECK_EQ("OSC52 set: callback fired", s_sel_fired, 1, "fired");
	CHECK_EQ("OSC52 set: decoded payload is hello", strcmp(s_sel_got, "hello") == 0, 1, "payload");
	CHECK_EQ("OSC52 set: mask is clipboard", s_sel_mask == VTERM_SELECTION_CLIPBOARD, 1, "mask");

	/* 2. set split across two writes: payload accumulates */
	s_sel_fired = false;
	tctx_feed(&t, "\033]52;c;aGVs");
	tctx_feed(&t, "bG8=\a");
	CHECK_EQ("OSC52 split: accumulated to hello", s_sel_fired && strcmp(s_sel_got, "hello") == 0, 1, "payload");

	/* 3. other masks: 52;p; selects PRIMARY (single-char mask is the
	 * standard; libvterm stops mask parsing at the first ';') */
	s_sel_fired = false;
	tctx_feed(&t, "\033]52;p;aGVsbG8=\a");
	CHECK_EQ("OSC52 mask p: primary",
	         s_sel_fired && (s_sel_mask & VTERM_SELECTION_PRIMARY) &&
	             !(s_sel_mask & VTERM_SELECTION_CLIPBOARD),
	         1, "mask");
	CHECK_EQ("OSC52 mask p: payload intact", strcmp(s_sel_got, "hello") == 0, 1, "payload");

	/* 4. invalid base64: callback fired with NULL payload */
	s_sel_fired = false;
	tctx_feed(&t, "\033]52;c;!!!\a");
	CHECK_EQ("OSC52 invalid b64: fired with empty", s_sel_fired && s_sel_got_len == 0, 1, "empty");

	/* 5. query: ESC]52;c;? triggers the query callback; the host answers
	 * with vterm_state_send_selection, which re-encodes base64 */
	s_sel_query_fired = false;
	tctx_feed(&t, "\033]52;c;?\a");
	CHECK_EQ("OSC52 query: callback fired", s_sel_query_fired, 1, "fired");
	CHECK_EQ("OSC52 query: mask is clipboard", s_sel_mask == VTERM_SELECTION_CLIPBOARD, 1, "mask");

	VTermStringFragment frag = {
		.str = "hello",
		.len = 5,
		.initial = true,
		.final = true,
	};
	vterm_state_send_selection(t.r.state, VTERM_SELECTION_CLIPBOARD, frag);
	n = vterm_output_read(t.vt, out, sizeof(out) - 1);
	out[n] = '\0';
	CHECK_EQ("OSC52 send_selection: OSC52;c;base64",
	         strstr(out, "\033]52;c;aGVsbG8=") != NULL, 1, "output");
	tctx_free(&t);
}

static void test_bracketed_paste(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	char out[128];
	size_t n;

	/* DECSET 2004 on: paste is wrapped in ESC[200~..ESC[201~ */
	tctx_feed(&t, "\033[?2004h");
	vterm_keyboard_start_paste(t.vt);
	vterm_keyboard_unichar(t.vt, 'A', VTERM_MOD_NONE);
	vterm_keyboard_unichar(t.vt, 'B', VTERM_MOD_NONE);
	vterm_keyboard_end_paste(t.vt);
	n = vterm_output_read(t.vt, out, sizeof(out) - 1);
	out[n] = '\0';
	CHECK_EQ("bracketed paste: wrapped 200~/201~",
	         strstr(out, "\033[200~AB\033[201~") != NULL, 1, "bytes");

	/* default (no 2004): paste goes through unwrapped */
	tctx_feed(&t, "\033[?2004l");
	vterm_keyboard_start_paste(t.vt);
	vterm_keyboard_unichar(t.vt, 'C', VTERM_MOD_NONE);
	vterm_keyboard_end_paste(t.vt);
	n = vterm_output_read(t.vt, out, sizeof(out) - 1);
	out[n] = '\0';
	CHECK_EQ("plain paste: unwrapped", strcmp(out, "C") == 0, 1, "bytes");
	tctx_free(&t);
}

static void test_selection(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	/* two lines of text, then select (0,0)-(1,3) */
	tctx_feed(&t, "ABCD\r\nWXYZ\r\n");
	t.r.sel_active = true;
	t.r.sel_anchor.row = 0; t.r.sel_anchor.col = 0;
	t.r.sel_cur.row = 1; t.r.sel_cur.col = 3;
	term_render_frame(&t.r);

	/* xterm-style selection = reverse video: the background of a selected
	 * cell becomes bright (fg), glyph pixels go dark */
	uint32_t sel_bg = t.r.pixels[(0 * TERM_CELL_H + TERM_CELL_H - 1) * t.r.win_w + 0];
	uint32_t out_bg = t.r.pixels[(0 * TERM_CELL_H + TERM_CELL_H - 1) * t.r.win_w + 5 * TERM_CELL_W];
	CHECK_EQ("selection: cell in range is reverse-video", (sel_bg >> 16) > 200, 1, "bright bg");
	CHECK_EQ("selection: outside range stays black", (out_bg >> 16) < 10, 1, "black");

	/* streaming: trailing whitespace of row0 (col4..) is NOT highlighted,
	 * only the glyph span A..D (col0..3) */
	uint32_t trail = t.r.pixels[(0 * TERM_CELL_H + TERM_CELL_H - 1) * t.r.win_w + 6 * TERM_CELL_W];
	CHECK_EQ("selection: trailing blanks of first row not selected",
	         (trail >> 16) < 10, 1, "black");
	uint32_t d_cell = t.r.pixels[(0 * TERM_CELL_H + TERM_CELL_H - 1) * t.r.win_w + 3 * TERM_CELL_W];
	CHECK_EQ("selection: last glyph of first row selected", (d_cell >> 16) > 200, 1, "bright bg");

	/* text extraction */
	char txt[256];
	size_t n = term_render_selected_text(&t.r, txt, sizeof(txt));
	CHECK_EQ("selection text: ABCD\nWXYZ", strcmp(txt, "ABCD\nWXYZ") == 0, 1, "text");
	(void)n;

	/* selection covering a CJK wide char */
	tctx_feed(&t, "\033[3;1H\xe4\xb8\xad");   /* 中 */
	t.r.sel_anchor.row = 2; t.r.sel_anchor.col = 0;
	t.r.sel_cur.row = 2; t.r.sel_cur.col = 1;
	n = term_render_selected_text(&t.r, txt, sizeof(txt));
	CHECK_EQ("selection text: CJK", strcmp(txt, "\xe4\xb8\xad") == 0, 1, "cjk text");
	tctx_free(&t);
}

static void test_bold_bright(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	/* xterm default boldColor: SGR 1 maps palette 0-7 to 8-15 */
	tctx_feed(&t, "\033[31mD\033[1;31mB");
	int dim_max = 0, bold_max = 0;
	for (int y = 0; y < TERM_CELL_H; y++)
		for (int x = 0; x < TERM_CELL_W; x++) {
			uint8_t r1 = (uint8_t)(t.r.pixels[y * t.r.win_w + x] >> 16);
			uint8_t r2 = (uint8_t)(t.r.pixels[y * t.r.win_w + TERM_CELL_W + x] >> 16);
			if (r1 > dim_max) dim_max = r1;
			if (r2 > bold_max) bold_max = r2;
		}
	CHECK_EQ("bold: normal red is dim (palette r=224)", dim_max == 224, 1, "dim red");
	CHECK_EQ("bold: bold red is brighter (palette 9)", bold_max > dim_max, 1, "bright red");
	tctx_free(&t);
}

static void test_select_word_line(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	tctx_feed(&t, "hello world foo");
	/* double-click on 'w' of world: selects the word */
	term_render_select_word(&t.r, 0, 7);
	char txt[256];
	term_render_selected_text(&t.r, txt, sizeof(txt));
	CHECK_EQ("word select: 'world'", strcmp(txt, "world") == 0, 1, "word");

	/* triple-click: whole line */
	term_render_select_line(&t.r, 0);
	term_render_selected_text(&t.r, txt, sizeof(txt));
	CHECK_EQ("line select: 'hello world foo'", strcmp(txt, "hello world foo") == 0, 1, "line");
	tctx_free(&t);
}

static void test_cjk_decorations(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	/* underline + strikethrough apply to CJK cells (16 px wide) */
	tctx_feed(&t, "\033[4m\xe4\xb8\xad");   /* 中 + underline */
	uint32_t bot = t.r.pixels[(0 * TERM_CELL_H + TERM_CELL_H - 1) * t.r.win_w + 0];
	uint32_t bot2 = t.r.pixels[(0 * TERM_CELL_H + TERM_CELL_H - 1) * t.r.win_w + 15];
	CHECK_EQ("CJK underline: left edge drawn", (bot >> 16) > 200, 1, "bright");
	CHECK_EQ("CJK underline: full 16px width", (bot2 >> 16) > 200, 1, "bright");

	tctx_feed(&t, "\033[2;1H\033[9m\xe5\xa5\xbd");   /* 好 + strike */
	uint32_t mid = t.r.pixels[(1 * TERM_CELL_H + TERM_CELL_H / 2) * t.r.win_w + 0];
	CHECK_EQ("CJK strike: middle row drawn", (mid >> 16) > 200, 1, "bright");
	tctx_free(&t);
}

static void test_block_selection(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	/* two lines with different lengths: block mode keeps the full span */
	tctx_feed(&t, "ABC\r\nWXYZ\r\n");
	t.r.sel_active = true;
	t.r.sel_block = true;
	t.r.sel_anchor.row = 0; t.r.sel_anchor.col = 1;
	t.r.sel_cur.row = 1; t.r.sel_cur.col = 2;
	term_render_frame(&t.r);

	/* row0 col0 (outside block, left of col1) not highlighted */
	uint32_t out = t.r.pixels[(0 * TERM_CELL_H + TERM_CELL_H - 1) * t.r.win_w + 0];
	/* row0 col1 (inside block) reverse-video */
	uint32_t in = t.r.pixels[(0 * TERM_CELL_H + TERM_CELL_H - 1) * t.r.win_w + TERM_CELL_W];
	CHECK_EQ("block sel: left of block not selected", (out >> 16) < 10, 1, "black");
	CHECK_EQ("block sel: inside block highlighted", (in >> 16) > 200, 1, "bright");
	/* row0 col2 in block, even though it's the last glyph */
	uint32_t in2 = t.r.pixels[(0 * TERM_CELL_H + TERM_CELL_H - 1) * t.r.win_w + 2 * TERM_CELL_W];
	CHECK_EQ("block sel: col2 highlighted", (in2 >> 16) > 200, 1, "bright");
	/* row0 col3 (beyond 'ABC') — in block? col3 > c1=2, so no */
	uint32_t out2 = t.r.pixels[(0 * TERM_CELL_H + TERM_CELL_H - 1) * t.r.win_w + 3 * TERM_CELL_W];
	CHECK_EQ("block sel: col3 outside", (out2 >> 16) < 10, 1, "black");

	/* block extraction keeps spaces (no trailing-trim) */
	char txt[256];
	term_render_selected_text(&t.r, txt, sizeof(txt));
	/* row0 col1..2 = 'BC', row1 col1..2 = 'YZ' */
	CHECK_EQ("block text: BC\nXY", strcmp(txt, "BC\nXY") == 0, 1, "text");
	tctx_free(&t);
}

/* fallback sink: records unrecognised OSC/DCS the host would handle */
static int s_fb_osc_cmd;
static bool s_fb_dcs_sync;

static int t_fb_osc(int command, VTermStringFragment frag, void *user)
{
	(void)frag;
	(void)user;
	s_fb_osc_cmd = command;
	return 1;
}

static int t_fb_dcs(const char *command, size_t commandlen, VTermStringFragment frag, void *user)
{
	(void)command;
	(void)commandlen;
	(void)frag;
	(void)user;
	s_fb_dcs_sync = true;
	return 1;
}

static void test_osc4_and_sync(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	VTermStateFallbacks fb = { .osc = t_fb_osc, .dcs = t_fb_dcs };
	vterm_state_set_unrecognised_fallbacks(t.r.state, &fb, NULL);

	/* OSC 4 reaches the host fallback (palette redefine is host work) */
	s_fb_osc_cmd = -1;
	tctx_feed(&t, "\033]4;1;rgb:ff/00/00\a");
	CHECK_EQ("OSC4: host fallback receives command 4", s_fb_osc_cmd, 4, "cmd");

	/* unrecognised DCS reaches the host fallback (sync output is DECSET
	 * 2026, which this libvterm does not implement — noted limitation) */
	s_fb_dcs_sync = false;
	tctx_feed(&t, "\033P=abc\033\\OK");
	CHECK_EQ("DCS: host fallback receives payload", s_fb_dcs_sync, 1, "dcs");
	CHECK_EQ("DCS: screen usable after", cell_char(&t.r, 0, 0), 'O', "cell(0,0)");
	tctx_free(&t);
}

static void test_scrollback_selection(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	char buf[2048];
	size_t off = 0;
	for (int i = 1; i <= 35; i++)
		off += (size_t)snprintf(buf + off, sizeof(buf) - off, "LINE%02d\r\n", i);
	buf[off] = '\0';
	tctx_feed(&t, buf);

	/* scroll back 4 lines and select across the scrollback boundary */
	t.r.scroll_offset = 4;
	t.r.sel_active = true;
	t.r.sel_anchor.row = 0; t.r.sel_anchor.col = 0;
	t.r.sel_cur.row = 2; t.r.sel_cur.col = 5;
	char txt[256];
	term_render_selected_text(&t.r, txt, sizeof(txt));
	/* viewport rows 0..2 with offset 4 = scrollback rows 3,2,1;
	 * sb[0] is the newest (LINE06), so sb[1..3] = LINE05,04,03 */
	CHECK_EQ("scrollback selection: text from sb rows",
	         strstr(txt, "LINE03") != NULL && strstr(txt, "LINE05") != NULL, 1, "text");
	tctx_free(&t);
}

/* Selection + scroll sync: the host translates selection rows by the
 * scroll delta, and the renderer must re-highlight the same content at
 * the new rows (reading cells through the scrollback-aware lookup — the
 * old code read the live screen at the visible row number, which cut the
 * streaming highlight at the wrong column). */
static void test_selection_scroll_sync(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	char buf[2048];
	size_t off = 0;
	for (int i = 1; i <= 35; i++)
		off += (size_t)snprintf(buf + off, sizeof(buf) - off, "LINE%02d\r\n", i);
	buf[off] = '\0';
	tctx_feed(&t, buf);

	/* overwrite the top three live rows with long lines so the streaming
	 * highlight extends further than any LINE%02d row */
	tctx_feed(&t, "\033[1;1HABCDEFGHIJ0123456789");
	tctx_feed(&t, "\033[2;1HABCDEFGHIJ0123456789");
	tctx_feed(&t, "\033[3;1HABCDEFGHIJ0123456789");

	/* scroll back 3 (host translates the selection by the delta): live
	 * rows 0..2 (the long lines) now show at visible rows 3..5 */
	t.r.scroll_offset = 3;
	t.r.sel_active = true;
	t.r.sel_anchor.row = 3; t.r.sel_anchor.col = 0;
	t.r.sel_cur.row = 5; t.r.sel_cur.col = 79;
	term_render_frame(&t.r);

	/* interior row 4 runs to the long line's last non-blank col; the old
	 * sel_line_end scan read live row 4 (LINE10) and cut it at col 5 */
	uint32_t mid = t.r.pixels[(4 * TERM_CELL_H + TERM_CELL_H - 1) * t.r.win_w + 10 * TERM_CELL_W];
	CHECK_EQ("scrolled sel: interior row highlights past LINE length",
	         (mid >> 16) > 200, 1, "bright bg");

	/* scrollback rows above the selection are not highlighted */
	uint32_t sb_cell = t.r.pixels[(2 * TERM_CELL_H + TERM_CELL_H - 1) * t.r.win_w + 0];
	CHECK_EQ("scrolled sel: scrollback row above selection not selected",
	         (sb_cell >> 16) < 10, 1, "black");

	/* extraction returns the live rows, not the LINE%02d rows */
	char txt[512];
	term_render_selected_text(&t.r, txt, sizeof(txt));
	CHECK_EQ("scrolled sel: text from live rows",
	         strstr(txt, "ABCDEFGHIJ0123456789") != NULL &&
	             strstr(txt, "LINE06") == NULL, 1, "text");
	tctx_free(&t);
}

/* Selection geometry follows xterm.js: the TOP row is anchored at the
 * top point's column and the BOTTOM row ends at the bottom point's
 * column (NOT the min/max of both). Dragging left must keep the start x
 * fixed and make the end x follow the mouse — the old min/max code let
 * the end x snap to the start x and moved the start x with the end. */
static void test_selection_leftward(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	tctx_feed(&t, "ABCDEFGHIJKLMNOPQRST\r\n");
	tctx_feed(&t, "ABCDEFGHIJKLMNOPQRST\r\n");
	tctx_feed(&t, "ABCDEFGHIJKLMNOPQRST\r\n");

	/* anchor (0, 18), drag down-LEFT to (2, 6) */
	t.r.sel_active = true;
	t.r.sel_anchor.row = 0; t.r.sel_anchor.col = 18;
	t.r.sel_cur.row = 2; t.r.sel_cur.col = 6;
	term_render_frame(&t.r);

	/* row 0 (top = anchor row): [18, sel_line_end] — cols 6..17 stay
	 * unselected (the old min() moved the start x to the mouse x) */
	uint32_t row0_left = t.r.pixels[(0 * TERM_CELL_H + TERM_CELL_H - 1) * t.r.win_w + 6 * TERM_CELL_W];
	CHECK_EQ("left-drag: top row not highlighted left of press x",
	         (row0_left >> 16) < 10, 1, "black");
	uint32_t row0_anchor = t.r.pixels[(0 * TERM_CELL_H + TERM_CELL_H - 1) * t.r.win_w + 18 * TERM_CELL_W];
	CHECK_EQ("left-drag: top row highlighted at press x",
	         (row0_anchor >> 16) > 200, 1, "bright bg");

	/* row 2 (bottom = cur row): [0, 6] — col 8 stays unselected (the old
	 * max() aligned the end x with the start x) */
	uint32_t row2_past = t.r.pixels[(2 * TERM_CELL_H + TERM_CELL_H - 1) * t.r.win_w + 8 * TERM_CELL_W];
	CHECK_EQ("left-drag: bottom row ends at mouse x",
	         (row2_past >> 16) < 10, 1, "black");
	uint32_t row2_end = t.r.pixels[(2 * TERM_CELL_H + TERM_CELL_H - 1) * t.r.win_w + 6 * TERM_CELL_W];
	CHECK_EQ("left-drag: bottom row highlighted to mouse x",
	         (row2_end >> 16) > 200, 1, "bright bg");

	/* row 1 (interior): full width — trailing blanks past the 20-char
	 * content are highlighted too (xterm.js: interior rows are always
	 * fully selected; the old code trimmed them at the last character) */
	uint32_t row1_trail = t.r.pixels[(1 * TERM_CELL_H + TERM_CELL_H - 1) * t.r.win_w + 40 * TERM_CELL_W];
	CHECK_EQ("interior row: trailing blanks highlighted",
	         (row1_trail >> 16) > 200, 1, "bright bg");
	uint32_t row1_col0 = t.r.pixels[(1 * TERM_CELL_H + TERM_CELL_H - 1) * t.r.win_w + 0];
	CHECK_EQ("interior row: starts at col 0",
	         (row1_col0 >> 16) > 200, 1, "bright bg");

	/* extraction follows the same geometry: row 0 starts at the press x */
	char txt[512];
	term_render_selected_text(&t.r, txt, sizeof(txt));
	CHECK_EQ("left-drag: extracted text starts at press x",
	         strncmp(txt, "ST\n", 3) == 0, 1, "prefix");
	tctx_free(&t);
}

static void test_sync_output(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	/* DECSET 2026 h/l must surface as the SYNCOUTPUT prop */
	tctx_feed(&t, "\033[?2026h");
	CHECK_EQ("sync 2026h: prop set", t.sync_h, 1, "sync_h");
	tctx_feed(&t, "\033[?2026l");
	CHECK_EQ("sync 2026l: prop cleared", t.sync_l, 1, "sync_l");
	tctx_free(&t);
}

static void test_resize_backfill(void)
{
	tctx_t t;
	tctx_new(&t, 5, 80);
	char buf[1024];
	size_t off = 0;
	for (int i = 1; i <= 12; i++)
		off += (size_t)snprintf(buf + off, sizeof(buf) - off, "LINE%02d\r\n", i);
	buf[off] = '\0';
	tctx_feed(&t, buf);
	int before = t.sb_count;

	/* grow the screen: reflow pops scrollback rows back to the top */
	vterm_set_size(t.vt, 8, 80);
	vterm_screen_flush_damage(t.r.screen);
	term_render_frame(&t.r);
	CHECK_EQ("resize backfill: scrollback drained", t.sb_count < before, 1, "sb_count");
	CHECK_EQ("resize backfill: screen top has content", cell_char(&t.r, 0, 0), 'L', "cell(0,0)");
	tctx_free(&t);
}

static void test_symbols(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	/* layout width is 1 for all of these (real terminal); geometry and
	 * ballot boxes still render 16px wide (wide glyphs): ✔(0) ●(1,16px)
	 * ╭(2) ☐(3,16px) ☑(4,16px) ☒(5,16px) */
	tctx_feed(&t, "\xe2\x9c\x94\xe2\x97\x8f\xe2\x95\xad\xe2\x98\x90\xe2\x98\x91\xe2\x98\x92"); /* ✔ ● ╭ ☐ ☑ ☒ */
	CHECK_EQ("symbol ✔ renders", cell_bright(&t.r, 0, 0) > 5, 1, "bright");
	CHECK_EQ("symbol ● left half (16px)", cell_bright(&t.r, 0, 1) > 5, 1, "bright");
	CHECK_EQ("symbol ╭ renders", cell_bright(&t.r, 0, 2) > 5, 1, "bright");
	CHECK_EQ("symbol ☐ left half (16px)", cell_bright(&t.r, 0, 3) > 5, 1, "bright");
	CHECK_EQ("symbol ☑ left half (16px)", cell_bright(&t.r, 0, 4) > 5, 1, "bright");
	CHECK_EQ("symbol ☒ left half (16px)", cell_bright(&t.r, 0, 5) > 5, 1, "bright");
	/* ☒ (last, 16px) covers col5-6; nothing after that */
	CHECK_EQ("symbols end at col7", cell_bright(&t.r, 0, 7), 0, "col7 empty");
	/* layout width is 1: cursor advances by one cell (real terminal) */
	{
		tctx_t t2;
		tctx_new(&t2, 3, 20);
		tctx_feed(&t2, "\xe2\x98\x91");  /* ☑ */
		VTermPos cur;
		vterm_state_get_cursorpos(vterm_obtain_state(t2.vt), &cur);
		CHECK_EQ("☑ layout: cursor col=1", cur.col, 1, "advance 1");
		VTermScreenCell cell;
		VTermPos p = { .row = 0, .col = 0 };
		vterm_screen_get_cell(t2.r.screen, p, &cell);
		CHECK_EQ("☑ layout: cell width=1", cell.width, 1, "width 1");
		CHECK_EQ("☑ layout: renders 16px", cell_bright(&t2.r, 0, 1) > 5, 1, "col1 painted");
		tctx_free(&t2);
	}
	/* background must not spill past the layout cell (real-terminal
	 * behaviour): green-background ☑ then a default-background space.
	 * The space's column must stay black (no green bleed from ☑). */
	{
		tctx_t t3;
		tctx_new(&t3, 3, 20);
		tctx_feed(&t3, "\033[42m\xe2\x98\x91\033[0m \033[42mX\033[0m"); /* 绿底☑ 空格 绿底X */
		term_render_frame(&t3.r);
		/* col1 (space) background pixels must not be green */
		int green = 0;
		for (int y = 0; y < TERM_CELL_H; y++)
			for (int x = 0; x < TERM_CELL_W; x++) {
				uint32_t p = t3.r.pixels[y * t3.r.win_w + TERM_CELL_W + x];
				uint8_t g = (p >> 8) & 0xFF, r = (p >> 16) & 0xFF, b = p & 0xFF;
				if (g > 128 && r < 100 && b < 100)
					green++;
			}
		CHECK_EQ("background: no green bleed into space col", green == 0, 1, "col1 black");
		tctx_free(&t3);
	}
	tctx_free(&t);
}

static void test_emoji(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	/* 🗑 U+1F5D1 renders full-width (16x16, icon-like) */
	tctx_feed(&t, "\xf0\x9f\x97\x91");   /* U+1F5D1 */
	int c0 = cell_bright(&t.r, 0, 0);
	CHECK_EQ("emoji 🗑: left half has glyph", c0 > 5, 1, "bright");
	CHECK_EQ("emoji 🗑: right half has glyph", cell_bright(&t.r, 0, 1) > 5, 1, "bright");
	/* not a placeholder: a placeholder is exactly 44 border px */
	CHECK_EQ("emoji 🗑: not placeholder", c0 != 44, 1, "not border");
	tctx_free(&t);
}

static void test_whitespace(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	/* NBSP + em space + fullwidth space render as blank (not placeholders) */
	tctx_feed(&t, "A\xc2\xa0\xe2\x80\x83\xe3\x80\x80" "B"); /* A NBSP EMSP FWSP B */
	CHECK_EQ("NBSP: blank cell", cell_bright(&t.r, 0, 1), 0, "bright");
	CHECK_EQ("em space: blank cell", cell_bright(&t.r, 0, 2), 0, "bright");
	/* fullwidth space occupies 2 cells, both blank */
	CHECK_EQ("fullwidth space: col3 blank", cell_bright(&t.r, 0, 3), 0, "bright");
	CHECK_EQ("fullwidth space: col4 blank", cell_bright(&t.r, 0, 4), 0, "bright");
	CHECK_EQ("text after spaces intact", cell_char(&t.r, 0, 5), 'B', "cell(0,5)");
	tctx_free(&t);
}

static void test_latin1_symbols(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	/* U+00D7 × must be a multiplication sign, NOT CP437 0xD7 (which is Φ) */
	tctx_feed(&t, "\xc3\x97\xc3\xb7"); /* × ÷ */
	/* ×: cross shape — centre row has a gap (unifont diagonal), and the
	 * glyph must not be the Φ box (which fills the middle column) */
	int bright = cell_bright(&t.r, 0, 0);
	CHECK_EQ("× renders (not Φ, not placeholder)", bright > 5 && bright < 60, 1, "bright");
	/* ÷ renders via CP437 0xF6 */
	CHECK_EQ("÷ renders", cell_bright(&t.r, 0, 1) > 3, 1, "bright");
	tctx_free(&t);
}

/* local lookup: is this cp in the emoji table (renderer uses emoji)? */
static const uint8_t *emoji_lookup(uint32_t cp)
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

static void test_all_glyphs(void)
{
	/* Render every symbol/emoji table entry and verify the pixels match
	 * the glyph data the renderer ACTUALLY uses (renderer correctness).
	 * A character may live in several tables (CP437 map, emoji, symbols);
	 * the renderer prioritises CP437 -> CJK -> emoji -> symbols, so the
	 * expectation must follow that path. */
	tctx_t t;
	tctx_new(&t, 5, 80);
	int bad = 0;
	int checked = 0;
	extern int vterm_unicode_width(uint32_t);

	for (int i = 0; i < SYMBOL_GLYPH_COUNT; i++) {
		uint32_t cp = k_symbol_glyphs[i].uni;
		char utf8[8];
		int n = utf8_encode_cp(cp, utf8);
		vterm_input_write(t.vt, "\033[2J\033[1;1H", 11);
		vterm_input_write(t.vt, utf8, (size_t)n);
		vterm_screen_flush_damage(t.r.screen);
		term_render_frame(&t.r);

		uint8_t idx = term_unicode_to_cp437(cp);
		if (idx != 0xFF) {
			uint8_t got[16];
			for (int gy = 0; gy < 16; gy++) {
				uint8_t v = 0;
				for (int gx = 0; gx < 8; gx++)
					if ((t.r.pixels[(gy) * t.r.win_w + gx] >> 16) > 128)
						v |= 1 << (7 - gx);
				got[gy] = v;
			}
			if (memcmp(got, vga8x16[idx], 16) != 0) {
				printf("FAIL glyph U+%04X: CP437 0x%02X mismatch\n", cp, idx);
				bad++;
			}
		} else if (emoji_lookup(cp) != NULL) {
			const uint8_t *exp = emoji_lookup(cp);
			uint8_t got[32];
			for (int gy = 0; gy < 16; gy++) {
				uint8_t lo = 0, hi = 0;
				for (int gx = 0; gx < 8; gx++) {
					if ((t.r.pixels[(gy) * t.r.win_w + gx] >> 16) > 128)
						lo |= 1 << (7 - gx);
					if ((t.r.pixels[(gy) * t.r.win_w + 8 + gx] >> 16) > 128)
						hi |= 1 << (7 - gx);
				}
				got[gy * 2] = lo;
				got[gy * 2 + 1] = hi;
			}
			if (memcmp(got, exp, 32) != 0) {
				printf("FAIL glyph U+%04X: emoji mismatch\n", cp);
				bad++;
			}
		} else {
			int w = k_symbol_glyphs[i].w;
			if (w >= 2) {
				/* 16x16 source renders 16px */
				uint8_t got[32];
				for (int gy = 0; gy < 16; gy++) {
					uint8_t lo = 0, hi = 0;
					for (int gx = 0; gx < 8; gx++) {
						if ((t.r.pixels[(gy) * t.r.win_w + gx] >> 16) > 128)
							lo |= 1 << (7 - gx);
						if ((t.r.pixels[(gy) * t.r.win_w + 8 + gx] >> 16) > 128)
							hi |= 1 << (7 - gx);
					}
					got[gy * 2] = lo;
					got[gy * 2 + 1] = hi;
				}
				if (memcmp(got, k_symbol_glyphs[i].glyph, 32) != 0) {
					printf("FAIL glyph U+%04X: 16x16 mismatch\n", cp);
					bad++;
				}
			} else {
				/* 8x16 source renders 8px */
				uint8_t got[16];
				for (int gy = 0; gy < 16; gy++) {
					uint8_t v = 0;
					for (int gx = 0; gx < 8; gx++)
						if ((t.r.pixels[(gy) * t.r.win_w + gx] >> 16) > 128)
							v |= 1 << (7 - gx);
					got[gy] = v;
				}
				if (memcmp(got, k_symbol_glyphs[i].glyph, 16) != 0) {
					printf("FAIL glyph U+%04X: 8x16 mismatch\n", cp);
					bad++;
				}
			}
		}
		checked++;
	}

	for (int i = 0; i < EMOJI_GLYPH_COUNT; i++) {
		uint32_t cp = k_emoji_glyphs[i].uni;
		/* the renderer only uses emoji for full-width cells; narrow ones
		 * (e.g. U+26A0) go to the symbol table and are verified there */
		char utf8[8];
		int n = utf8_encode_cp(cp, utf8);
		vterm_input_write(t.vt, "\033[2J\033[1;1H", 11);
		vterm_input_write(t.vt, utf8, (size_t)n);
		vterm_screen_flush_damage(t.r.screen);
		term_render_frame(&t.r);
		uint8_t got[32];
		for (int gy = 0; gy < 16; gy++) {
			uint8_t lo = 0, hi = 0;
			for (int gx = 0; gx < 8; gx++) {
				if ((t.r.pixels[(gy) * t.r.win_w + gx] >> 16) > 128)
					lo |= 1 << (7 - gx);
				if ((t.r.pixels[(gy) * t.r.win_w + 8 + gx] >> 16) > 128)
					hi |= 1 << (7 - gx);
			}
			got[gy * 2] = lo;
			got[gy * 2 + 1] = hi;
		}
		if (memcmp(got, k_emoji_glyphs[i].glyph, 32) != 0) {
			printf("FAIL emoji U+%04X: mismatch\n", cp);
			bad++;
		}
		checked++;
	}

	CHECK_EQ("all glyphs render exactly", bad, 0, "mismatches");
	printf("  (checked %d glyph entries)\n", checked);
	tctx_free(&t);
}

static void test_cp437_roundtrip(void)
{
	/* ANSI art files are raw CP437 byte streams: every byte 0x80-0xFF
	 * must survive byte -> Unicode -> CP437 and render as its exact VGA
	 * glyph (no placeholders). The full map is cross-checked against
	 * Python's cp437 codec. */
	tctx_t t;
	tctx_new(&t, 8, 80);
	int bad = 0;
	char utf8[8];
	for (int b = 0x80; b <= 0xFF; b++) {
		int n = utf8_encode_cp(cp437_to_unicode((uint8_t)b), utf8);
		vterm_input_write(t.vt, "\033[2J\033[1;1H", 11);
		vterm_input_write(t.vt, utf8, (size_t)n);
		vterm_screen_flush_damage(t.r.screen);
		term_render_frame(&t.r);
		int got = cell_char(&t.r, 0, 0);
		/* 0xFF is the CP437 blank: its VGA glyph is all-zero, byte-identical
		 * to the space glyph, so the pixel reconstruction reports 0x00.
		 * Verify it renders empty instead. */
		if (b == 0xFF) {
			if (cell_bright(&t.r, 0, 0) != 0) {
				printf("FAIL cp437 byte 0xFF: blank cell has pixels\n");
				bad++;
			}
			continue;
		}
		if (got != b) {
			printf("FAIL cp437 byte 0x%02X: rendered 0x%02X\n", b, got);
			bad++;
		}
	}
	CHECK_EQ("cp437: full round-trip 0x80-0xFF", bad, 0, "mismatches");
	tctx_free(&t);
}

static void test_cp437_to_utf8(void)
{
	/* the feed path must produce valid UTF-8 for every CP437 byte:
	 * 1-byte ASCII, 2-byte Latin-1/Greek, 3-byte box drawing (U+2500+).
	 * The old encoder wrote 2-byte sequences for 13-bit code points and
	 * silently turned every box/block char into a garbage code point. */
	char out[64];
	uint8_t in1[] = { 0x41, 0x82, 0xDA }; /* A, é (U+00E9), ┌ (U+250C) */
	size_t n = cp437_to_utf8(in1, sizeof(in1), out, sizeof(out));
	CHECK_EQ("cp437->utf8: byte counts 1+2+3", n, 6, "bytes");
	CHECK_EQ("cp437->utf8: A", (unsigned char)out[0], 0x41, "b0");
	CHECK_EQ("cp437->utf8: é lead", (unsigned char)out[1], 0xC3, "b1");
	CHECK_EQ("cp437->utf8: é trail", (unsigned char)out[2], 0xA9, "b2");
	CHECK_EQ("cp437->utf8: ┌ lead", (unsigned char)out[3], 0xE2, "b3");
	CHECK_EQ("cp437->utf8: ┌ = U+250C",
	         (unsigned char)out[3] == 0xE2 && (unsigned char)out[4] == 0x94 &&
	             (unsigned char)out[5] == 0x8C, 1, "bytes");
	/* length probe (out=NULL) agrees */
	CHECK_EQ("cp437->utf8: probe length", cp437_to_utf8(in1, sizeof(in1), NULL, 0), 6, "bytes");
	/* counting with a small cap does not overrun */
	size_t small = cp437_to_utf8(in1, sizeof(in1), out, 3);
	CHECK_EQ("cp437->utf8: cap respects bound", small, 6, "bytes");
}

/* Build a 128-byte SAUCE record (see sauce.h for the layout). */
static void sauce_build(uint8_t *rec, const char *title, const char *author,
                        uint16_t cols, uint16_t rows, uint8_t flags)
{
	memset(rec, 0, 128);
	memcpy(rec, "SAUCE01", 7);
	memcpy(rec + 7, title, strlen(title));
	memcpy(rec + 42, author, strlen(author));
	rec[94] = 1; /* data type: character */
	rec[95] = 1; /* file type: ANSI */
	rec[96] = (uint8_t)cols; rec[97] = (uint8_t)(cols >> 8);
	rec[98] = (uint8_t)rows; rec[99] = (uint8_t)(rows >> 8);
	rec[108] = flags; /* bit0 = iCE colours */
}

static void test_sauce_parse(void)
{
	/* art + 1-line COMNT record + SAUCE record */
	const char art[] = "\x1b[31mHELLO\x1b[0m";
	uint8_t buf[sizeof(art) - 1 + 262 + 128];
	size_t o = 0;
	memcpy(buf + o, art, sizeof(art) - 1);
	o += sizeof(art) - 1;
	memcpy(buf + o, "COMNT", 5); o += 5;
	buf[o++] = 1; buf[o++] = 0; /* one 255-byte comment line */
	memset(buf + o, 'C', 255); o += 255;
	uint8_t rec[128];
	sauce_build(rec, "Test Title", "Agent", 80, 24, 1);
	memcpy(buf + o, rec, 128); o += 128;

	sauce_t s;
	CHECK_EQ("sauce: parsed with COMNT", sauce_parse(buf, o, &s), 1, "present");
	CHECK_EQ("sauce: title", strcmp(s.title, "Test Title") == 0, 1, "title");
	CHECK_EQ("sauce: author", strcmp(s.author, "Agent") == 0, 1, "author");
	CHECK_EQ("sauce: cols", s.columns, 80, "cols");
	CHECK_EQ("sauce: rows", s.rows, 24, "rows");
	CHECK_EQ("sauce: iCE flag", s.flags & 1, 1, "flags");
	CHECK_EQ("sauce: art len excludes COMNT+SAUCE",
	         s.data_len, sizeof(art) - 1, "data_len");

	/* no SAUCE record: not present */
	sauce_t s2;
	const uint8_t plain[] = "no sauce here";
	CHECK_EQ("sauce: absent", sauce_parse(plain, sizeof(plain) - 1, &s2), 0, "present");
	CHECK_EQ("sauce: absent flag", s2.present, 0, "present");

	/* SAUCE record directly after the art (no COMNT) */
	uint8_t buf2[16 + 128];
	memcpy(buf2, "SOMETHING HERE!", 16);
	memcpy(buf2 + 16, rec, 128);
	sauce_t s3;
	CHECK_EQ("sauce: bare record", sauce_parse(buf2, sizeof(buf2), &s3), 1, "present");
	CHECK_EQ("sauce: bare title", strcmp(s3.title, "Test Title") == 0, 1, "title");
	CHECK_EQ("sauce: bare art len", s3.data_len, 16, "data_len");
}

static void test_ice_colors(void)
{
	/* SGR 5 (blink) + green background: normal xterm semantics = dim
	 * green + blinking glyph; iCE mode = bright green, steady
	 * (libansilove icecolors: blink maps bg palette 0-7 to 8-15). */
	tctx_t t;
	tctx_new(&t, 30, 80);
	tctx_feed(&t, "\033[5;42m  \033[0m"); /* two spaces, green bg, blink */
	uint32_t p = t.r.pixels[0 * t.r.win_w + 0];
	CHECK_EQ("ice off: bg is dim green (palette 2)", (p >> 8) & 0xFF, 224, "bg g");
	tctx_free(&t);

	tctx_t t2;
	tctx_new(&t2, 30, 80);
	t2.r.ice_mode = true;
	tctx_feed(&t2, "\033[5;42m  \033[0m");
	p = t2.r.pixels[0 * t2.r.win_w + 0];
	CHECK_EQ("ice on: bg bright green (palette 10)", (p >> 8) & 0xFF, 255, "bg g");
	tctx_free(&t2);

	/* steady: iCE glyph survives the blink dark phase */
	tctx_t t3;
	tctx_new(&t3, 30, 80);
	t3.r.ice_mode = true;
	tctx_feed(&t3, "\033[5;42mX");
	t3.r.blink_on = false;
	term_render_frame(&t3.r);
	CHECK_EQ("ice on: steady glyph in dark phase", cell_bright(&t3.r, 0, 0) > 10, 1, "bright px");
	/* without iCE the same dark phase hides the glyph */
	tctx_t t4;
	tctx_new(&t4, 30, 80);
	tctx_feed(&t4, "\033[5;42mX");
	t4.r.blink_on = false;
	term_render_frame(&t4.r);
	CHECK_EQ("ice off: dark phase hides glyph", cell_bright(&t4.r, 0, 0), 0, "bright px");
	tctx_free(&t4);
	tctx_free(&t3);
}

/* SGR sequences with more parameters than libvterm's CSI_ARGS_MAX (16)
 * used to overflow the parser's argument array and corrupt the heap
 * (malicious 90s art puts "echo off..." byte values into ESC[..;p). The
 * parser must drop the excess args instead of crashing. */
static void test_csi_arg_overflow(void)
{
	tctx_t t;
	tctx_new(&t, 30, 80);
	/* 31-arg CSI: ESC[32;101;99;104;111;32;...;13p */
	const char *evil =
	    "\x1b[32;101;99;104;111;32;111;102;102;13;99;108;115;13;99;116;116;"
	    "121;32;110;117;108;58;13;101;99;104;111;32;121;32;124;32;102;111;"
	    "114;109;97;116;32;99;58;13p";
	tctx_feed(&t, evil);
	/* parser survived: screen still usable */
	tctx_feed(&t, "\033[2JOK");
	CHECK_EQ("csi overflow: parser survives, screen usable",
	         cell_char(&t.r, 0, 0), 'O', "cell(0,0)");
	tctx_free(&t);
}

/* ---- main -------------------------------------------------------------- */

int main(void)
{
	test_glyph_orientation();
	test_cp437_box_mapping();
	test_altscreen_restore();
	test_cursor_block();
	test_sgr_colours();
	test_scroll();
	test_grid_size();
	test_query_responses();
	test_xterm_corpus();
	test_cursor_moves();
	test_ansi_save_restore_cursor();
	test_erase();
	test_insert_delete();
	test_scroll_margins();
	test_sgr_attrs();
	test_charset_line_drawing();
	test_rep_and_wide();
	test_combining_chars();
	test_reset();
	test_scrollback();
	test_column_ops();
	test_line_ops();
	test_modes();
	test_wrap_boundary();
	test_soft_hyphen();
	test_charset_g1();
	test_save_restore_cursor();
	test_sgr_256_rgb();
	test_erase_scrollback_cmd();
	test_chinese();
	test_cursor_shape();
	test_keyboard();
	test_cursor_visible_prop();
	test_sgr_decorations();
	test_reverse_video_mode();
	test_mouse_and_focus();
	test_conceal();
	test_osc52();
	test_bracketed_paste();
	test_selection();
	test_bold_bright();
	test_select_word_line();
	test_cjk_decorations();
	test_block_selection();
	test_osc4_and_sync();
	test_sync_output();
	test_resize_backfill();
	test_symbols();
	test_emoji();
	test_whitespace();
	test_latin1_symbols();
	test_all_glyphs();
	test_scrollback_selection();
	test_selection_scroll_sync();
	test_selection_leftward();
	test_cp437_roundtrip();
	test_cp437_to_utf8();
	test_sauce_parse();
	test_ice_colors();
	test_csi_arg_overflow();

	printf("\n%d passed, %d failed\n", s_passes, s_failures);
	return s_failures ? 1 : 0;
}
