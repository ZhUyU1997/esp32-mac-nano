/* vterm-ans — ANSI art to image converter (libansilove-style).
 *
 * Feeds a BBS-era art file (.ans/.ice/.nfo, CP437 bytes) through the
 * vendored libvterm parser, renders the whole grid with the existing
 * term_render pipeline and writes a PNG (or BMP). Uses the SAUCE record
 * when present to auto-detect columns, iCE colours and the font code,
 * mirroring what the ansilove CLI does on top of libansilove.
 *
 * Usage:
 *   vterm-ans file.ans                 -> file.png (SAUCE-aware)
 *   vterm-ans -o out.png file.ans
 *   vterm-ans -o out.bmp --aspect file.ans   (2x horizontal stretch)
 *   vterm-ans --ice file.ans           (force iCE colours)
 *   vterm-ans --cols 100 file.ans      (override SAUCE columns)
 *
 * Differences vs libansilove (deliberate):
 *   - parsing is libvterm (xterm semantics), not the mini ANSI.SYS
 *     state machine — EL/K and the full SGR set behave like a terminal;
 *   - the 16-colour palette is libvterm's xterm palette, not the VGA
 *     ANSI palette;
 *   - output is the full declared width (SAUCE cols or 80) like
 *     libansilove's columns option; height is the last content row.
 */
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zlib.h>

#include "vterm.h"
#include "term_render.h"
#include "cp437.h"
#include "sauce.h"

static int s_rows_screen = 256; /* generous screen; cropped to content */

/* Walk the art bytes and return the deepest row any content can reach:
 * LF counts, CUP (\x1b[n;mH/f) jumps, CUD/CUU (nB/nA) moves, and ED 2
 * (which homes the cursor and drops the rows before it). Used to size
 * the libvterm screen so long art does not scroll. */
static int scan_max_row(const uint8_t *buf, size_t len)
{
	int row = 0, col = 0, max = 0;
	const int cols = 80; /* art width; CUF wraps at it */
	for (size_t i = 0; i < len; i++) {
		uint8_t b = buf[i];
		if (b == '\n') {
			row++;
			col = 0;
			if (row > max)
				max = row;
		} else if (b == '\r') {
			col = 0;
		} else if (b >= 0x20) {
			/* ordinary char (incl. CP437 0x80+): advances the column;
			 * writing at the margin wraps the next char to the next
			 * row — long art lines wrap hundreds of rows this way. */
			col++;
			if (col >= cols) {
				row++;
				col = 0;
				if (row > max)
					max = row;
			}
		} else if (b == 0x00) {
			/* NUL renders as a blank char (see cp437.c): advances too */
			col++;
			if (col >= cols) {
				row++;
				col = 0;
				if (row > max)
					max = row;
			}
		} else if (b == 0x1b && i + 1 < len && buf[i + 1] == '[') {
			size_t j = i + 2;
			int n = -1;
			while (j < len && buf[j] >= '0' && buf[j] <= '9') {
				n = (n < 0 ? 0 : n * 10) + (buf[j] - '0');
				j++;
			}
			if (j < len && buf[j] == ';') {
				/* CUP: n;mH/f, n is the 1-based target row */
				j++;
				while (j < len && buf[j] >= '0' && buf[j] <= '9')
					j++;
				if (j < len && (buf[j] == 'H' || buf[j] == 'f')) {
					int r = n < 1 ? 1 : n;
					if (r - 1 > max)
						max = r - 1;
					row = r - 1; /* CUP moves the cursor: later LF counts
					               from here, not from 0 (art initialises
					               with a run of CUP before the LF stream) */
				}
			} else if (j < len && buf[j] == 'B') {
				row += n < 1 ? 1 : n;
				if (row > max)
					max = row;
			} else if (j < len && buf[j] == 'A') {
				/* CUU only moves the cursor up for over-drawing; the deepest
				 * row content can reach is set by LF/CUD/CUP, not by how
				 * often art redraws a line. Not decrementing over-estimates
				 * the screen size slightly (harmless: it only avoids
				 * scrolling); under-estimating scrolls content away. */
			} else if (j < len && buf[j] == 'C') {
				/* CUF past the right margin wraps the next char to the
				 * next row (libansilove clamps to the width, the char
				 * then wraps); art uses e.g. \x1b[79C\x1b[1C heavily. */
				col += n < 1 ? 1 : n;
				/* a char is drawn at the margin too, so wrapping can be
				 * triggered from col cols-1; count that conservatively
				 * (over-counting is harmless, under-counting scrolls). */
				if (col >= cols - 1) {
					row++;
					col = 0;
					if (row > max)
						max = row;
				}
			} else if (j < len && buf[j] == 'D') {
				col -= n < 1 ? 1 : n;
				if (col < 0)
					col = 0;
			} else if (j < len && buf[j] == 'J' && n == 2) {
				row = 0; /* ED 2 homes: rows before it are gone */
				col = 0;
			}
		}
	}
	return max;
}

/* ---- libvterm callbacks ------------------------------------------------ */

/* deepest row that received a cell change (chars/attrs/erase), 1-based
 * exclusive end_row; and the highest cursor row reached (1-based) as a
 * fallback for animations that finish with an empty screen. */
typedef struct { int max_row; int damage_row; int screen_rows; } ans_ctx_t;

static int ans_damage(VTermRect rect, void *user)
{
	ans_ctx_t *ctx = user;
	/* a full-screen erase (ED 2, or the initial reset) wipes the canvas:
	 * libansilove's rowMax resets too (clear + redraw art shrinks back),
	 * so restart the count instead of keeping the pre-clear rows. */
	if (rect.start_row == 0 && rect.end_row >= ctx->screen_rows) {
		ctx->damage_row = 0;
		return 1;
	}
	/* deepest row with any cell change (chars/attrs/bg): libansilove's
	 * canvas reaches the last row a character was written to — trailing
	 * LF-only rows never damage and must not extend the canvas. */
	if (rect.end_row > ctx->damage_row)
		ctx->damage_row = rect.end_row;
	return 1;
}

/* highest cursor row reached. ANSI art end-frames often finish with an
 * empty screen (clear + overwrite animations); libansilove still renders
 * them at the cursor's max row, so track it as a fallback. */
static int ans_movecursor(VTermPos pos, VTermPos oldpos, int visible, void *user)
{
	(void)oldpos;
	(void)visible;
	ans_ctx_t *ctx = user;
	if (pos.row + 1 > ctx->max_row)
		ctx->max_row = pos.row + 1;
	return 1;
}

/* ---- helpers ----------------------------------------------------------- */

static void *read_file(const char *path, size_t *len)
{
	FILE *f = fopen(path, "rb");
	if (!f) {
		perror(path);
		return NULL;
	}
	if (fseek(f, 0, SEEK_END) != 0) {
		fclose(f);
		return NULL;
	}
	long sz = ftell(f);
	if (sz < 0) {
		fclose(f);
		return NULL;
	}
	rewind(f);
	uint8_t *buf = malloc((size_t)sz + 1);
	if (!buf) {
		fclose(f);
		return NULL;
	}
	if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
		free(buf);
		fclose(f);
		return NULL;
	}
	fclose(f);
	*len = (size_t)sz;
	return buf;
}

/* last row with any rendered content: a non-blank glyph, or a cell whose
 * background is a visible colour. Space cells with a painted bg count;
 * black/default backgrounds do not (an art file may start with ED2 which
 * erases the whole tall screen to the current pen background). */
static int screen_last_row(VTermScreen *scr, VTermState *state, int rows, int cols)
{
	for (int row = rows - 1; row >= 0; row--) {
		for (int col = 0; col < cols; col++) {
			VTermScreenCell cell;
			VTermPos p = { .row = row, .col = col };
			if (!vterm_screen_get_cell(scr, p, &cell))
				continue;
			if (cell.chars[0] != 0 && cell.chars[0] != ' ' &&
			    cell.chars[0] != (uint32_t)-1)
				return row;
			VTermColor bg = cell.bg;
			vterm_state_convert_color_to_rgb(state, &bg);
			if (bg.rgb.red != 0 || bg.rgb.green != 0 || bg.rgb.blue != 0)
				return row;
		}
	}
	return -1;
}

/* ---- image output ------------------------------------------------------ */

static void px_row(const uint32_t *px, int w, int xscale, uint8_t *row)
{
	/* expand horizontally by xscale (aspect correction) */
	for (int x = 0; x < w; x++) {
		uint8_t r = (uint8_t)(px[x] >> 16);
		uint8_t g = (uint8_t)(px[x] >> 8);
		uint8_t b = (uint8_t)px[x];
		for (int s = 0; s < xscale; s++) {
			row[(x * xscale + s) * 3] = r;
			row[(x * xscale + s) * 3 + 1] = g;
			row[(x * xscale + s) * 3 + 2] = b;
		}
	}
}

static int write_bmp(const char *path, const uint32_t *px, int w, int h, int xscale)
{
	FILE *f = fopen(path, "wb");
	if (!f) {
		perror(path);
		return -1;
	}
	int out_w = w * xscale;
	int row_pad = (4 - (out_w * 3) % 4) % 4;
	uint32_t data_size = (uint32_t)((out_w * 3 + row_pad) * h);
	uint8_t hdr[54] = { 0 };
	memcpy(hdr, "BM", 2);
	*(uint32_t *)(hdr + 2) = 54 + data_size;      /* file size */
	*(uint32_t *)(hdr + 10) = 54;                 /* pixel data offset */
	*(uint32_t *)(hdr + 14) = 40;                 /* info header size */
	*(int32_t *)(hdr + 18) = out_w;
	*(int32_t *)(hdr + 22) = h;
	*(uint16_t *)(hdr + 26) = 1;                  /* planes */
	*(uint16_t *)(hdr + 28) = 24;                 /* bpp */
	*(uint32_t *)(hdr + 34) = data_size;
	fwrite(hdr, 1, sizeof(hdr), f);
	uint8_t *row = malloc((size_t)out_w * 3 + row_pad);
	if (!row) {
		fclose(f);
		return -1;
	}
	for (int y = h - 1; y >= 0; y--) { /* BMP is bottom-up */
		px_row(px + (size_t)y * w, w, xscale, row);
		memset(row + out_w * 3, 0, (size_t)row_pad);
		fwrite(row, 1, (size_t)out_w * 3 + row_pad, f);
	}
	free(row);
	fclose(f);
	return 0;
}

static uint32_t crc32_table[256];
static void crc32_init(void)
{
	for (uint32_t i = 0; i < 256; i++) {
		uint32_t c = i;
		for (int k = 0; k < 8; k++)
			c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
		crc32_table[i] = c;
	}
}

static void png_chunk(FILE *f, const char type[4], const uint8_t *data, uint32_t n)
{
	uint8_t len[4] = { (uint8_t)(n >> 24), (uint8_t)(n >> 16), (uint8_t)(n >> 8), (uint8_t)n };
	fwrite(len, 1, 4, f);
	fwrite(type, 1, 4, f);
	if (n)
		fwrite(data, 1, n, f);
	/* CRC-32 over the type + data */
	uint32_t crc = 0xFFFFFFFFu;
	for (int i = 0; i < 4; i++)
		crc = crc32_table[(crc ^ (uint8_t)type[i]) & 0xFF] ^ (crc >> 8);
	for (uint32_t i = 0; i < n; i++)
		crc = crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
	crc ^= 0xFFFFFFFFu;
	uint8_t cb[4] = { (uint8_t)(crc >> 24), (uint8_t)(crc >> 16), (uint8_t)(crc >> 8), (uint8_t)crc };
	fwrite(cb, 1, 4, f);
}

static int write_png(const char *path, const uint32_t *px, int w, int h, int xscale)
{
	FILE *f = fopen(path, "wb");
	if (!f) {
		perror(path);
		return -1;
	}
	int out_w = w * xscale;
	/* raw scanlines: 1 filter byte + 3 bytes per pixel */
	size_t stride = (size_t)out_w * 3 + 1;
	uint8_t *raw = malloc(stride * (size_t)h);
	uint8_t *row = malloc((size_t)out_w * 3);
	if (!raw || !row) {
		free(raw);
		free(row);
		fclose(f);
		return -1;
	}
	for (int y = 0; y < h; y++) {
		raw[y * stride] = 0; /* filter: none */
		px_row(px + (size_t)y * w, w, xscale, row);
		memcpy(raw + y * stride + 1, row, (size_t)out_w * 3);
	}
	uLongf clen = compressBound((uLong)(stride * (size_t)h));
	uint8_t *cdata = malloc(clen);
	if (!cdata) {
		free(raw);
		free(row);
		fclose(f);
		return -1;
	}
	if (compress2(cdata, &clen, raw, (uLong)(stride * (size_t)h), 9) != Z_OK) {
		free(raw);
		free(row);
		free(cdata);
		fclose(f);
		return -1;
	}
	free(raw);
	free(row);

	static const uint8_t sig[8] = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n' };
	fwrite(sig, 1, 8, f);
	uint8_t ihdr[13];
	memset(ihdr, 0, sizeof(ihdr));
	ihdr[0] = (uint8_t)(out_w >> 24); ihdr[1] = (uint8_t)(out_w >> 16);
	ihdr[2] = (uint8_t)(out_w >> 8);  ihdr[3] = (uint8_t)out_w;
	ihdr[4] = (uint8_t)(h >> 24);     ihdr[5] = (uint8_t)(h >> 16);
	ihdr[6] = (uint8_t)(h >> 8);      ihdr[7] = (uint8_t)h;
	ihdr[8] = 8;  /* bit depth */
	ihdr[9] = 2;  /* colour type: truecolour */
	png_chunk(f, "IHDR", ihdr, sizeof(ihdr));
	png_chunk(f, "IDAT", cdata, (uint32_t)clen);
	png_chunk(f, "IEND", NULL, 0);
	free(cdata);
	fclose(f);
	return 0;
}

/* ---- main -------------------------------------------------------------- */

static void usage(const char *prog)
{
	fprintf(stderr,
	    "usage: %s [-o out.png|out.bmp] [--ice] [--cols N] [--aspect] [--utf8] file\n"
	    "  -o FILE   output image (default: file with .png extension)\n"
	    "  --ice     force iCE colours (SGR 5 blink -> bright background)\n"
	    "  --cols N  override the grid width (default: SAUCE cols or 80)\n"
	    "  --aspect  2x horizontal stretch (8x16 cells look square-ish)\n"
	    "  --utf8    input is already UTF-8 (no CP437 conversion)\n",
	    prog);
}

int main(int argc, char **argv)
{
	const char *out_path = NULL;
	const char *in_path = NULL;
	bool ice = false, aspect = false, utf8_in = false;
	int cols_override = 0;

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-o") && i + 1 < argc) {
			out_path = argv[++i];
		} else if (!strcmp(argv[i], "--ice")) {
			ice = true;
		} else if (!strcmp(argv[i], "--aspect")) {
			aspect = true;
		} else if (!strcmp(argv[i], "--utf8")) {
			utf8_in = true;
		} else if (!strcmp(argv[i], "--cols") && i + 1 < argc) {
			cols_override = atoi(argv[++i]);
		} else if (argv[i][0] == '-') {
			usage(argv[0]);
			return 1;
		} else {
			in_path = argv[i];
		}
	}
	if (!in_path) {
		usage(argv[0]);
		return 1;
	}

	size_t len;
	uint8_t *buf = read_file(in_path, &len);
	if (!buf)
		return 1;

	/* SAUCE metadata (columns, iCE flag, font) — optional */
	sauce_t sauce;
	bool has_sauce = sauce_parse(buf, len, &sauce);
	if (has_sauce) {
		fprintf(stderr, "sauce: \"%s\" by %s (%s), %ux%u, font=%u%s\n",
		    sauce.title, sauce.author, sauce.date, sauce.columns,
		    sauce.rows, sauce.font,
		    (sauce.flags & 1) ? ", iCE" : "");
	}

	/* art bytes = before the SAUCE record. SUB (0x1A) terminates the art
	 * anywhere: everything after it is BBS metadata (file name, desc),
	 * libansilove stops there too. Then trim trailing EOF/NUL. */
	size_t art_len = has_sauce ? sauce.data_len : len;
	for (size_t i = 0; i < art_len; i++)
		if (buf[i] == 0x1A) {
			art_len = i;
			break;
		}
	while (art_len > 0 && (buf[art_len - 1] == 0x1A || buf[art_len - 1] == 0))
		art_len--;

	int cols = cols_override > 0 ? cols_override :
	           (has_sauce && sauce.columns > 0 ? sauce.columns : 80);
	if (cols < 1 || cols > 200) {
		fprintf(stderr, "vterm-ans: columns %d out of range (1..200)\n", cols);
		free(buf);
		return 1;
	}
	/* screen must fit tall art; the renderer crops to the content box.
	 * libansilove renders on an unbounded canvas (its rowMax): art taller
	 * than the screen must NOT scroll — pre-scan the byte stream for the
	 * deepest row any content can reach and size the libvterm screen
	 * accordingly (the ESP32 gallery is a real terminal and scrolls). */
	int rows = s_rows_screen;
	int scan = scan_max_row(buf, art_len) + 1;
	/* generous headroom: content reaching row rows-1 and then LF'ing
	 * again scrolls (libansilove's canvas never does) and the scan
	 * under-counts (CUF column wraps, CUU coverage, border cases), so
	 * give it 128 rows of slack — the cost is only memory. */
	if (scan > rows)
		rows = scan < 8064 ? scan + 128 : 8192;
	if (has_sauce && sauce.rows > rows)
		rows = sauce.rows < 1024 ? sauce.rows : 1024;

	/* feed the art through libvterm */
	VTerm *vt = vterm_new(rows, cols);
	vterm_set_utf8(vt, 1);
	/* ANSI.SYS default fg is colour 7 (light grey), libvterm's is xterm
	 * white (240) — after a SGR 0 reset both must render colour 7, or
	 * every uncoloured char drifts from libansilove. */
	VTermColor def_fg, def_bg;
	vterm_color_indexed(&def_fg, 7);
	vterm_color_indexed(&def_bg, 0);
	vterm_state_set_default_colors(vterm_obtain_state(vt), &def_fg, &def_bg);
	VTermScreen *scr = vterm_obtain_screen(vt);
	ans_ctx_t ctx = { 0 };
	ctx.screen_rows = rows;
	VTermScreenCallbacks cbs = {
		.damage = ans_damage,
		.movecursor = ans_movecursor,
	};
	vterm_screen_reset(scr, 1);
	/* register after reset so the reset's full-screen erase does not
	 * inflate damage_row to the whole screen */
	vterm_screen_set_callbacks(scr, &cbs, &ctx);

	char *utf8;
	size_t utf8_len;
	if (utf8_in) {
		/* already UTF-8 (e.g. the telnet server's converted bytes) */
		utf8 = (char *)buf;
		utf8_len = art_len;
	} else {
		utf8_len = cp437_to_utf8(buf, art_len, NULL, 0);
		utf8 = malloc(utf8_len);
		if (!utf8) {
			free(buf);
			vterm_free(vt);
			return 1;
		}
		cp437_to_utf8(buf, art_len, utf8, utf8_len);
		free(buf);
	}
	vterm_input_write(vt, utf8, utf8_len);
	free(utf8);
	vterm_screen_flush_damage(scr);

	/* content box: full declared width, height to the last content row.
	 * End-frame animations may finish with an empty screen: fall back to
	 * the highest cursor row (libansilove's rowMax semantics). Files that
	 * are only a SAUCE record (no art bytes at all) fall back to the
	 * SAUCE-declared size, so the blank canvas still renders. */
	VTermState *state = vterm_obtain_state(vt);
	int last_row = screen_last_row(scr, state, rows, cols);
	if (last_row < 0)
		last_row = ctx.damage_row - 1;
	if (last_row < 0)
		last_row = ctx.max_row - 1;
	if (last_row < 0 && has_sauce && sauce.rows > 0)
		last_row = (int)sauce.rows - 1;
	if (last_row < 0) {
		fprintf(stderr, "vterm-ans: no content rendered\n");
		vterm_free(vt);
		return 1;
	}
	/* libansilove's canvas reaches the deepest row that received a cell
	 * change (a written char, an attr, an erase — but not a bare LF or
	 * cursor move): trailing LF-only rows and empty CUP gaps after the
	 * last content stay cropped, while overwrite gaps inside the content
	 * (e.g. CRS-CCL's black-shadow rows) are kept. damage_row is the
	 * 1-based exclusive end of that deepest damaged rect. */
	int img_h = last_row + 1;
	if (ctx.damage_row > img_h)
		img_h = ctx.damage_row;

	/* render the grid */
	crc32_init();
	uint32_t *px = calloc((size_t)rows * cols * TERM_CELL_W * TERM_CELL_H,
	                      sizeof(uint32_t));
	if (!px) {
		vterm_free(vt);
		return 1;
	}
	term_renderer_t r;
	term_render_init(&r, vt, px);
	/* iCE (SGR 5 -> bright background) is opt-in only: libansilove does not
	 * read the SAUCE iCE flag by default (-S enables it), so auto-enabling
	 * it here makes art with a SAUCE iCE flag render with brighter
	 * backgrounds than the reference. */
	r.ice_mode = ice;
	r.blink_on = true; /* iCE / art render is steady */
	term_render_frame(&r);
	vterm_free(vt);

	int xscale = aspect ? 2 : 1;

	if (!out_path) {
		size_t n = strlen(in_path);
		out_path = malloc(n + 5);
		memcpy((char *)out_path, in_path, n);
		strcpy((char *)out_path + n, ".png");
	}
	bool png_out = strstr(out_path, ".png") != NULL ||
	               strstr(out_path, ".PNG") != NULL;
	int rc = png_out ? write_png(out_path, px, cols * TERM_CELL_W, img_h * TERM_CELL_H, xscale)
	                 : write_bmp(out_path, px, cols * TERM_CELL_W, img_h * TERM_CELL_H, xscale);
	free(px);
	if (rc != 0)
		return 1;
	printf("%s: %dx%d -> %s\n", in_path, cols, img_h, out_path);
	return 0;
}
