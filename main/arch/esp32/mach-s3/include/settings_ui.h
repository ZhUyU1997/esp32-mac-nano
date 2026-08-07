#ifndef MACH_S3_SETTINGS_UI_H
#define MACH_S3_SETTINGS_UI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "machine_backend.h"

#define SETTINGS_UI_VOLUME_MAX 25u
#define SETTINGS_UI_BACKLIGHT_DEFAULT 75u
#define SETTINGS_UI_VOLUME_DEFAULT 15u

void mach_s3_settings_ui_bind_backend(machine_backend_t *backend);
void mach_s3_settings_ui_show(void);
bool mach_s3_settings_ui_take_exit_request(void);

#endif
