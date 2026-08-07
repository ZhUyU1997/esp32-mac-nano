#ifndef WIFI_PANEL_H
#define WIFI_PANEL_H

#include <stdbool.h>
#include "lvgl.h"

void wifi_panel_create(lv_obj_t *screen, int32_t x, int32_t y, int32_t w, int32_t h);
void wifi_panel_set_enabled(bool enabled);
void wifi_panel_refresh(void);

#endif
