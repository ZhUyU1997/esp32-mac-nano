/*
 * vterm-pty — libvterm PTY loopback verification tool.
 *
 * Runs a real bash inside a libvterm screen and renders the result to
 * stdout. Host-side only (x64), used to validate the terminal core
 * against a real shell before any ESP32 wiring.
 *
 * Usage:
 *   vterm-pty                     interactive (raw stdin -> pty)
 *   vterm-pty -c "cmd"            run cmd, dump plain-text screen, exit
 *
 * Interactive keys: Ctrl+] exits.
 */
#define _DEFAULT_SOURCE
#include <errno.h>
#include <pty.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include "vterm.h"

#define TERM_ROWS 40
#define TERM_COLS 80

static VTerm *s_vt;
static VTermScreen *s_screen;
static bool s_dirty = true;
static VTermPos s_cursor = { 0, 0 };
static bool s_cursor_visible = false;

/* ---- callbacks -------------------------------------------------------- */

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
	s_cursor = pos;
	s_cursor_visible = visible != 0;
	return 1;
}

/* ---- rendering -------------------------------------------------------- */

static int utf8_encode(uint32_t cp, char *buf)
{
	if (cp < 0x80) {
		buf[0] = (char)cp;
		return 1;
	}
	if (cp < 0x800) {
		buf[0] = (char)(0xC0 | (cp >> 6));
		buf[1] = (char)(0x80 | (cp & 0x3F));
		return 2;
	}
	if (cp < 0x10000) {
		buf[0] = (char)(0xE0 | (cp >> 12));
		buf[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
		buf[2] = (char)(0x80 | (cp & 0x3F));
		return 3;
	}
	buf[0] = (char)(0xF0 | (cp >> 18));
	buf[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
	buf[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
	buf[3] = (char)(0x80 | (cp & 0x3F));
	return 4;
}

/* Current SGR state for minimal output */
typedef struct {
	int attrs;   /* bit1 bold, bit2 underline, bit3 blink, bit4 reverse */
	int fg_mode; /* 0 none, 1 ansi16, 2 index256, 3 rgb */
	int fg0, fg1, fg2;
	int bg_mode;
	int bg0, bg1, bg2;
} sgr_t;

static void sgr_color(int mode, int *v, int code, char *out, size_t cap, size_t *off, bool bg)
{
	int n;
	if (mode == 1) {
		n = snprintf(out + *off, cap - *off, bg ? ";%d" : ";%d", code < 8 ? (bg ? 40 + code : 30 + code) : (bg ? 100 + code - 8 : 90 + code - 8));
	} else if (mode == 2) {
		n = snprintf(out + *off, cap - *off, bg ? ";48;5;%d" : ";38;5;%d", code);
	} else if (mode == 3) {
		n = snprintf(out + *off, cap - *off, bg ? ";48;2;%d;%d;%d" : ";38;2;%d;%d;%d", v[0], v[1], v[2]);
	} else {
		return;
	}
	if (n > 0)
		*off += (size_t)n;
}

static void sgr_emit(char *out, size_t cap, size_t *off, sgr_t *st, const VTermScreenCell *cell, bool cursor_here)
{
	/* desired state */
	int attrs = 0;
	if (cell->attrs.bold)     attrs |= 1;
	if (cell->attrs.underline) attrs |= 2;
	if (cell->attrs.blink)    attrs |= 4;
	if (cell->attrs.reverse)  attrs |= 8;
	if (cursor_here)          attrs |= 8; /* reverse video for cursor */

	int fg_mode = 0, fg_code = 0, fgv[3] = { 0, 0, 0 };
	int bg_mode = 0, bg_code = 0, bgv[3] = { 0, 0, 0 };

	if (!(cell->fg.type & VTERM_COLOR_DEFAULT_MASK)) {
		if (VTERM_COLOR_IS_INDEXED(&cell->fg)) {
			fg_code = cell->fg.indexed.idx;
			fg_mode = (fg_code < 16) ? 1 : 2;
		} else {
			fg_mode = 3;
			fgv[0] = cell->fg.rgb.red; fgv[1] = cell->fg.rgb.green; fgv[2] = cell->fg.rgb.blue;
		}
	}
	if (!(cell->bg.type & VTERM_COLOR_DEFAULT_MASK)) {
		if (VTERM_COLOR_IS_INDEXED(&cell->bg)) {
			bg_code = cell->bg.indexed.idx;
			bg_mode = (bg_code < 16) ? 1 : 2;
		} else {
			bg_mode = 3;
			bgv[0] = cell->bg.rgb.red; bgv[1] = cell->bg.rgb.green; bgv[2] = cell->bg.rgb.blue;
		}
	}

	if (attrs == st->attrs && fg_mode == st->fg_mode && bg_mode == st->bg_mode &&
	    (fg_mode != 1 || fg_code == st->fg0) && (bg_mode != 1 || bg_code == st->bg0) &&
	    (fg_mode != 3 || (fgv[0] == st->fg0 && fgv[1] == st->fg1 && fgv[2] == st->fg2)) &&
	    (bg_mode != 3 || (bgv[0] == st->bg0 && bgv[1] == st->bg1 && bgv[2] == st->bg2))) {
		return; /* unchanged */
	}

	st->attrs = attrs; st->fg_mode = fg_mode; st->bg_mode = bg_mode;
	st->fg0 = fgv[0]; st->fg1 = fgv[1]; st->fg2 = fgv[2];
	st->bg0 = bgv[0]; st->bg1 = bgv[1]; st->bg2 = bgv[2];

	int n = snprintf(out + *off, cap - *off, "\033[0m");
	if (n > 0)
		*off += (size_t)n;
	if (attrs & 1) { n = snprintf(out + *off, cap - *off, ";1"); if (n > 0) *off += (size_t)n; }
	if (attrs & 2) { n = snprintf(out + *off, cap - *off, ";4"); if (n > 0) *off += (size_t)n; }
	if (attrs & 4) { n = snprintf(out + *off, cap - *off, ";5"); if (n > 0) *off += (size_t)n; }
	if (attrs & 8) { n = snprintf(out + *off, cap - *off, ";7"); if (n > 0) *off += (size_t)n; }
	sgr_color(fg_mode, fgv, fg_code, out, cap, off, false);
	sgr_color(bg_mode, bgv, bg_code, out, cap, off, true);
	n = snprintf(out + *off, cap - *off, "m");
	if (n > 0)
		*off += (size_t)n;
}

static void redraw_ansi(void)
{
	char buf[16384];
	size_t off = 0;

	int n = snprintf(buf + off, sizeof(buf) - off, "\033[H");
	if (n > 0)
		off += (size_t)n;

	for (int r = 0; r < TERM_ROWS; r++) {
		/* find last non-blank column to avoid painting trailing bg */
		int last = -1;
		for (int c = 0; c < TERM_COLS; c++) {
			VTermScreenCell cell;
			VTermPos pos = { .row = r, .col = c };
			if (vterm_screen_get_cell(s_screen, pos, &cell) && cell.chars[0] != 0 && cell.chars[0] != ' ')
				last = c;
		}

		sgr_t st = { 0 };
		for (int c = 0; c <= last; c++) {
			VTermScreenCell cell;
			VTermPos pos = { .row = r, .col = c };
			if (!vterm_screen_get_cell(s_screen, pos, &cell) || cell.width == 0)
				continue;

			bool cursor_here = s_cursor_visible && r == s_cursor.row && c == s_cursor.col;
			sgr_emit(buf, sizeof(buf), &off, &st, &cell, cursor_here);

			if (cell.chars[0] != 0 && (c == last || cell.width == 1)) {
				char u8[5];
				int nb = utf8_encode(cell.chars[0], u8);
				if (off + (size_t)nb + 32 >= sizeof(buf)) {
					fwrite(buf, 1, off, stdout);
					off = 0;
				}
				memcpy(buf + off, u8, (size_t)nb);
				off += (size_t)nb;
			}
			if (cell.width == 2)
				c++; /* skip continuation cell */
		}

		n = snprintf(buf + off, sizeof(buf) - off, "\033[0m\033[K\r\n");
		if (n > 0)
			off += (size_t)n;
	}

	fwrite(buf, 1, off, stdout);
	fflush(stdout);
}

static void dump_text(void)
{
	for (int r = 0; r < TERM_ROWS; r++) {
		VTermRect rect = { .start_row = r, .end_row = r + 1, .start_col = 0, .end_col = TERM_COLS };
		char line[TERM_COLS * 4 + 1];
		size_t n = vterm_screen_get_text(s_screen, line, sizeof(line), rect);
		while (n > 0 && (line[n - 1] == ' ' || line[n - 1] == '\0'))
			n--;
		line[n] = '\0';
		printf("%s\n", line);
	}
	fflush(stdout);
}

/* ---- auto mode: run one command, wait for quiet, dump text ------------- */

static void run_auto(const char *cmd, int master)
{
	char buf[1024];
	int n = snprintf(buf, sizeof(buf), "%s\r", cmd);
	if (n > 0 && write(master, buf, (size_t)n) < 0)
		perror("write");

	/* wait until the pty stays quiet for 300ms */
	int quiet = 0;
	while (quiet < 300) {
		fd_set rfds;
		FD_ZERO(&rfds);
		FD_SET(master, &rfds);
		struct timeval tv = { .tv_sec = 0, .tv_usec = 50000 };
		int r = select(master + 1, &rfds, NULL, NULL, &tv);
		if (r > 0 && FD_ISSET(master, &rfds)) {
			char data[4096];
			ssize_t got = read(master, data, sizeof(data));
			if (got <= 0)
				break;
			vterm_input_write(s_vt, data, (size_t)got);
			vterm_screen_flush_damage(s_screen);
			quiet = 0;
		} else if (r == 0) {
			quiet += 50;
		} else if (errno != EINTR) {
			break;
		}
	}
	dump_text();
}

/* ---- main -------------------------------------------------------------- */

int main(int argc, char **argv)
{
	bool auto_mode = false;
	const char *cmd = NULL;
	if (argc >= 3 && strcmp(argv[1], "-c") == 0) {
		auto_mode = true;
		cmd = argv[2];
	}

	int master;
	pid_t pid = forkpty(&master, NULL, NULL, NULL);
	if (pid == 0) {
		/* child: exec bash inside the pty */
		setenv("TERM", "xterm-256color", 1);
		execl("/bin/bash", "bash", "--norc", "--noprofile", (char *)NULL);
		_exit(127);
	}
	if (pid < 0) {
		perror("forkpty");
		return 1;
	}

	s_vt = vterm_new(TERM_ROWS, TERM_COLS);
	vterm_set_utf8(s_vt, 1);
	s_screen = vterm_obtain_screen(s_vt);

	VTermScreenCallbacks cbs = {
		.damage = on_damage,
		.movecursor = on_movecursor,
	};
	vterm_screen_set_callbacks(s_screen, &cbs, NULL);
	vterm_screen_reset(s_screen, 1);

	if (auto_mode) {
		run_auto(cmd, master);
		return 0;
	}

	/* interactive: raw stdin passthrough to the pty */
	struct termios orig, raw;
	tcgetattr(STDIN_FILENO, &orig);
	raw = orig;
	cfmakeraw(&raw);
	tcsetattr(STDIN_FILENO, TCSANOW, &raw);

	printf("\033[2J");
	fflush(stdout);

	while (1) {
		fd_set rfds;
		FD_ZERO(&rfds);
		FD_SET(master, &rfds);
		FD_SET(STDIN_FILENO, &rfds);
		int nf = select(master > STDIN_FILENO ? master + 1 : STDIN_FILENO + 1, &rfds, NULL, NULL, NULL);
		if (nf < 0) {
			if (errno == EINTR)
				continue;
			break;
		}

		if (FD_ISSET(master, &rfds)) {
			char data[4096];
			ssize_t got = read(master, data, sizeof(data));
			if (got <= 0)
				break; /* shell exited */
			vterm_input_write(s_vt, data, (size_t)got);
			vterm_screen_flush_damage(s_screen);
			if (s_dirty) {
				s_dirty = false;
				redraw_ansi();
			}
		}

		if (FD_ISSET(STDIN_FILENO, &rfds)) {
			char ch;
			ssize_t got = read(STDIN_FILENO, &ch, 1);
			if (got <= 0)
				break;
			if (ch == 0x1d)
				break; /* Ctrl+] quit */
			if (write(master, &ch, 1) < 0)
				break;
		}
	}

	tcsetattr(STDIN_FILENO, TCSANOW, &orig);
	printf("\033[0m\r\n[pty exited]\r\n");
	return 0;
}
