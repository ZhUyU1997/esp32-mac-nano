#ifndef VTERM_ESP32_H
#define VTERM_ESP32_H

struct framebuffer_t;
struct mach_s3_blit_worker;

/* M1: render the static selftest page (character table + 64-colour bars)
 * to the given framebuffer via the blit worker (vsync-synced write). */
void vterm_esp32_selftest(struct framebuffer_t *lcd, struct mach_s3_blit_worker *blit_worker);

/* M2/M3: interactive terminal mode (temporary MODE_UI replacement).
 * Runs until F10 is pressed; returns true when the user exits. */
bool vterm_esp32_enter(struct framebuffer_t *lcd, struct mach_s3_blit_worker *blit_worker);

#endif
