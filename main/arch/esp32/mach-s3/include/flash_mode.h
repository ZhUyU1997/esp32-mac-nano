#ifndef MACH_S3_FLASH_MODE_H
#define MACH_S3_FLASH_MODE_H

#include <stdbool.h>

/* Pause-menu action: set the one-shot flag and reboot. On the next boot
 * the app skips USB Host init, leaving the USB-Serial-JTAG controller
 * exposed so a browser (esptool-js over WebSerial) can flash the chip
 * without any hardware button. */
void mach_s3_flash_mode_enter(void);

/* Called once at startup, before USB Host init. Checks both entry paths
 * (forced: physical key held at boot; menu: one-shot NVS flag), clears
 * the one-shot flag. Returns true if this boot is flash mode. */
bool mach_s3_flash_mode_check(void);

/* True after flash_mode_check() found the flag (used by UI). */
bool mach_s3_flash_mode_active(void);

#endif
