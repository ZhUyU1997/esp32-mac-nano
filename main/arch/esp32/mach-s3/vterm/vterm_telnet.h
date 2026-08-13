#ifndef VTERM_TELNET_H
#define VTERM_TELNET_H

#include <stddef.h>
#include <stdint.h>

/* start the telnet client task (connects to host:23, retries) */
void vterm_telnet_start(void);

/* drain received host bytes into out (max cap); returns count in *len */
void vterm_telnet_pop(uint8_t *out, size_t *len, size_t cap);

/* forward keyboard bytes to the host; false when not connected */
bool vterm_telnet_send(const uint8_t *data, size_t len);

#endif
