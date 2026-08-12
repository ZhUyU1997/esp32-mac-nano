/*
 * vterm-sdl — graphical libvterm PTY loopback.
 *
 * libvterm -> IBM VGA 8x16 glyphs (CP437) -> RGB pixels -> SDL window.
 * Grid: 80x30 on 640x480 (XT-era text layout, height extended).
 *
 * Usage:
 *   vterm-sdl                        interactive window (Ctrl+Esc quits)
 *   vterm-sdl -c "cmd" -o shot.bmp  run cmd, screenshot, exit (headless ok)
 *
 * Env: SDL_VIDEODRIVER=dummy for headless runs.
 */
#define _DEFAULT_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <pty.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include <SDL.h>

#include "vterm.h"
#include "term_render.h"

#define CELL_W TERM_CELL_W
#define CELL_H TERM_CELL_H
#define ROWS   30
#define COLS   80

#define WIN_W (COLS * CELL_W)
#define WIN_H (ROWS * CELL_H)

static VTerm *s_vt;
static VTermScreen *s_screen;
static VTermState *s_state;
static term_renderer_t s_renderer;
static bool s_dirty = true;
static SDL_Texture *s_tex;
static uint32_t *s_pixels;
static SDL_Window *s_win;
static SDL_Renderer *s_render_ren;
static bool s_sync_update;  /* DECSET 2026 synchronized update in progress */

static int s_mouse_mode;    /* VTERM_PROP_MOUSE (0 = off) */
static bool s_focus_report; /* VTERM_PROP_FOCUSREPORT */
static bool s_sel_dragging; /* left button held for text selection */
static uint32_t s_click_time;
static int s_click_count;
static VTermPos s_click_pos;
static bool s_sync_update;  /* (reserved) DECSET 2026 sync — libvterm lacks it */

/* ---- scrollback: lines scrolled out of the live screen --------------
 * libvterm hands each scrolled-out line to the host via sb_pushline.
 * We keep a ring of VTermScreenCell rows and let the renderer draw
 * them above the live viewport when scrolled back (mouse wheel). */

#define SB_CAP 1000
#define SB_COLS 256          /* ring row capacity; resize clamps cols <= 256 */

static VTermScreenCell *s_sb;   /* SB_CAP rows x SB_COLS cells */
static int s_sb_cols[SB_CAP];   /* actual width of each stored row */
static int s_sb_count;          /* valid rows stored */
static int s_sb_head;           /* ring index of newest row */

static int on_sb_pushline(int cols, const VTermScreenCell *cells, void *user)
{
	(void)user;
	if (cols > SB_COLS)
		cols = SB_COLS;
	if (s_sb_count < SB_CAP)
		s_sb_count++;
	s_sb_head = (s_sb_head + 1) % SB_CAP;
	s_sb_cols[s_sb_head] = cols;
	memcpy(&s_sb[s_sb_head * SB_COLS], cells, (size_t)cols * sizeof(VTermScreenCell));
	return 1;
}

static int on_sb_popline(int cols, VTermScreenCell *cells, void *user)
{
	(void)user;
	if (s_sb_count == 0)
		return 0;
	/* reflow asks for the oldest scrolled-out row (backfills the top of
	 * the screen after a resize, as neovim does) */
	int old_idx = (s_sb_head - (s_sb_count - 1) + SB_CAP) % SB_CAP;
	int n = s_sb_cols[old_idx] < cols ? s_sb_cols[old_idx] : cols;
	memcpy(cells, &s_sb[old_idx * SB_COLS], (size_t)n * sizeof(VTermScreenCell));
	for (int c = n; c < cols; c++)
		memset(&cells[c], 0, sizeof(VTermScreenCell));
	s_sb_count--;
	return 1;
}

static int on_sb_clear(void *user)
{
	(void)user;
	s_sb_count = 0;
	s_sb_head = 0;
	return 1;
}

/* row 0 = most recently scrolled-out line */
static int sb_get_cell(void *user, int row, int col, VTermScreenCell *cell)
{
	(void)user;
	if (row < 0 || row >= s_sb_count)
		return 0;
	int idx = (s_sb_head - row + SB_CAP) % SB_CAP;
	if (col < 0 || col >= s_sb_cols[idx]) {
		memset(cell, 0, sizeof(*cell)); /* past this row's stored width */
		return 1;
	}
	*cell = s_sb[idx * SB_COLS + col];
	return 1;
}

/* ---- libvterm callbacks ---------------------------------------------- */

static int on_damage(VTermRect rect, void *user)
{
	(void)rect;
	(void)user;
	s_dirty = true;
	return 1;
}

static int on_movecursor(VTermPos pos, VTermPos oldpos, int visible, void *user)
{
	(void)oldpos;
	(void)user;
	s_renderer.cursor = pos;
	s_renderer.cursor_visible = visible != 0;
	s_dirty = true;
	return 1;
}

static int on_bell(void *user)
{
	(void)user;
	/* visual bell: flash the window (xterm flashes instead of beeping
	 * when a visual bell is configured) */
	if (s_win)
		SDL_FlashWindow(s_win, SDL_FLASH_BRIEFLY);
	return 1;
}

static int on_settermprop(VTermProp prop, VTermValue *val, void *user)
{
	(void)user;
	switch (prop) {
	case VTERM_PROP_CURSORVISIBLE:
		/* DECSET/DECRST 25: no damage is emitted for visibility changes,
		 * so redraw explicitly to show/hide the cursor */
		s_renderer.cursor_visible = val->boolean;
		s_dirty = true;
		break;
	case VTERM_PROP_CURSORBLINK:
		s_renderer.cursor_blink = val->boolean;
		s_dirty = true;
		break;
	case VTERM_PROP_CURSORSHAPE:
		s_renderer.cursor_shape = val->number;
		s_renderer.cursor_shape_set = true;
		s_dirty = true;
		break;
	case VTERM_PROP_ALTSCREEN:
		/* fullscreen apps (vim/htop) own the whole screen: scrollback is
		 * not reachable while the alt screen is active (as neovim does) */
		if (val->boolean && s_renderer.scroll_offset > 0) {
			s_renderer.scroll_offset = 0;
			s_dirty = true;
		}
		break;
	case VTERM_PROP_TITLE:
	case VTERM_PROP_ICONNAME:
		if (s_win && val->string.str) {
			char buf[128];
			size_t n = val->string.len < sizeof(buf) - 1 ? val->string.len : sizeof(buf) - 1;
			memcpy(buf, val->string.str, n);
			buf[n] = '\0';
			SDL_SetWindowTitle(s_win, buf);
		}
		break;
	case VTERM_PROP_REVERSE:
		/* libvterm XORs global reverse into cell attrs at get_cell time;
		 * we only need to redraw */
		s_dirty = true;
		break;
	case VTERM_PROP_MOUSE:
		s_mouse_mode = val->number;
		break;
	case VTERM_PROP_FOCUSREPORT:
		s_focus_report = val->boolean;
		break;
	case VTERM_PROP_SYNCOUTPUT:
		/* DECSET 2026: defer rendering until the end of the update so
		 * vim/htop repaints are presented as one frame. The main loop
		 * skips rendering while s_sync_update is set; dirty stays set
		 * so the end of the update commits the whole frame at once. */
		s_sync_update = val->boolean;
		break;
	default:
		break;
	}
	return 1;
}

/* ---- rendering (delegated to term_render.c) --------------------------- */

static void render_frame(void)
{
	term_render_frame(&s_renderer);
	SDL_UpdateTexture(s_tex, NULL, s_pixels, s_renderer.win_w * sizeof(uint32_t));
}

/* ---- pty --------------------------------------------------------------- */

static int s_master = -1;
static bool s_bash_dead;

static void pty_init(void)
{
	/* Tell bash/ncurses the real grid size, otherwise they default to 80x25
	 * and leave the bottom rows blank (htop etc.). */
	struct winsize ws = {
		.ws_row = ROWS,
		.ws_col = COLS,
		.ws_xpixel = 0,
		.ws_ypixel = 0,
	};
	pid_t pid = forkpty(&s_master, NULL, NULL, &ws);
	if (pid == 0) {
		setenv("TERM", "xterm-256color", 1);
		execl("/bin/bash", "bash", "--norc", "--noprofile", (char *)NULL);
		_exit(127);
	}
	if (pid < 0) {
		perror("forkpty");
		exit(1);
	}
	fcntl(s_master, F_SETFL, fcntl(s_master, F_GETFL) | O_NONBLOCK);
}

static void pty_drain(void)
{
	char buf[4096];
	fd_set rfds;
	struct timeval tv = { 0, 0 };
	FD_ZERO(&rfds);
	FD_SET(s_master, &rfds);
	if (select(s_master + 1, &rfds, NULL, NULL, &tv) <= 0)
		return; /* nothing to read yet */
	for (;;) {
		ssize_t n = read(s_master, buf, sizeof(buf));
		if (n <= 0) {
			if (n == 0)
				s_bash_dead = true;
			break;
		}
		vterm_input_write(s_vt, buf, (size_t)n);
		vterm_screen_flush_damage(s_screen);
	}
}

/* ---- keyboard -> libvterm -> output callback -> pty -------------------- */

static void term_output_cb(const char *s, size_t len, void *user)
{
	(void)user;
	if (s_master >= 0)
		write(s_master, s, len);
}

/* any key press while scrolled back returns to the live view */
static void reset_scrollback(void)
{
	if (s_renderer.scroll_offset > 0) {
		s_renderer.scroll_offset = 0;
		s_dirty = true;
	}
}

/* typing invalidates the active selection (xterm behaviour) */
static void clear_selection(void)
{
	if (s_renderer.sel_active) {
		s_renderer.sel_active = false;
		s_dirty = true;
	}
}

/* ---- clipboard paste (Ctrl+Shift+V / Shift+Insert) --------------------- */

static void paste_clipboard(void)
{
	char *clip = SDL_GetClipboardText();
	if (!clip || !clip[0])
		return;
	reset_scrollback();
	vterm_keyboard_start_paste(s_vt);
	const unsigned char *t = (const unsigned char *)clip;
	while (*t) {
		uint32_t cp = *t;
		int len = 1;
		if ((*t & 0xE0) == 0xC0) { cp = *t & 0x1F; len = 2; }
		else if ((*t & 0xF0) == 0xE0) { cp = *t & 0x0F; len = 3; }
		else if ((*t & 0xF8) == 0xF0) { cp = *t & 0x07; len = 4; }
		else if ((*t & 0x80) == 0x80) { t++; continue; } /* stray continuation */
		for (int i = 1; i < len; i++)
			cp = (cp << 6) | (t[i] & 0x3F);
		vterm_keyboard_unichar(s_vt, cp, VTERM_MOD_NONE);
		t += len;
	}
	vterm_keyboard_end_paste(s_vt);
	SDL_free(clip);
}

static void handle_keydown(const SDL_Event *ev)
{
	SDL_Keycode kc = ev->key.keysym.sym;
	Uint16 mod = ev->key.keysym.mod;

	if (kc == SDLK_ESCAPE && (mod & KMOD_CTRL))
		exit(0); /* Ctrl+Esc: quit */

	/* paste: Ctrl+Shift+V or Shift+Insert (Ctrl+V stays a shell key) */
	if (kc == SDLK_v && (mod & KMOD_CTRL) && (mod & KMOD_SHIFT)) {
		clear_selection();
		paste_clipboard();
		return;
	}
	if (kc == SDLK_INSERT && (mod & KMOD_SHIFT)) {
		clear_selection();
		paste_clipboard();
		return;
	}

	/* Ctrl+C with an active selection copies it (modern terminal
	 * behaviour); without a selection it stays the interrupt key */
	if ((mod & KMOD_CTRL) && !(mod & KMOD_SHIFT) && kc == SDLK_c && s_renderer.sel_active) {
		char txt[4096];
		size_t n = term_render_selected_text(&s_renderer, txt, sizeof(txt));
		if (n)
			SDL_SetClipboardText(txt);
		s_renderer.sel_active = false;
		s_dirty = true;
		return;
	}

	/* any other key press drops the selection (content will change) */
	clear_selection();

	VTermModifier vm = VTERM_MOD_NONE;
	if (mod & KMOD_SHIFT) vm |= VTERM_MOD_SHIFT;
	if (mod & KMOD_CTRL) vm |= VTERM_MOD_CTRL;
	if (mod & KMOD_ALT) vm |= VTERM_MOD_ALT;

	VTermKey key = VTERM_KEY_NONE;
	switch (kc) {
	case SDLK_RETURN: case SDLK_KP_ENTER: key = VTERM_KEY_ENTER; break;
	case SDLK_TAB: key = VTERM_KEY_TAB; break;
	case SDLK_BACKSPACE: key = VTERM_KEY_BACKSPACE; break;
	case SDLK_ESCAPE: key = VTERM_KEY_ESCAPE; break;
	case SDLK_UP: key = VTERM_KEY_UP; break;
	case SDLK_DOWN: key = VTERM_KEY_DOWN; break;
	case SDLK_LEFT: key = VTERM_KEY_LEFT; break;
	case SDLK_RIGHT: key = VTERM_KEY_RIGHT; break;
	case SDLK_INSERT: key = VTERM_KEY_INS; break;
	case SDLK_DELETE: key = VTERM_KEY_DEL; break;
	case SDLK_HOME: key = VTERM_KEY_HOME; break;
	case SDLK_END: key = VTERM_KEY_END; break;
	case SDLK_PAGEUP: key = VTERM_KEY_PAGEUP; break;
	case SDLK_PAGEDOWN: key = VTERM_KEY_PAGEDOWN; break;
	case SDLK_KP_0: key = VTERM_KEY_KP_0; break;
	case SDLK_KP_1: key = VTERM_KEY_KP_1; break;
	case SDLK_KP_2: key = VTERM_KEY_KP_2; break;
	case SDLK_KP_3: key = VTERM_KEY_KP_3; break;
	case SDLK_KP_4: key = VTERM_KEY_KP_4; break;
	case SDLK_KP_5: key = VTERM_KEY_KP_5; break;
	case SDLK_KP_6: key = VTERM_KEY_KP_6; break;
	case SDLK_KP_7: key = VTERM_KEY_KP_7; break;
	case SDLK_KP_8: key = VTERM_KEY_KP_8; break;
	case SDLK_KP_9: key = VTERM_KEY_KP_9; break;
	case SDLK_KP_MULTIPLY: key = VTERM_KEY_KP_MULT; break;
	case SDLK_KP_PLUS: key = VTERM_KEY_KP_PLUS; break;
	case SDLK_KP_MINUS: key = VTERM_KEY_KP_MINUS; break;
	case SDLK_KP_PERIOD: key = VTERM_KEY_KP_PERIOD; break;
	case SDLK_KP_DIVIDE: key = VTERM_KEY_KP_DIVIDE; break;
	default:
		if (kc >= SDLK_F1 && kc <= SDLK_F12)
			key = VTERM_KEY_FUNCTION((int)(kc - SDLK_F1) + 1);
		break;
	}

	reset_scrollback();

	if (key != VTERM_KEY_NONE) {
		vterm_keyboard_key(s_vt, key, vm);
		return;
	}

	/* Ctrl+letter -> classic control characters (SDL sends no text input
	 * for these) */
	if ((vm & VTERM_MOD_CTRL) && kc >= SDLK_a && kc <= SDLK_z) {
		vterm_keyboard_unichar(s_vt, (uint32_t)(kc - SDLK_a + 1), VTERM_MOD_NONE);
		return;
	}
	/* letters/digits/symbols arrive via SDL_TEXTINPUT */
}

/* ---- OSC 52 clipboard (base64 over the selection protocol) ------------- */

static const char k_b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static size_t b64_encode(const char *in, size_t inlen, char *out)
{
	size_t o = 0;
	for (size_t i = 0; i < inlen; i += 3) {
		uint32_t v = (uint8_t)in[i] << 16;
		if (i + 1 < inlen) v |= (uint8_t)in[i + 1] << 8;
		if (i + 2 < inlen) v |= (uint8_t)in[i + 2];
		out[o++] = k_b64[(v >> 18) & 63];
		out[o++] = k_b64[(v >> 12) & 63];
		out[o++] = (i + 1 < inlen) ? k_b64[(v >> 6) & 63] : '=';
		out[o++] = (i + 2 < inlen) ? k_b64[v & 63] : '=';
	}
	out[o] = '\0';
	return o;
}

/* OSC 52 clipboard: libvterm decodes the base64 payload before calling
 * set, so the fragment already holds decoded bytes. Fragments may arrive
 * split; accumulate until final. */
static char s_sel_acc[4096];
static size_t s_sel_acc_len;

static int sel_set(VTermSelectionMask mask, VTermStringFragment frag, void *user)
{
	(void)mask;
	(void)user;
	if (frag.initial)
		s_sel_acc_len = 0;
	if (frag.str && s_sel_acc_len + frag.len < sizeof(s_sel_acc)) {
		memcpy(s_sel_acc + s_sel_acc_len, frag.str, frag.len);
		s_sel_acc_len += frag.len;
	}
	if (frag.final) {
		s_sel_acc[s_sel_acc_len] = '\0';
		SDL_SetClipboardText(s_sel_acc);
	}
	return 1;
}

static int sel_query(VTermSelectionMask mask, void *user)
{
	(void)user;
	char *clip = SDL_GetClipboardText();
	if (clip) {
		char enc[4096];
		size_t n = b64_encode(clip, strlen(clip), enc);
		VTermStringFragment frag = {
			.str = enc,
			.len = n,
			.initial = true,
			.final = true,
		};
		vterm_state_send_selection(s_state, mask, frag);
		SDL_free(clip);
	}
	return 1;
}

static const VTermSelectionCallbacks k_sel_cbs = {
	.set = sel_set,
	.query = sel_query,
};

/* ---- terminal resize --------------------------------------------------- */

static void resize_terminal(int win_w, int win_h)
{
	int cols = win_w / CELL_W;
	int rows = win_h / CELL_H;
	if (cols < 20)
		cols = 20;
	if (rows < 5)
		rows = 5;
	if (cols > 256)
		cols = 256; /* scrollback ring is sized for COLS */
	if (rows > 200)
		rows = 200;

	vterm_set_size(s_vt, rows, cols);
	struct winsize ws = { .ws_row = (unsigned short)rows, .ws_col = (unsigned short)cols, 0, 0 };
	ioctl(s_master, TIOCSWINSZ, &ws);

	s_renderer.rows = rows;
	s_renderer.cols = cols;
	s_renderer.win_w = cols * CELL_W;
	s_renderer.win_h = rows * CELL_H;

	uint32_t *np = calloc((size_t)s_renderer.win_w * s_renderer.win_h, sizeof(uint32_t));
	if (!np)
		return; /* keep the old buffer on alloc failure */
	uint32_t *old = s_pixels;
	s_pixels = NULL;
	s_renderer.pixels = NULL;
	free(old);
	s_pixels = np;
	s_renderer.pixels = s_pixels;

	SDL_Texture *nt = SDL_CreateTexture(s_render_ren, SDL_PIXELFORMAT_XRGB8888,
	                                    SDL_TEXTUREACCESS_STREAMING,
	                                    s_renderer.win_w, s_renderer.win_h);
	if (nt) {
		SDL_DestroyTexture(s_tex);
		s_tex = nt;
	}
	s_dirty = true;
}

/* ---- unrecognised OSC/DCS fallbacks ------------------------------------ */

/* OSC 4 ; N ; rgb:RR/GG/BB — redefine a palette entry (xterm colour) */
static void osc4_apply(int idx, const char *s, size_t len)
{
	unsigned r, g, b;
	if (sscanf(s, "rgb:%2x/%2x/%2x", &r, &g, &b) == 3 ||
	    sscanf(s, "#%2x%2x%2x", &r, &g, &b) == 3) {
		VTermColor col;
		vterm_color_rgb(&col, (uint8_t)r, (uint8_t)g, (uint8_t)b);
		vterm_state_set_palette_color(s_state, idx, &col);
	}
	(void)len;
}

static int on_osc_fallback(int command, VTermStringFragment frag, void *user)
{
	(void)user;
	if (command == 4 && frag.str) {
		/* one or more "N;spec" pairs, e.g. "4;1;rgb:ff/00/00" */
		size_t i = 0;
		while (i < frag.len) {
			int idx = 0;
			while (i < frag.len && frag.str[i] >= '0' && frag.str[i] <= '9')
				idx = idx * 10 + (frag.str[i++] - '0');
			if (i < frag.len && frag.str[i] == ';')
				i++;
			size_t start = i;
			while (i < frag.len && frag.str[i] != ';')
				i++;
			if (i > start)
				osc4_apply(idx, frag.str + start, i - start);
			if (i < frag.len && frag.str[i] == ';')
				i++; /* next pair */
		}
	}
	return 0;
}

/* DCS fallback: reserved for future extensions (e.g. if we later add
 * DECSET 2026 synchronized output support to the vendored libvterm). */
static int on_dcs_fallback(const char *command, size_t commandlen, VTermStringFragment frag, void *user)
{
	(void)command;
	(void)commandlen;
	(void)frag;
	(void)user;
	return 1;
}

static const VTermStateFallbacks k_fallbacks = {
	.osc = on_osc_fallback,
	.dcs = on_dcs_fallback,
};

/* ---- main -------------------------------------------------------------- */

int main(int argc, char **argv)
{
	const char *cmd = NULL;
	const char *shot = NULL;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-c") == 0 && i + 1 < argc)
			cmd = argv[++i];
		else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc)
			shot = argv[++i];
	}

	pty_init();

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
		return 1;
	}

	s_win = SDL_CreateWindow("vterm (libvterm + SDL)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIN_W, WIN_H, SDL_WINDOW_RESIZABLE);
	if (!s_win) {
		fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
		return 1;
	}
	SDL_Renderer *ren = SDL_CreateRenderer(s_win, -1, 0);
	s_render_ren = ren;
	if (!ren) {
		fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
		return 1;
	}
	s_tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, WIN_W, WIN_H);
	if (!s_tex) {
		fprintf(stderr, "SDL_CreateTexture: %s\n", SDL_GetError());
		return 1;
	}
	s_pixels = calloc((size_t)WIN_W * WIN_H, sizeof(uint32_t));

	s_vt = vterm_new(ROWS, COLS);
	vterm_set_utf8(s_vt, 1);
	term_render_init(&s_renderer, s_vt, s_pixels);
	s_screen = s_renderer.screen;
	s_state = s_renderer.state;
	vterm_output_set_callback(s_vt, term_output_cb, NULL);
	vterm_state_set_selection_callbacks(s_state, &k_sel_cbs, NULL, s_sel_acc, sizeof(s_sel_acc));
	vterm_state_set_unrecognised_fallbacks(s_state, &k_fallbacks, NULL);

	VTermScreenCallbacks cbs = {
		.damage = on_damage,
		.movecursor = on_movecursor,
		.bell = on_bell,
		.settermprop = on_settermprop,
		.sb_pushline = on_sb_pushline,
		.sb_popline = on_sb_popline,
		.sb_clear = on_sb_clear,
	};
	vterm_screen_set_callbacks(s_screen, &cbs, NULL);
	/* Allocate the alternate screen buffer; without this, DECSET 1049
	 * (vim/htop fullscreen apps) silently no-ops and writes over the
	 * primary screen instead. */
	vterm_screen_enable_altscreen(s_screen, 1);
	/* keep content on resize (xterm reflows the buffer) */
	vterm_screen_enable_reflow(s_screen, true);
	vterm_screen_reset(s_screen, 1);
	vterm_state_get_cursorpos(s_state, &s_renderer.cursor);
	s_renderer.cursor_visible = true;

	/* scrollback ring */
	s_sb = calloc((size_t)SB_CAP * SB_COLS, sizeof(VTermScreenCell));
	if (!s_sb) {
		fprintf(stderr, "scrollback alloc failed\n");
		return 1;
	}
	s_renderer.sb_get_cell = sb_get_cell;
	s_renderer.sb_user = NULL;

	if (cmd) {
		char buf[1024];
		int n = snprintf(buf, sizeof(buf), "%s\r", cmd);
		if (n > 0)
			write(s_master, buf, (size_t)n);
	}

	bool quit = false;
	int frames = 0;
	int last_blink_phase = -1;
	while (!quit) {
		SDL_Event ev;
		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_QUIT:
				quit = true;
				break;
			case SDL_KEYDOWN:
				handle_keydown(&ev);
				break;
			case SDL_MOUSEMOTION:
				if (s_mouse_mode) {
					vterm_mouse_move(s_vt, ev.motion.y / CELL_H, ev.motion.x / CELL_W, VTERM_MOD_NONE);
					s_dirty = true;
				} else if (s_sel_dragging) {
					/* the selection activates on the first drag motion */
					s_renderer.sel_active = true;
					s_renderer.sel_cur.row = ev.motion.y / CELL_H;
					s_renderer.sel_cur.col = ev.motion.x / CELL_W;
					s_dirty = true;
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
				/* drag-select with the left button when no mouse protocol
				 * is active; right-click pastes */
				if (!s_mouse_mode && ev.button.button == SDL_BUTTON_LEFT) {
					int brow = ev.button.y / CELL_H;
					int bcol = ev.button.x / CELL_W;
					uint32_t now = SDL_GetTicks();
					if (now - s_click_time < 400 && s_click_pos.row == brow && s_click_pos.col == bcol) {
						s_click_count++;
					} else {
						s_click_count = 1;
					}
					s_click_time = now;
					s_click_pos.row = brow;
					s_click_pos.col = bcol;
					if (s_click_count == 2) {
						/* double-click: word selection, done on release-less press */
						term_render_select_word(&s_renderer, brow, bcol);
						s_dirty = true;
						break;
					}
					if (s_click_count >= 3) {
						term_render_select_line(&s_renderer, brow);
						s_dirty = true;
						break;
					}
					/* single click: clear any previous selection and arm a
					 * drag; the selection only starts once the mouse moves
					 * (xterm: click alone does not select). Alt+drag is a
					 * column-block selection. */
					s_renderer.sel_active = false;
					s_renderer.sel_block = (SDL_GetModState() & KMOD_ALT) != 0;
					s_renderer.sel_anchor.row = brow;
					s_renderer.sel_anchor.col = bcol;
					s_sel_dragging = true;
					s_dirty = true;
					break;
				}
				if (!s_mouse_mode && ev.button.button == SDL_BUTTON_RIGHT) {
					paste_clipboard();
					break;
				}
				/* middle-click pastes (X11 classic) when no mouse protocol */
				if (!s_mouse_mode && ev.button.button == SDL_BUTTON_MIDDLE) {
					paste_clipboard();
					break;
				}
				/* fallthrough */
			case SDL_MOUSEBUTTONUP:
				if (!s_mouse_mode && ev.button.button == SDL_BUTTON_LEFT && s_sel_dragging) {
					s_sel_dragging = false;
					if (s_renderer.sel_active) {
						/* dragged: copy on release and keep the selection so
						 * Ctrl+C etc. can still act on it */
						char txt[4096];
						size_t n = term_render_selected_text(&s_renderer, txt, sizeof(txt));
						if (n)
							SDL_SetClipboardText(txt);
					}
					/* single click without motion: nothing selected */
					s_dirty = true;
					break;
				}
				if (s_mouse_mode) {
					int b = 0;
					if (ev.button.button == SDL_BUTTON_LEFT) b = 1;
					else if (ev.button.button == SDL_BUTTON_RIGHT) b = 2;
					else if (ev.button.button == SDL_BUTTON_MIDDLE) b = 3;
					if (b) {
						vterm_mouse_button(s_vt, b, ev.type == SDL_MOUSEBUTTONDOWN, VTERM_MOD_NONE);
						s_dirty = true;
					}
				}
				break;
			case SDL_WINDOWEVENT:
				if (ev.window.event == SDL_WINDOWEVENT_FOCUS_GAINED && s_focus_report)
					vterm_state_focus_in(s_state);
				else if (ev.window.event == SDL_WINDOWEVENT_FOCUS_LOST && s_focus_report)
					vterm_state_focus_out(s_state);
				else if (ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
					resize_terminal(ev.window.data1, ev.window.data2);
				break;
			case SDL_MOUSEWHEEL:
				if (s_mouse_mode) {
					/* mouse protocol active (vim set mouse=a): wheel becomes
					 * buttons 4/5, not scrollback scrolling */
					int b = ev.wheel.y > 0 ? 4 : 5;
					VTermModifier vm = VTERM_MOD_NONE;
					SDL_Keymod m = SDL_GetModState();
					if (m & KMOD_SHIFT) vm |= VTERM_MOD_SHIFT;
					if (m & KMOD_CTRL) vm |= VTERM_MOD_CTRL;
					if (m & KMOD_ALT) vm |= VTERM_MOD_ALT;
					vterm_mouse_button(s_vt, b, true, vm);
					vterm_mouse_button(s_vt, b, false, vm);
					s_dirty = true;
				} else if (ev.wheel.y > 0)
					s_renderer.scroll_offset++;
				else if (ev.wheel.y < 0)
					s_renderer.scroll_offset--;
				if (!s_mouse_mode && s_renderer.scroll_offset < 0)
					s_renderer.scroll_offset = 0;
				if (s_renderer.scroll_offset > s_sb_count)
					s_renderer.scroll_offset = s_sb_count;
				s_dirty = true;
				break;
			case SDL_TEXTINPUT:
				if (ev.text.text[0]) {
					/* decode first UTF-8 code point, pass through libvterm
					 * (handles Alt prefix, Ctrl combos, CSI u) */
					const unsigned char *t = (const unsigned char *)ev.text.text;
					uint32_t cp = *t;
					int len = 1;
					if ((*t & 0xE0) == 0xC0) { cp = *t & 0x1F; len = 2; }
					else if ((*t & 0xF0) == 0xE0) { cp = *t & 0x0F; len = 3; }
					else if ((*t & 0xF8) == 0xF0) { cp = *t & 0x07; len = 4; }
					for (int i = 1; i < len; i++)
						cp = (cp << 6) | (t[i] & 0x3F);
					VTermModifier vm = VTERM_MOD_NONE;
					SDL_Keymod m = SDL_GetModState();
					if (m & KMOD_SHIFT) vm |= VTERM_MOD_SHIFT;
					if (m & KMOD_ALT) vm |= VTERM_MOD_ALT;
					reset_scrollback();
					clear_selection();
					vterm_keyboard_unichar(s_vt, cp, vm);
				}
				break;
			default:
				break;
			}
		}

		pty_drain();

		/* cursor blink needs periodic redraws even when no pty data arrives */
		int blink_phase = (SDL_GetTicks() / 500) % 2;
		s_renderer.blink_on = (blink_phase == 0);
		if (blink_phase != last_blink_phase) {
			last_blink_phase = blink_phase;
			s_dirty = true;
		}

		if (s_bash_dead) {
			/* keep rendering the last frame, allow screenshot */
			if (shot && frames > 30)
				break;
		}

		if (s_dirty && !s_sync_update) {
			s_dirty = false;
			render_frame();
			SDL_RenderClear(ren);
			SDL_RenderCopy(ren, s_tex, NULL, NULL);
			SDL_RenderPresent(ren);
		}

		if (shot && ++frames > 900)
			break; /* enough frames for the command output */
		SDL_Delay(8);
	}

	if (shot) {
		SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(0, WIN_W, WIN_H, 32, SDL_PIXELFORMAT_RGB888);
		if (surf && SDL_RenderReadPixels(ren, NULL, SDL_PIXELFORMAT_XRGB8888, surf->pixels, surf->pitch) == 0) {
			if (SDL_SaveBMP(surf, shot) != 0)
				fprintf(stderr, "SDL_SaveBMP: %s\n", SDL_GetError());
			fprintf(stderr, "saved %s\n", shot);
		}
		if (surf)
			SDL_FreeSurface(surf);
	}

	SDL_DestroyTexture(s_tex);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(s_win);
	SDL_Quit();
	return 0;
}
