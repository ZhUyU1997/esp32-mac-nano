#ifndef MACPLUS_MAC_HID_BRIDGE_H
#define MACPLUS_MAC_HID_BRIDGE_H

#include <stdint.h>

struct macplus;
void macplus_input_poll(struct macplus *s);

#endif
