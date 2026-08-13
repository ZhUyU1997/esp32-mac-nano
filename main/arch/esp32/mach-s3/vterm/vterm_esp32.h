#ifndef VTERM_ESP32_H
#define VTERM_ESP32_H

struct framebuffer_t;
struct mach_s3_blit_worker;

/* Init/reinit: allocate, reset the screen, clear and push the first
 * frame to the given framebuffer via the blit worker (vsync-synced). */
void vterm_esp32_selftest(struct framebuffer_t *lcd, struct mach_s3_blit_worker *blit_worker);

/* Interactive terminal mode (MODE_UI replacement). Runs until F10/F12 is
 * pressed; returns true when the user exits back to the Mac. */
bool vterm_esp32_enter(struct framebuffer_t *lcd, struct mach_s3_blit_worker *blit_worker);

#endif
