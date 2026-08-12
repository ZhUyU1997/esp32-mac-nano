/* term_render.h — pixel renderer for libvterm (platform-free).
 *
 * Converts a libvterm screen into an RGB888 pixel frame using the IBM
 * VGA 8x16 glyph set (CP437). Used by the SDL host tool and the
 * automated render test suite.
 */
#ifndef TERM_RENDER_H
#define TERM_RENDER_H

#include <stdbool.h>
#include <stdint.h>

#include "vterm.h"

#define TERM_CELL_W 8
#define TERM_CELL_H 16

typedef struct {
	VTermScreen *screen;
	VTermState *state;
	uint32_t *pixels;   /* RGB888, win_w * win_h, caller-owned */
	int rows;
	int cols;
	int win_w;          /* rows * TERM_CELL_W */
	int win_h;          /* cols * TERM_CELL_H */
	VTermPos cursor;
	bool cursor_visible;
	bool blink_on;      /* external 2 Hz phase, drives cursor blink */
	int cursor_shape;   /* VTERM_PROP_CURSORSHAPE_*: 1 block, 2 underline, 3 bar */
	int cursor_blink;   /* VTERM_PROP_CURSORBLINK from DECSCUSR: 1 blink, 0 steady */
	bool cursor_shape_set; /* true once an app issued DECSCUSR */
	int scroll_offset;  /* 0 = live view; >0 = scrolled back that many lines */
	/* mouse selection: cells between anchor and cur get highlighted */
	bool sel_active;
	VTermPos sel_anchor;
	VTermPos sel_cur;
	int sel_line_end; /* last non-blank col of the row being painted */
	bool sel_block;   /* column-block selection (Alt+drag, xterm style) */
	/* Read a cell from the host scrollback storage (row 0 = most recently
	 * scrolled-out line). Return 1 on success, 0 if out of range. */
	int (*sb_get_cell)(void *user, int row, int col, VTermScreenCell *cell);
	void *sb_user;
} term_renderer_t;

/* Attach a renderer to a libvterm instance. */
void term_render_init(term_renderer_t *r, VTerm *vt, uint32_t *pixels);

/* Paint the whole frame from the current screen state. */
void term_render_frame(term_renderer_t *r);

/* Unicode -> CP437 glyph index (box drawing / blocks / common symbols). */
uint8_t term_unicode_to_cp437(uint32_t cp);

/* Extract the text of the active selection as UTF-8. */
size_t term_render_selected_text(const term_renderer_t *r, char *out, size_t cap);

/* xterm-style word selection: extend the selection to the whitespace-
 * delimited word around (row, col). */
void term_render_select_word(term_renderer_t *r, int row, int col);

/* xterm-style line selection: select the whole line. */
void term_render_select_line(term_renderer_t *r, int row);

#endif
