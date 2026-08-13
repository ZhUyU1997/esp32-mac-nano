#ifndef VTERM_HID_MAP_H
#define VTERM_HID_MAP_H

#include <stddef.h>
#include <stdint.h>
#include "input.h"

/* track a modifier key event (press/release); true if consumed */
bool vterm_hid_mod_event(input_keycode_t code, uint8_t value);

/* true while either Shift key is held */
bool vterm_hid_shift_down(void);

/* map a character key press to VT100 bytes; returns length (0 = none) */
size_t vterm_hid_map(input_keycode_t code, char *out, size_t out_sz);

#endif
