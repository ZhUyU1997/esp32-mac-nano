/*
 * Mac: drain generic input queue on emu thread — HID→pce_key_t here, then mac_kbd_set_key / mac_set_mouse.
 */

#include "mac_hid_bridge.h"
#include "macplus.h"
#include "input.h"
#include "pce_keys.h"

static const pce_key_t hid_letter_to_pce[26] = {
        PCE_KEY_A, PCE_KEY_B, PCE_KEY_C, PCE_KEY_D, PCE_KEY_E, PCE_KEY_F, PCE_KEY_G, PCE_KEY_H, PCE_KEY_I, PCE_KEY_J, PCE_KEY_K, PCE_KEY_L, PCE_KEY_M,
        PCE_KEY_N, PCE_KEY_O, PCE_KEY_P, PCE_KEY_Q, PCE_KEY_R, PCE_KEY_S, PCE_KEY_T, PCE_KEY_U, PCE_KEY_V, PCE_KEY_W, PCE_KEY_X, PCE_KEY_Y, PCE_KEY_Z,
};

static pce_key_t input_key_to_pce(input_keycode_t k)
{
	if (k >= INPUT_KEY_A && k <= INPUT_KEY_Z)
		return hid_letter_to_pce[k - INPUT_KEY_A];
	if (k >= INPUT_KEY_1 && k <= INPUT_KEY_9)
		return (pce_key_t)(PCE_KEY_1 + (k - INPUT_KEY_1));
	if (k == INPUT_KEY_0)
		return PCE_KEY_0;
	switch (k) {
	case INPUT_KEY_ENTER:
		return PCE_KEY_RETURN;
	case INPUT_KEY_ESC:
		return PCE_KEY_ESC;
	case INPUT_KEY_BACKSPACE:
		return PCE_KEY_BACKSPACE;
	case INPUT_KEY_TAB:
		return PCE_KEY_TAB;
	case INPUT_KEY_SPACE:
		return PCE_KEY_SPACE;
	case INPUT_KEY_MINUS:
		return PCE_KEY_MINUS;
	case INPUT_KEY_EQUAL:
		return PCE_KEY_EQUAL;
	case INPUT_KEY_LEFTBRACE:
		return PCE_KEY_LBRACKET;
	case INPUT_KEY_RIGHTBRACE:
		return PCE_KEY_RBRACKET;
	case INPUT_KEY_BACKSLASH:
		return PCE_KEY_BACKSLASH;
	case INPUT_KEY_SEMICOLON:
		return PCE_KEY_SEMICOLON;
	case INPUT_KEY_APOSTROPHE:
		return PCE_KEY_QUOTE;
	case INPUT_KEY_GRAVE:
		return PCE_KEY_BACKQUOTE;
	case INPUT_KEY_COMMA:
		return PCE_KEY_COMMA;
	case INPUT_KEY_DOT:
		return PCE_KEY_PERIOD;
	case INPUT_KEY_SLASH:
		return PCE_KEY_SLASH;
	case INPUT_KEY_CAPSLOCK:
		return PCE_KEY_CAPSLOCK;
	case INPUT_KEY_NUMLOCK:
		return PCE_KEY_NUMLOCK;
	case INPUT_KEY_KP_SLASH:
		return PCE_KEY_KP_SLASH;
	case INPUT_KEY_KP_ASTERISK:
		return PCE_KEY_KP_STAR;
	case INPUT_KEY_KP_MINUS:
		return PCE_KEY_KP_MINUS;
	case INPUT_KEY_KP_PLUS:
		return PCE_KEY_KP_PLUS;
	case INPUT_KEY_KP_ENTER:
		return PCE_KEY_KP_ENTER;
	case INPUT_KEY_KP_1:
		return PCE_KEY_KP_1;
	case INPUT_KEY_KP_2:
		return PCE_KEY_KP_2;
	case INPUT_KEY_KP_3:
		return PCE_KEY_KP_3;
	case INPUT_KEY_KP_4:
		return PCE_KEY_KP_4;
	case INPUT_KEY_KP_5:
		return PCE_KEY_KP_5;
	case INPUT_KEY_KP_6:
		return PCE_KEY_KP_6;
	case INPUT_KEY_KP_7:
		return PCE_KEY_KP_7;
	case INPUT_KEY_KP_8:
		return PCE_KEY_KP_8;
	case INPUT_KEY_KP_9:
		return PCE_KEY_KP_9;
	case INPUT_KEY_KP_0:
		return PCE_KEY_KP_0;
	case INPUT_KEY_KP_DOT:
		return PCE_KEY_KP_PERIOD;
	case INPUT_KEY_INSERT:
		return PCE_KEY_INS;
	case INPUT_KEY_HOME:
		return PCE_KEY_HOME;
	case INPUT_KEY_PAGEUP:
		return PCE_KEY_PAGEUP;
	case INPUT_KEY_DELETE:
		return PCE_KEY_DEL;
	case INPUT_KEY_END:
		return PCE_KEY_END;
	case INPUT_KEY_PAGEDOWN:
		return PCE_KEY_PAGEDN;
	case INPUT_KEY_RIGHT:
		return PCE_KEY_RIGHT;
	case INPUT_KEY_LEFT:
		return PCE_KEY_LEFT;
	case INPUT_KEY_DOWN:
		return PCE_KEY_DOWN;
	case INPUT_KEY_UP:
		return PCE_KEY_UP;
	case INPUT_KEY_LEFTCTRL:
		return PCE_KEY_NONE;
	case INPUT_KEY_LEFTSHIFT:
		return PCE_KEY_LSHIFT;
	case INPUT_KEY_LEFTALT:
		return PCE_KEY_LCTRL;
	case INPUT_KEY_LEFTMETA:
		return PCE_KEY_LALT;
	case INPUT_KEY_RIGHTCTRL:
		return PCE_KEY_NONE;
	case INPUT_KEY_RIGHTSHIFT:
		return PCE_KEY_RSHIFT;
	case INPUT_KEY_RIGHTALT:
		return PCE_KEY_LCTRL;
	case INPUT_KEY_RIGHTMETA:
		return PCE_KEY_RALT;
	default:
		return PCE_KEY_NONE;
	}
}

void macplus_input_poll(macplus_t *s)
{
	input_evt_t e;

	if (s == NULL)
		return;

	while (input_pop(&e)) {
		switch (e.kind) {
		case INPUT_EVT_KEY: {
			/* Brightness: 3-way switch sides (F4/F5) and standard keys. */
			if (e.u.key.value != 0 && s->backlight_adjust &&
			    (e.u.key.code == INPUT_KEY_F4 || e.u.key.code == INPUT_KEY_BRIGHTNESS_DOWN)) {
				s->backlight_adjust(s->backlight_adjust_ctx, -10);
				break;
			}
			if (e.u.key.value != 0 && s->backlight_adjust &&
			    (e.u.key.code == INPUT_KEY_F5 || e.u.key.code == INPUT_KEY_BRIGHTNESS_UP)) {
				s->backlight_adjust(s->backlight_adjust_ctx, 10);
				break;
			}
			if (e.u.key.code == INPUT_KEY_F12 && e.u.key.value != 0) {
				mac_set_pause(s, !mac_get_pause(s));
				break;
			}
			/* Volume keys (system keys, via the arch volume_adjust callback) */
			if (e.u.key.value != 0 && s->volume_adjust) {
				if (e.u.key.code == INPUT_KEY_VOLUME_DOWN) {
					s->volume_adjust(s->volume_adjust_ctx, -1);
					break;
				}
				if (e.u.key.code == INPUT_KEY_VOLUME_UP) {
					s->volume_adjust(s->volume_adjust_ctx, 1);
					break;
				}
				if (e.u.key.code == INPUT_KEY_VOLUME_MUTE) {
					s->volume_adjust(s->volume_adjust_ctx, -1000);
					break;
				}
			}
			pce_key_t pk = input_key_to_pce(e.u.key.code);
			if (pk != PCE_KEY_NONE) {
				unsigned ev = (e.u.key.value == 0) ? PCE_KEY_EVENT_UP : PCE_KEY_EVENT_DOWN;
				mac_set_key(s, ev, pk);
			}
			break;
		}
		case INPUT_EVT_MOUSE_MOVE_REL: {
			unsigned left_down = (s->mouse_button != 0u) ? 1u : 0u;
			mac_set_mouse(s, (int)e.u.mouse_move_rel.dx, (int)e.u.mouse_move_rel.dy, left_down);
			break;
		}
		case INPUT_EVT_MOUSE_MOVE_ABS: {
			unsigned left_down = (s->mouse_button != 0u) ? 1u : 0u;
			mac_set_mouse_abs(s, e.u.mouse_move_abs.x, e.u.mouse_move_abs.y, left_down);
			break;
		}
		case INPUT_EVT_MOUSE_DOWN:
			if (e.u.mouse_button.button == INPUT_MOUSE_BTN_LEFT) {
				mac_set_mouse(s, 0, 0, 1u);
			}
			break;
		case INPUT_EVT_MOUSE_UP:
			if (e.u.mouse_button.button == INPUT_MOUSE_BTN_LEFT) {
				mac_set_mouse(s, 0, 0, 0u);
			} else if (e.u.mouse_button.button == INPUT_MOUSE_BTN_MIDDLE) {
				mac_set_pause(s, !mac_get_pause(s));
			}
			break;
		}
	}
}
