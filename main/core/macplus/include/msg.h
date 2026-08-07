/*
 * Cross-thread message dispatch (PCE mac_set_msg style).
 *
 * Producers (e.g. the httpd task) queue commands; the emulator thread
 * polls and dispatches them via the registered handler table, so all
 * emulator-state mutations happen on the emulator thread.
 *
 * Ownership: the caller keeps its own buffer (allocates/frees it); the
 * queue copies msg/val internally via the injected allocator (PSRAM on
 * the ESP32 build) and frees the copies after dispatch.
 */
#ifndef MAC_MSG_H
#define MAC_MSG_H

#include <stddef.h>

struct macplus;

typedef int (*mac_msg_set_fn)(struct macplus *sim, const char *msg, const char *val);

/* Emulator thread: set the sim context and the allocator for internal
 * copies (platform allocator — PSRAM on ESP32). */
void mac_msg_init(struct macplus *sim, void *(*alloc)(size_t), void (*dealloc)(void *));

/* Emulator thread: register a command (name + handler). */
void mac_msg_register(const char *msg, mac_msg_set_fn fn);

/* Any thread: queue a message. msg/val are copied internally via the
 * injected allocator; the caller keeps its own buffers. */
void mac_msg_submit(const char *msg, const char *val);

/* Emulator thread: dispatch queued messages via the registered table.
 * Returns 1 if any message was processed. */
int mac_msg_dispatch(void);

#endif /* MAC_MSG_H */
