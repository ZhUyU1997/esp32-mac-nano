/*
 * Generic input → ring queue. No Mac / pce_key_t; consumers translate on emu thread.
 */

#include "input.h"

#include <stddef.h>
#include <string.h>

#include <asm/mutex.h>

#define INPUT_Q 128

static input_evt_t q[INPUT_Q];
static volatile uint16_t q_head;
static volatile uint16_t q_tail;
static asm_mutex_t input_mutex = ASM_MUTEX_INITIALIZER;
static uint8_t mouse_btn_state;
static uint8_t kbd_prev_mod;
static uint8_t kbd_prev_keys[6];

/* HID Keyboard Usage (0x07) → input_keycode_t */
static input_keycode_t hid_kbd_to_input(uint8_t hid)
{
	switch (hid) {
	case 0x04: return INPUT_KEY_A;      case 0x05: return INPUT_KEY_B;
	case 0x06: return INPUT_KEY_C;      case 0x07: return INPUT_KEY_D;
	case 0x08: return INPUT_KEY_E;      case 0x09: return INPUT_KEY_F;
	case 0x0a: return INPUT_KEY_G;      case 0x0b: return INPUT_KEY_H;
	case 0x0c: return INPUT_KEY_I;      case 0x0d: return INPUT_KEY_J;
	case 0x0e: return INPUT_KEY_K;      case 0x0f: return INPUT_KEY_L;
	case 0x10: return INPUT_KEY_M;      case 0x11: return INPUT_KEY_N;
	case 0x12: return INPUT_KEY_O;      case 0x13: return INPUT_KEY_P;
	case 0x14: return INPUT_KEY_Q;      case 0x15: return INPUT_KEY_R;
	case 0x16: return INPUT_KEY_S;      case 0x17: return INPUT_KEY_T;
	case 0x18: return INPUT_KEY_U;      case 0x19: return INPUT_KEY_V;
	case 0x1a: return INPUT_KEY_W;      case 0x1b: return INPUT_KEY_X;
	case 0x1c: return INPUT_KEY_Y;      case 0x1d: return INPUT_KEY_Z;
	case 0x1e: return INPUT_KEY_1;      case 0x1f: return INPUT_KEY_2;
	case 0x20: return INPUT_KEY_3;      case 0x21: return INPUT_KEY_4;
	case 0x22: return INPUT_KEY_5;      case 0x23: return INPUT_KEY_6;
	case 0x24: return INPUT_KEY_7;      case 0x25: return INPUT_KEY_8;
	case 0x26: return INPUT_KEY_9;      case 0x27: return INPUT_KEY_0;
	case 0x28: return INPUT_KEY_ENTER;
	case 0x29: return INPUT_KEY_ESC;
	case 0x2a: return INPUT_KEY_BACKSPACE;
	case 0x2b: return INPUT_KEY_TAB;
	case 0x2c: return INPUT_KEY_SPACE;
	case 0x2d: return INPUT_KEY_MINUS;
	case 0x2e: return INPUT_KEY_EQUAL;
	case 0x2f: return INPUT_KEY_LEFTBRACE;
	case 0x30: return INPUT_KEY_RIGHTBRACE;
	case 0x31: return INPUT_KEY_BACKSLASH;
	case 0x33: return INPUT_KEY_SEMICOLON;
	case 0x34: return INPUT_KEY_APOSTROPHE;
	case 0x35: return INPUT_KEY_GRAVE;
	case 0x36: return INPUT_KEY_COMMA;
	case 0x37: return INPUT_KEY_DOT;
	case 0x38: return INPUT_KEY_SLASH;
	case 0x39: return INPUT_KEY_CAPSLOCK;
	case 0x4a: return INPUT_KEY_HOME;
	case 0x4b: return INPUT_KEY_PAGEUP;
	case 0x4c: return INPUT_KEY_DELETE;
	case 0x4d: return INPUT_KEY_END;
	case 0x4e: return INPUT_KEY_PAGEDOWN;
	case 0x4f: return INPUT_KEY_RIGHT;
	case 0x50: return INPUT_KEY_LEFT;
	case 0x51: return INPUT_KEY_DOWN;
	case 0x52: return INPUT_KEY_UP;
	case 0x53: return INPUT_KEY_NUMLOCK;
	case 0x54: return INPUT_KEY_KP_SLASH;
	case 0x55: return INPUT_KEY_KP_ASTERISK;
	case 0x56: return INPUT_KEY_KP_MINUS;
	case 0x57: return INPUT_KEY_KP_PLUS;
	case 0x58: return INPUT_KEY_KP_ENTER;
	case 0x59: return INPUT_KEY_KP_1;   case 0x5a: return INPUT_KEY_KP_2;
	case 0x5b: return INPUT_KEY_KP_3;   case 0x5c: return INPUT_KEY_KP_4;
	case 0x5d: return INPUT_KEY_KP_5;   case 0x5e: return INPUT_KEY_KP_6;
	case 0x5f: return INPUT_KEY_KP_7;   case 0x60: return INPUT_KEY_KP_8;
	case 0x61: return INPUT_KEY_KP_9;   case 0x62: return INPUT_KEY_KP_0;
	case 0x63: return INPUT_KEY_KP_DOT;
	case 0x3a: return INPUT_KEY_F1;   case 0x3b: return INPUT_KEY_F2;
	case 0x3c: return INPUT_KEY_F3;   case 0x3d: return INPUT_KEY_F4;
	case 0x3e: return INPUT_KEY_F5;   case 0x3f: return INPUT_KEY_F6;
	case 0x40: return INPUT_KEY_F7;   case 0x41: return INPUT_KEY_F8;
	case 0x42: return INPUT_KEY_F9;   case 0x43: return INPUT_KEY_F10;
	case 0x44: return INPUT_KEY_F11;  case 0x45: return INPUT_KEY_F12;
	case 0x65: return INPUT_KEY_INSERT;
	default: return INPUT_KEY_NONE;
	}
}

void input_init(void)
{
	asm_mutex_init(&input_mutex);
}

static inline void input_lock(void)
{
	asm_mutex_lock(&input_mutex);
}

static inline void input_unlock(void)
{
	asm_mutex_unlock(&input_mutex);
}

/* Advance tail, dropping oldest if full. Returns slot to fill, or NULL. */
static input_evt_t *input_reserve(void)
{
	uint16_t next = (uint16_t)((q_tail + 1u) % INPUT_Q);
	if (next == q_head) {
		q_head = (uint16_t)((q_head + 1u) % INPUT_Q);
		next = (uint16_t)((q_tail + 1u) % INPUT_Q);
		if (next == q_head)
			return NULL;
	}
	input_evt_t *evt = &q[q_tail];
	q_tail = next;
	return evt;
}

void input_post_key(input_keycode_t code, uint8_t value)
{
	input_lock();
	input_evt_t *evt = input_reserve();
	if (!evt) { input_unlock(); return; }
	evt->kind = INPUT_EVT_KEY;
	evt->u.key.code = code;
	evt->u.key.value = value;
	input_unlock();
}

void input_post_mouse_move_rel(int dx, int dy)
{
	input_lock();
	input_evt_t *evt = input_reserve();
	if (!evt) { input_unlock(); return; }
	evt->kind = INPUT_EVT_MOUSE_MOVE_REL;
	evt->u.mouse_move_rel.dx = dx;
	evt->u.mouse_move_rel.dy = dy;
	input_unlock();
}

void input_post_mouse_move_abs(uint16_t x, uint16_t y)
{
	input_lock();
	input_evt_t *evt = input_reserve();
	if (!evt) { input_unlock(); return; }
	evt->kind = INPUT_EVT_MOUSE_MOVE_ABS;
	evt->u.mouse_move_abs.x = x;
	evt->u.mouse_move_abs.y = y;
	input_unlock();
}

void input_post_mouse_down(input_mouse_button_t button)
{
	input_lock();
	input_evt_t *evt = input_reserve();
	if (!evt) { input_unlock(); return; }
	evt->kind = INPUT_EVT_MOUSE_DOWN;
	evt->u.mouse_button.button = button;
	input_unlock();
}

void input_post_mouse_up(input_mouse_button_t button)
{
	input_lock();
	input_evt_t *evt = input_reserve();
	if (!evt) { input_unlock(); return; }
	evt->kind = INPUT_EVT_MOUSE_UP;
	evt->u.mouse_button.button = button;
	input_unlock();
}

void input_post_mouse_wheel(int steps)
{
	input_lock();
	input_evt_t *evt = input_reserve();
	if (!evt) { input_unlock(); return; }
	evt->kind = INPUT_EVT_MOUSE_WHEEL;
	evt->u.mouse_wheel.steps = steps;
	input_unlock();
}

void input_report_mouse_button(input_mouse_button_t button, bool pressed)
{
	input_lock();
	uint8_t mask = 1u << (uint8_t)button;
	bool was = (mouse_btn_state & mask) != 0;
	if (pressed == was) { input_unlock(); return; }

	if (pressed) mouse_btn_state |= mask;
	else         mouse_btn_state &= ~mask;

	input_evt_t *evt = input_reserve();
	if (!evt) { input_unlock(); return; }
	evt->kind = pressed ? INPUT_EVT_MOUSE_DOWN : INPUT_EVT_MOUSE_UP;
	evt->u.mouse_button.button = button;
	input_unlock();
}

void input_report_keyboard(uint8_t mod, const uint8_t keys[6])
{
	input_lock();

	/* Modifier changes */
	const input_keycode_t mod_map[8] = {
		INPUT_KEY_LEFTCTRL, INPUT_KEY_LEFTSHIFT, INPUT_KEY_LEFTALT, INPUT_KEY_LEFTMETA,
		INPUT_KEY_RIGHTCTRL, INPUT_KEY_RIGHTSHIFT, INPUT_KEY_RIGHTALT, INPUT_KEY_RIGHTMETA,
	};
	for (int b = 0; b < 8; b++) {
		uint8_t m = 1u << b;
		if ((mod & m) == (kbd_prev_mod & m))
			continue;
		input_keycode_t k = mod_map[b];
		if (k == INPUT_KEY_NONE) continue;
		input_evt_t *evt = input_reserve();
		if (!evt) { input_unlock(); return; }
		evt->kind = INPUT_EVT_KEY;
		evt->u.key.code = k;
		evt->u.key.value = (mod & m) ? 1 : 0;
	}
	kbd_prev_mod = mod;

	/* Releases: in prev but not in cur */
	for (int i = 0; i < 6; i++) {
		uint8_t kc = kbd_prev_keys[i];
		if (kc == 0) continue;
		bool found = false;
		for (int j = 0; j < 6; j++)
			if (keys[j] == kc) { found = true; break; }
		if (found) continue;
		input_keycode_t k = hid_kbd_to_input(kc);
		if (k == INPUT_KEY_NONE) continue;
		input_evt_t *evt = input_reserve();
		if (!evt) { input_unlock(); return; }
		evt->kind = INPUT_EVT_KEY;
		evt->u.key.code = k;
		evt->u.key.value = 0;
	}

	/* Presses: in cur but not in prev */
	for (int i = 0; i < 6; i++) {
		uint8_t kc = keys[i];
		if (kc == 0) continue;
		bool found = false;
		for (int j = 0; j < 6; j++)
			if (kbd_prev_keys[j] == kc) { found = true; break; }
		if (found) continue;
		input_keycode_t k = hid_kbd_to_input(kc);
		if (k == INPUT_KEY_NONE) continue;
		input_evt_t *evt = input_reserve();
		if (!evt) { input_unlock(); return; }
		evt->kind = INPUT_EVT_KEY;
		evt->u.key.code = k;
		evt->u.key.value = 1;
	}

	memcpy(kbd_prev_keys, keys, 6);
	input_unlock();
}

bool input_peek(input_evt_t *out)
{
	if (out == NULL)
		return false;

	input_lock();
	if (q_head == q_tail) {
		input_unlock();
		return false;
	}
	*out = q[q_head];
	input_unlock();
	return true;
}

bool input_pop(input_evt_t *out)
{
	if (out == NULL)
		return false;

	input_lock();
	if (q_head == q_tail) {
		input_unlock();
		return false;
	}
	*out = q[q_head];
	q_head = (uint16_t)((q_head + 1u) % INPUT_Q);
	input_unlock();
	return true;
}
