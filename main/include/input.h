#ifndef INPUT_INPUT_H
#define INPUT_INPUT_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
	INPUT_EVT_KEY,
	INPUT_EVT_MOUSE_MOVE_REL,
	INPUT_EVT_MOUSE_MOVE_ABS,
	INPUT_EVT_MOUSE_DOWN,
	INPUT_EVT_MOUSE_UP,
	INPUT_EVT_MOUSE_WHEEL,
} input_kind_t;

typedef enum {
	INPUT_MOUSE_BTN_LEFT = 0,
	INPUT_MOUSE_BTN_RIGHT,
	INPUT_MOUSE_BTN_MIDDLE,
} input_mouse_button_t;

typedef enum {
	INPUT_KEY_NONE = 0,

	INPUT_KEY_A,
	INPUT_KEY_B,
	INPUT_KEY_C,
	INPUT_KEY_D,
	INPUT_KEY_E,
	INPUT_KEY_F,
	INPUT_KEY_G,
	INPUT_KEY_H,
	INPUT_KEY_I,
	INPUT_KEY_J,
	INPUT_KEY_K,
	INPUT_KEY_L,
	INPUT_KEY_M,
	INPUT_KEY_N,
	INPUT_KEY_O,
	INPUT_KEY_P,
	INPUT_KEY_Q,
	INPUT_KEY_R,
	INPUT_KEY_S,
	INPUT_KEY_T,
	INPUT_KEY_U,
	INPUT_KEY_V,
	INPUT_KEY_W,
	INPUT_KEY_X,
	INPUT_KEY_Y,
	INPUT_KEY_Z,

	INPUT_KEY_1,
	INPUT_KEY_2,
	INPUT_KEY_3,
	INPUT_KEY_4,
	INPUT_KEY_5,
	INPUT_KEY_6,
	INPUT_KEY_7,
	INPUT_KEY_8,
	INPUT_KEY_9,
	INPUT_KEY_0,

	INPUT_KEY_ENTER,
	INPUT_KEY_ESC,
	INPUT_KEY_BACKSPACE,
	INPUT_KEY_TAB,
	INPUT_KEY_SPACE,

	INPUT_KEY_MINUS,
	INPUT_KEY_EQUAL,
	INPUT_KEY_LEFTBRACE,
	INPUT_KEY_RIGHTBRACE,
	INPUT_KEY_BACKSLASH,
	INPUT_KEY_SEMICOLON,
	INPUT_KEY_APOSTROPHE,
	INPUT_KEY_GRAVE,
	INPUT_KEY_COMMA,
	INPUT_KEY_DOT,
	INPUT_KEY_SLASH,

	INPUT_KEY_CAPSLOCK,
	INPUT_KEY_NUMLOCK,

	INPUT_KEY_KP_SLASH,
	INPUT_KEY_KP_ASTERISK,
	INPUT_KEY_KP_MINUS,
	INPUT_KEY_KP_PLUS,
	INPUT_KEY_KP_ENTER,
	INPUT_KEY_KP_1,
	INPUT_KEY_KP_2,
	INPUT_KEY_KP_3,
	INPUT_KEY_KP_4,
	INPUT_KEY_KP_5,
	INPUT_KEY_KP_6,
	INPUT_KEY_KP_7,
	INPUT_KEY_KP_8,
	INPUT_KEY_KP_9,
	INPUT_KEY_KP_0,
	INPUT_KEY_KP_DOT,

	INPUT_KEY_INSERT,
	INPUT_KEY_HOME,
	INPUT_KEY_PAGEUP,
	INPUT_KEY_DELETE,
	INPUT_KEY_END,
	INPUT_KEY_PAGEDOWN,
	INPUT_KEY_RIGHT,
	INPUT_KEY_LEFT,
	INPUT_KEY_DOWN,
	INPUT_KEY_UP,

	INPUT_KEY_LEFTCTRL,
	INPUT_KEY_LEFTSHIFT,
	INPUT_KEY_LEFTALT,
	INPUT_KEY_RIGHTCTRL,
	INPUT_KEY_RIGHTSHIFT,
	INPUT_KEY_RIGHTALT,

	INPUT_KEY_LEFTMETA,
	INPUT_KEY_RIGHTMETA,

	INPUT_KEY_F1 = 0x100,
	INPUT_KEY_F2,
	INPUT_KEY_F3,
	INPUT_KEY_F4,
	INPUT_KEY_F5,
	INPUT_KEY_F6,
	INPUT_KEY_F7,
	INPUT_KEY_F8,
	INPUT_KEY_F9,
	INPUT_KEY_F10,
	INPUT_KEY_F11,
	INPUT_KEY_F12,
	/* Standard system keys (clear semantics; not bound to the 3-way switch) */
	INPUT_KEY_BRIGHTNESS_DOWN,
	INPUT_KEY_BRIGHTNESS_UP,
	INPUT_KEY_VOLUME_DOWN,
	INPUT_KEY_VOLUME_UP,
	INPUT_KEY_VOLUME_MUTE,
	INPUT_KEY_POWER,
} input_keycode_t;

typedef struct {
	input_kind_t kind;
	union {
		struct {
			input_keycode_t code;
			uint8_t value;
		} key;
		struct {
			int32_t dx;
			int32_t dy;
		} mouse_move_rel;
		struct {
			uint16_t x;
			uint16_t y;
		} mouse_move_abs;
		struct {
			input_mouse_button_t button;
		} mouse_button;
		struct {
			int steps; /* +1 scroll up, -1 scroll down */
		} mouse_wheel;
	} u;
} input_evt_t;

void input_post_key(input_keycode_t code, uint8_t value);
void input_post_mouse_move_rel(int dx, int dy);
void input_post_mouse_move_abs(uint16_t x, uint16_t y);
void input_post_mouse_down(input_mouse_button_t button);
void input_post_mouse_up(input_mouse_button_t button);
void input_post_mouse_wheel(int steps);
void input_report_mouse_button(input_mouse_button_t button, bool pressed);
void input_report_keyboard(uint8_t mod, const uint8_t keys[6]);

void input_init(void);
bool input_pop(input_evt_t *out);
bool input_peek(input_evt_t *out);

#endif
