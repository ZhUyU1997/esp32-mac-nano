#ifndef MACH_S3_UI_H
#define MACH_S3_UI_H

#include <stdbool.h>
#include "blit_worker.h"

typedef struct ui_s ui_t;
typedef struct mach_s3_blit_worker mach_s3_blit_worker_t;

typedef struct {
	mach_s3_blit_worker_t *blit_worker;
	framebuffer_t *lcd;
} ui_config_t;

ui_t *ui_new(ui_config_t config);
void ui_run_frame(ui_t *ui);
void ui_pause_enter(ui_t *ui);
void ui_pause_leave(ui_t *ui);
bool ui_pause_take_exit_request(ui_t *ui);
void ui_set_mouse_pos(ui_t *ui, int32_t x, int32_t y);
void ui_get_mouse_pos(const ui_t *ui, int32_t *x, int32_t *y);
#endif
