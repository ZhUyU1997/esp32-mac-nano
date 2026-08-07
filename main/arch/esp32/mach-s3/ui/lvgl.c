#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "esp_timer.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "framebuffer.h"
#include "input.h"
#include "wifi_panel.h"
#include "video/frame_blit.h"
#include "blit_worker.h"
#include "ui.h"
#include "lvgl.h"
#include "fast_attr.h"

struct ui_s {
	esp_timer_handle_t tick_timer;
	lv_display_t *disp;
	framebuffer_t *lcd;
	mach_s3_blit_worker_t *blit_worker;
	lv_indev_t *mouse;
	lv_indev_t *keypad;
	lv_group_t *group;
	int32_t mouse_x;
	int32_t mouse_y;
	bool mouse_left;
	lv_obj_t *cursor;
	bool pause_exit_request;
};

static const uint8_t k_cursor_a1_data[] = {
        0xC0, 0x00, 0xE0, 0x00, 0xF0, 0x00, 0xF8, 0x00, 0xFC, 0x00, 0xFE, 0x00, 0xFF, 0x00, 0xFF, 0x80,
        0xFF, 0xC0, 0xFF, 0xE0, 0xFE, 0x00, 0xEF, 0x00, 0xCF, 0x00, 0x87, 0x80, 0x07, 0x80, 0x03, 0x80,
};
static const uint8_t k_cursor_a1_data_black[] = {
        0x00, 0x00, 0x40, 0x00, 0x60, 0x00, 0x70, 0x00, 0x78, 0x00, 0x7C, 0x00, 0x7E, 0x00, 0x7F, 0x00,
        0x7F, 0x80, 0x7C, 0x00, 0x6C, 0x00, 0x46, 0x00, 0x06, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00,
};
static const lv_image_dsc_t k_cursor_a1 = {
        .header =
                {
                        .magic = LV_IMAGE_HEADER_MAGIC,
                        .cf = LV_COLOR_FORMAT_A1,
                        .flags = 0,
                        .w = 16,
                        .h = 16,
                        .stride = 2,
                },
        .data_size = sizeof(k_cursor_a1_data),
        .data = k_cursor_a1_data,
        .reserved = NULL,
        .reserved_2 = NULL,
};
static const lv_image_dsc_t k_cursor_data_a1 = {
        .header =
                {
                        .magic = LV_IMAGE_HEADER_MAGIC,
                        .cf = LV_COLOR_FORMAT_A1,
                        .flags = 0,
                        .w = 16,
                        .h = 16,
                        .stride = 2,
                },
        .data_size = sizeof(k_cursor_a1_data_black),
        .data = k_cursor_a1_data_black,
        .reserved = NULL,
        .reserved_2 = NULL,
};

static const FAST_DATA_ATTR uint8_t k_bg_grid_a1_data[] = {
        0xAA,
        0x55,
        0xAA,
        0x55,
        0xAA,
        0x55,
        0xAA,
        0x55,
};

static const lv_image_dsc_t k_bg_grid_a1 = {
        .header =
                {
                        .magic = LV_IMAGE_HEADER_MAGIC,
                        .cf = LV_COLOR_FORMAT_A1,
                        .flags = 0,
                        .w = 8,
                        .h = 8,
                        .stride = 1,
                },
        .data_size = sizeof(k_bg_grid_a1_data),
        .data = k_bg_grid_a1_data,
        .reserved = NULL,
        .reserved_2 = NULL,
};

void mach_s3_settings_ui_show(void);
bool mach_s3_settings_ui_take_exit_request(void);

static void mach_s3_lvgl_tick_cb(void *arg)
{
	(void)arg;
	lv_tick_inc(1);
}

static int32_t clamp_i32(int32_t v, int32_t lo, int32_t hi)
{
	if (v < lo)
		return lo;
	if (v > hi)
		return hi;
	return v;
}

static void mach_s3_lvgl_mouse_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
	(void)indev;
	assert(data != NULL);

	ui_t *ctx = (ui_t *)lv_indev_get_user_data(indev);
	assert(ctx != NULL);

	int32_t dx = 0;
	int32_t dy = 0;
	bool have_abs = false;
	int32_t abs_x = 0;
	int32_t abs_y = 0;
	input_evt_t evt;
	while (input_peek(&evt)) {
		if (evt.kind == INPUT_EVT_KEY)
			break; /* all keys are routed to the keypad indev */
		input_pop(&evt);
		if (evt.kind == INPUT_EVT_MOUSE_MOVE_REL) {
			dx += evt.u.mouse_move_rel.dx;
			dy += evt.u.mouse_move_rel.dy;
		} else if (evt.kind == INPUT_EVT_MOUSE_MOVE_ABS) {
			have_abs = true;
			abs_x = (int32_t)evt.u.mouse_move_abs.x;
			abs_y = (int32_t)evt.u.mouse_move_abs.y;
		} else if (evt.kind == INPUT_EVT_MOUSE_DOWN) {
			if (evt.u.mouse_button.button == INPUT_MOUSE_BTN_LEFT) {
				ctx->mouse_left = true;
			}
		} else if (evt.kind == INPUT_EVT_MOUSE_UP) {
			if (evt.u.mouse_button.button == INPUT_MOUSE_BTN_LEFT) {
				ctx->mouse_left = false;
			} else if (evt.u.mouse_button.button == INPUT_MOUSE_BTN_MIDDLE) {
				ctx->pause_exit_request = true;
			}
		}
	}

	assert(ctx->disp != NULL);

	const int32_t w = (int32_t)lv_display_get_horizontal_resolution(ctx->disp);
	const int32_t h = (int32_t)lv_display_get_vertical_resolution(ctx->disp);
	assert(w > 0);
	assert(h > 0);

	if (have_abs) {
		assert(ctx->lcd != NULL);
		const int32_t lcd_w = ctx->lcd->width;
		const int32_t lcd_h = ctx->lcd->height;
		if (lcd_w > 0 && lcd_h > 0) {
			const int32_t cx = clamp_i32(abs_x, 0, lcd_w - 1);
			const int32_t cy = clamp_i32(abs_y, 0, lcd_h - 1);
			ctx->mouse_x = (cy * w) / lcd_h;
			ctx->mouse_y = ((lcd_w - 1 - cx) * h) / lcd_w;
		}
	} else {
		ctx->mouse_x += dx;
		ctx->mouse_y += dy;
	}

	ctx->mouse_x = clamp_i32(ctx->mouse_x, 0, w - 1);
	ctx->mouse_y = clamp_i32(ctx->mouse_y, 0, h - 1);

	data->point.x = (lv_coord_t)ctx->mouse_x;
	data->point.y = (lv_coord_t)ctx->mouse_y;
	data->state = ctx->mouse_left ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
	data->continue_reading = false;
}

typedef struct {
	const uint8_t *px_bits;
	int lcd_w;
	int lcd_h;
	bool bit1_white;
} mach_s3_lvgl_blit_ctx_t;

static void mach_s3_lvgl_blit_cb(framebuffer_t *lcd, void *user_ctx)
{
	const mach_s3_lvgl_blit_ctx_t *ctx = (const mach_s3_lvgl_blit_ctx_t *)user_ctx;
	assert(ctx != NULL);
	assert(ctx->px_bits != NULL);
	assert(lcd != NULL);
	void *fb = framebuffer_get_framebuffer(lcd);
	assert(fb != NULL);
	if (ctx->bit1_white) {
		blit_lvgl_i1_to_lcd_l8_rot90_bit1_white((uint8_t *)fb, ctx->px_bits, ctx->lcd_w, ctx->lcd_h);
	} else {
		blit_lvgl_i1_to_lcd_l8_rot90_bit1_black((uint8_t *)fb, ctx->px_bits, ctx->lcd_w, ctx->lcd_h);
	}
}

static void FAST_FUNC_ATTR FAST_O3_ATTR mach_s3_lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
	(void)area;
	if (!lv_display_flush_is_last(disp)) {
		lv_display_flush_ready(disp);
		return;
	}

	ui_t *ctx = (ui_t *)lv_display_get_user_data(disp);
	assert(ctx != NULL);
	assert(ctx->lcd != NULL);
	framebuffer_t *lcd = ctx->lcd;
	const int lcd_w = lcd->width;
	const int lcd_h = lcd->height;
	const int src_w = (int)lv_display_get_horizontal_resolution(disp);
	const int src_h = (int)lv_display_get_vertical_resolution(disp);
	assert(lcd_w > 0);
	assert(lcd_h > 0);
	assert(src_w > 0);
	assert(src_h > 0);

	const lv_draw_buf_t *draw_buf = lv_display_get_buf_active(disp);
	const uint8_t *base = (draw_buf != NULL && draw_buf->data != NULL) ? (const uint8_t *)draw_buf->data : px_map;
	assert(base != NULL);

	uint16_t l0 = (uint16_t)base[0] + (uint16_t)base[1] + (uint16_t)base[2];
	uint16_t l1 = (uint16_t)base[4] + (uint16_t)base[5] + (uint16_t)base[6];
	uint8_t map0;
	uint8_t map1;
	if (l0 == l1) {
		map0 = 0u;
		map1 = 1u;
	} else if (l0 > l1) {
		map0 = 1u;
		map1 = 0u;
	} else {
		map0 = 0u;
		map1 = 1u;
	}

	const uint8_t *px_bits = base + 8;

	const uint8_t byte0 = map0 ? 0xFFu : 0x00u;
	const uint8_t byte1 = map1 ? 0xFFu : 0x00u;
	const bool bit1_white = (byte0 == 0x00u && byte1 == 0xFFu);
	const mach_s3_lvgl_blit_ctx_t bctx = {
	        .px_bits = px_bits,
	        .lcd_w = lcd_w,
	        .lcd_h = lcd_h,
	        .bit1_white = bit1_white,
	};
	(void)mach_s3_blit_worker_submit_and_wait(ctx->blit_worker, mach_s3_lvgl_blit_cb, (void *)&bctx);
	lv_display_flush_ready(disp);
}

/* Keypad indev: consumes all key events from the shared input queue and
 * maps them to LVGL keys (sent to the focused group object).
 * The pointer indev skips key events (see mach_s3_lvgl_mouse_read_cb). */
static void mach_s3_lvgl_keypad_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
	(void)indev;
	assert(data != NULL);

	data->state = LV_INDEV_STATE_RELEASED;
	data->key = 0;

	input_evt_t evt;
	if (input_peek(&evt) && evt.kind == INPUT_EVT_KEY) {
		input_pop(&evt);
		if (evt.u.key.code == INPUT_KEY_F12) {
			data->state = evt.u.key.value ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
			data->key = LV_KEY_ESC;
		} else if (evt.u.key.code == INPUT_KEY_F4 && evt.u.key.value) {
			/* three-way switch left (F4) = WiFi off */
			wifi_panel_set_enabled(false);
		} else if (evt.u.key.code == INPUT_KEY_F5 && evt.u.key.value) {
			/* three-way switch right (F5) = WiFi on */
			wifi_panel_set_enabled(true);
		} else {
			data->state = evt.u.key.value ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
			switch (evt.u.key.code) {
			case INPUT_KEY_UP:     data->key = LV_KEY_UP;     break;
			case INPUT_KEY_DOWN:   data->key = LV_KEY_DOWN;   break;
			case INPUT_KEY_ENTER:  data->key = LV_KEY_ENTER;  break;
			default:               data->key = 0;             break;
			}
		}
	}
}

/* F12 press (short press) on the focused target exits the pause menu. */
static void on_f12_focus_key(lv_event_t *e)
{
	ui_t *ctx = (ui_t *)lv_event_get_user_data(e);
	lv_event_code_t code = lv_event_get_code(e);
	if (ctx != NULL && code == LV_EVENT_KEY) {
		if (lv_indev_get_key(lv_indev_active()) == LV_KEY_ESC) {
			ctx->pause_exit_request = true;
		}
	}
}

static void ui_setup_keypad(ui_t *ctx)
{
	assert(ctx != NULL);

	ctx->keypad = lv_indev_create();
	assert(ctx->keypad != NULL);
	lv_indev_set_type(ctx->keypad, LV_INDEV_TYPE_KEYPAD);
	lv_indev_set_read_cb(ctx->keypad, mach_s3_lvgl_keypad_read_cb);
	lv_indev_set_user_data(ctx->keypad, ctx);

	/* Hidden focus target receiving F12 press (ESC) events. */
	ctx->group = lv_group_create();
	assert(ctx->group != NULL);
	lv_obj_t *target = lv_obj_create(lv_layer_top());
	assert(target != NULL);
	lv_obj_remove_style_all(target);
	lv_obj_set_style_bg_opa(target, LV_OPA_TRANSP, 0);
	lv_obj_set_size(target, 1, 1);
	lv_obj_clear_flag(target, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(target, LV_OBJ_FLAG_IGNORE_LAYOUT);
	lv_obj_add_event_cb(target, on_f12_focus_key, LV_EVENT_KEY, ctx);
	lv_group_add_obj(ctx->group, target);
	lv_indev_set_group(ctx->keypad, ctx->group);
	lv_group_focus_obj(target);
}

static void ui_setup_mouse_and_cursor(ui_t *ctx, int lvgl_w, int lvgl_h)
{
	assert(ctx != NULL);
	assert(lvgl_w > 0);
	assert(lvgl_h > 0);

	ctx->mouse = lv_indev_create();
	assert(ctx->mouse != NULL);
	lv_indev_set_type(ctx->mouse, LV_INDEV_TYPE_POINTER);
	lv_indev_set_read_cb(ctx->mouse, mach_s3_lvgl_mouse_read_cb);
	lv_indev_set_user_data(ctx->mouse, ctx);

	ctx->mouse_x = lvgl_w / 2;
	ctx->mouse_y = lvgl_h / 2;
	ctx->mouse_left = false;

	ctx->cursor = lv_obj_create(lv_layer_top());
	assert(ctx->cursor != NULL);
	lv_obj_remove_style_all(ctx->cursor);
	lv_obj_set_size(ctx->cursor, 16, 16);
	lv_obj_set_style_bg_opa(ctx->cursor, LV_OPA_TRANSP, 0);
	lv_obj_clear_flag(ctx->cursor, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_clear_flag(ctx->cursor, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(ctx->cursor, LV_OBJ_FLAG_IGNORE_LAYOUT);

	lv_obj_t *cursor_white = lv_image_create(ctx->cursor);
	assert(cursor_white != NULL);
	lv_image_set_src(cursor_white, &k_cursor_a1);
	lv_obj_set_pos(cursor_white, 0, 0);
	lv_obj_clear_flag(cursor_white, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_clear_flag(cursor_white, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_image_recolor(cursor_white, lv_color_white(), 0);
	lv_obj_set_style_image_recolor_opa(cursor_white, LV_OPA_COVER, 0);

	lv_obj_t *cursor_black = lv_image_create(ctx->cursor);
	assert(cursor_black != NULL);
	lv_image_set_src(cursor_black, &k_cursor_data_a1);
	lv_obj_set_pos(cursor_black, 0, 0);
	lv_obj_clear_flag(cursor_black, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_clear_flag(cursor_black, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_image_recolor(cursor_black, lv_color_black(), 0);
	lv_obj_set_style_image_recolor_opa(cursor_black, LV_OPA_COVER, 0);

	lv_indev_set_cursor(ctx->mouse, ctx->cursor);
}

ui_t *ui_new(ui_config_t config)
{
	ui_t *ctx = (ui_t *)heap_caps_calloc(1, sizeof(*ctx), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
	assert(ctx != NULL);

	ctx->blit_worker = config.blit_worker;
	assert(ctx->blit_worker != NULL);
	framebuffer_t *lcd = config.lcd;
	assert(lcd != NULL);

	const int lcd_w = lcd->width;
	const int lcd_h = lcd->height;
	ctx->lcd = lcd;

	const int lvgl_w = lcd_h;
	const int lvgl_h = lcd_w;

	lv_init();

	lv_display_t *disp = lv_display_create(lvgl_w, lvgl_h);
	ctx->disp = disp;
	lv_display_set_user_data(disp, ctx);
	lv_display_set_flush_cb(disp, mach_s3_lvgl_flush_cb);
	lv_display_set_color_format(disp, LV_COLOR_FORMAT_I1);

	const size_t buf_size = ((size_t)lvgl_w * (size_t)lvgl_h) / 8u + 8u;
	void *buf1 = heap_caps_calloc(1, buf_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
	assert(buf1 != NULL);
	lv_display_set_buffers(disp, buf1, NULL, buf_size, LV_DISPLAY_RENDER_MODE_DIRECT);

	ui_setup_mouse_and_cursor(ctx, lvgl_w, lvgl_h);
	ui_setup_keypad(ctx);

	const esp_timer_create_args_t tick_args = {
	        .callback = mach_s3_lvgl_tick_cb,
	        .arg = NULL,
	        .dispatch_method = ESP_TIMER_TASK,
	        .name = "lvgl_tick",
	        .skip_unhandled_events = true,
	};
	assert(esp_timer_create(&tick_args, &ctx->tick_timer) == ESP_OK);
	(void)esp_timer_start_periodic(ctx->tick_timer, 1000);

	lv_obj_set_style_bg_image_src(lv_screen_active(), &k_bg_grid_a1, 0);
	lv_obj_set_style_bg_image_tiled(lv_screen_active(), true, 0);
	lv_obj_set_style_bg_image_recolor(lv_screen_active(), lv_color_white(), 0);
	lv_obj_set_style_bg_image_recolor_opa(lv_screen_active(), LV_OPA_COVER, 0);
	lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), 0);
	lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);
	return ctx;
}

void ui_run_frame(ui_t *ui)
{
	assert(ui != NULL);
	assert(ui->disp != NULL);

	uint32_t wait_ms = lv_timer_handler();
	if (wait_ms < 1) {
		wait_ms = 1;
	} else if (wait_ms > 20) {
		wait_ms = 20;
	}
	vTaskDelay(pdMS_TO_TICKS(wait_ms));
}

void ui_pause_enter(ui_t *ui)
{
	assert(ui != NULL);
	ui->pause_exit_request = false;

	mach_s3_settings_ui_show();
}

void ui_pause_leave(ui_t *ui)
{
	assert(ui != NULL);
	ui->pause_exit_request = false;
}

bool ui_pause_take_exit_request(ui_t *ui)
{
	assert(ui != NULL);
	const bool r = ui->pause_exit_request || mach_s3_settings_ui_take_exit_request();
	ui->pause_exit_request = false;
	return r;
}

void ui_set_mouse_pos(ui_t *ui, int32_t x, int32_t y)
{
	assert(ui != NULL);
	ui->mouse_x = x;
	ui->mouse_y = y;
}

void ui_get_mouse_pos(const ui_t *ui, int32_t *x, int32_t *y)
{
	assert(ui != NULL);
	if (x) *x = ui->mouse_x;
	if (y) *y = ui->mouse_y;
}
