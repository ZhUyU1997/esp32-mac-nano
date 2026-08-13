/*
 * vterm_hid_map.c — input keycode -> VT100 byte sequence.
 *
 * Consumes input_pop() events: modifier keys update a tracked state,
 * character keys produce the VT100 bytes (plain, Shift, Ctrl, Alt).
 * Modifier bit order matches input_report_keyboard: bit0 LCTRL, bit1
 * LSHIFT, bit2 LALT, bit4 RCTRL, bit5 RSHIFT, bit6 RALT.
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "input.h"

#define MOD_CTRL  ((1u << 0) | (1u << 4))
#define MOD_SHIFT ((1u << 1) | (1u << 5))
#define MOD_ALT   ((1u << 2) | (1u << 6))

static uint8_t s_mods;

static bool is_modifier(input_keycode_t code)
{
	return code == INPUT_KEY_LEFTCTRL || code == INPUT_KEY_RIGHTCTRL ||
	       code == INPUT_KEY_LEFTSHIFT || code == INPUT_KEY_RIGHTSHIFT ||
	       code == INPUT_KEY_LEFTALT || code == INPUT_KEY_RIGHTALT ||
	       code == INPUT_KEY_LEFTMETA || code == INPUT_KEY_RIGHTMETA;
}

static uint8_t mod_bit_for(input_keycode_t code)
{
	switch (code) {
	case INPUT_KEY_LEFTCTRL:  return 1u << 0;
	case INPUT_KEY_LEFTSHIFT: return 1u << 1;
	case INPUT_KEY_LEFTALT:   return 1u << 2;
	case INPUT_KEY_LEFTMETA:  return 1u << 3;
	case INPUT_KEY_RIGHTCTRL: return 1u << 4;
	case INPUT_KEY_RIGHTSHIFT: return 1u << 5;
	case INPUT_KEY_RIGHTALT:  return 1u << 6;
	case INPUT_KEY_RIGHTMETA: return 1u << 7;
	default:                  return 0;
	}
}

/* shift pair for the number row and punctuation */
static char shift_of(char c)
{
	switch (c) {
	case '1': return '!'; case '2': return '@'; case '3': return '#';
	case '4': return '$'; case '5': return '%'; case '6': return '^';
	case '7': return '&'; case '8': return '*'; case '9': return '(';
	case '0': return ')'; case '-': return '_'; case '=': return '+';
	case '[': return '{'; case ']': return '}'; case '\\': return '|';
	case ';': return ':'; case '\'': return '"'; case '`': return '~';
	case ',': return '<'; case '.': return '>'; case '/': return '?';
	default:  return c;
	}
}

/* feed a modifier key event (press or release); returns true if consumed */
bool vterm_hid_mod_event(input_keycode_t code, uint8_t value)
{
	if (!is_modifier(code))
		return false;
	uint8_t b = mod_bit_for(code);
	if (value)
		s_mods |= b;
	else
		s_mods &= (uint8_t)~b;
	return true;
}

/* true while either Shift key is held (used for Shift+PgUp/PgDn scrollback) */
bool vterm_hid_shift_down(void)
{
	return (s_mods & MOD_SHIFT) != 0;
}

/* map a character key press to VT100 bytes; returns length (0 = none) */
size_t vterm_hid_map(input_keycode_t code, char *out, size_t out_sz)
{
	bool ctrl = (s_mods & MOD_CTRL) != 0;
	bool shift = (s_mods & MOD_SHIFT) != 0;
	bool alt = (s_mods & MOD_ALT) != 0;

	char c = 0;

	/* numeric keypad: always emit the digit/symbol, Shift does not apply */
	if (code >= INPUT_KEY_KP_SLASH && code <= INPUT_KEY_KP_DOT) {
		if (code >= INPUT_KEY_KP_1 && code <= INPUT_KEY_KP_9)
			c = (char)('1' + (code - INPUT_KEY_KP_1));
		else switch (code) {
		case INPUT_KEY_KP_0:        c = '0'; break;
		case INPUT_KEY_KP_DOT:      c = '.'; break;
		case INPUT_KEY_KP_SLASH:    c = '/'; break;
		case INPUT_KEY_KP_ASTERISK: c = '*'; break;
		case INPUT_KEY_KP_MINUS:    c = '-'; break;
		case INPUT_KEY_KP_PLUS:     c = '+'; break;
		case INPUT_KEY_KP_ENTER:    c = '\r'; break;
		default: break;
		}
		if (c) {
			if (out_sz < 1)
				return 0;
			out[0] = c;
			return 1;
		}
	}

	if (code >= INPUT_KEY_A && code <= INPUT_KEY_Z) {
		c = (char)('a' + (code - INPUT_KEY_A));
		if (ctrl) {
			if (out_sz < 1) return 0;
			out[0] = (char)(c - 'a' + 1); /* Ctrl+A = 0x01 */
			return 1;
		}
		if (shift) c = (char)(c - 'a' + 'A');
	} else if (code >= INPUT_KEY_1 && code <= INPUT_KEY_0) {
		c = (char)('0' + (code - INPUT_KEY_1 + 1) % 10);
		if (ctrl) { /* Ctrl+digit -> ~xterm unused, plain digit */
			if (out_sz < 1) return 0;
			out[0] = c;
			return 1;
		}
		if (shift) c = shift_of(c);
	} else {
		switch (code) {
		case INPUT_KEY_SPACE:      c = ' '; break;
		case INPUT_KEY_MINUS:      c = '-'; break;
		case INPUT_KEY_EQUAL:      c = '='; break;
		case INPUT_KEY_LEFTBRACE:  c = '['; break;
		case INPUT_KEY_RIGHTBRACE: c = ']'; break;
		case INPUT_KEY_BACKSLASH:  c = '\\'; break;
		case INPUT_KEY_SEMICOLON:  c = ';'; break;
		case INPUT_KEY_APOSTROPHE: c = '\''; break;
		case INPUT_KEY_GRAVE:      c = '`'; break;
		case INPUT_KEY_COMMA:      c = ','; break;
		case INPUT_KEY_DOT:        c = '.'; break;
		case INPUT_KEY_SLASH:      c = '/'; break;
		default: break;
		}
		if (c && shift)
			c = shift_of(c);
	}

	if (c) {
		if (alt && out_sz >= 2) { /* Alt+x -> ESC x */
			out[0] = '\x1b';
			out[1] = c;
			return 2;
		}
		if (out_sz < 1) return 0;
		out[0] = c;
		return 1;
	}

	/* control keys */
	if (out_sz < 4) return 0;
	switch (code) {
	case INPUT_KEY_ENTER:      out[0] = '\r'; return 1;
	case INPUT_KEY_BACKSPACE:  out[0] = '\x7f'; return 1;
	case INPUT_KEY_TAB:        out[0] = '\t'; return 1;
	case INPUT_KEY_ESC:        out[0] = '\x1b'; return 1;
	case INPUT_KEY_UP:
		if (ctrl) { memcpy(out, "\x1b[1;5A", 6); return 6; }
		memcpy(out, "\x1b[A", 3); return 3;
	case INPUT_KEY_DOWN:
		if (ctrl) { memcpy(out, "\x1b[1;5B", 6); return 6; }
		memcpy(out, "\x1b[B", 3); return 3;
	case INPUT_KEY_RIGHT:
		if (ctrl) { memcpy(out, "\x1b[1;5C", 6); return 6; }
		memcpy(out, "\x1b[C", 3); return 3;
	case INPUT_KEY_LEFT:
		if (ctrl) { memcpy(out, "\x1b[1;5D", 6); return 6; }
		memcpy(out, "\x1b[D", 3); return 3;
	case INPUT_KEY_HOME:       memcpy(out, "\x1b[H", 3); return 3;
	case INPUT_KEY_END:        memcpy(out, "\x1b[F", 3); return 3;
	case INPUT_KEY_PAGEUP:     memcpy(out, "\x1b[5~", 4); return 4;
	case INPUT_KEY_PAGEDOWN:   memcpy(out, "\x1b[6~", 4); return 4;
	case INPUT_KEY_INSERT:     memcpy(out, "\x1b[2~", 4); return 4;
	case INPUT_KEY_DELETE:     memcpy(out, "\x1b[3~", 4); return 4;
	case INPUT_KEY_F1:  memcpy(out, "\x1bOP", 3); return 3;
	case INPUT_KEY_F2:  memcpy(out, "\x1bOQ", 3); return 3;
	case INPUT_KEY_F3:  memcpy(out, "\x1bOR", 3); return 3;
	case INPUT_KEY_F4:  memcpy(out, "\x1bOS", 3); return 3;
	case INPUT_KEY_F5:  memcpy(out, "\x1b[15~", 5); return 5;
	case INPUT_KEY_F6:  memcpy(out, "\x1b[17~", 5); return 5;
	case INPUT_KEY_F7:  memcpy(out, "\x1b[18~", 5); return 5;
	case INPUT_KEY_F8:  memcpy(out, "\x1b[19~", 5); return 5;
	case INPUT_KEY_F9:  memcpy(out, "\x1b[20~", 5); return 5;
	case INPUT_KEY_F10: memcpy(out, "\x1b[21~", 5); return 5;
	case INPUT_KEY_F11: memcpy(out, "\x1b[23~", 5); return 5;
	case INPUT_KEY_F12: memcpy(out, "\x1b[24~", 5); return 5;
	default:            return 0;
	}
}
