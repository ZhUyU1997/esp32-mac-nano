#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#include "lvgl.h"
#include "settings_ui.h"
#include "wifi_panel.h"
#include "ui_strings.h"

#include "esp_app_desc.h"
#include "esp_ota_ops.h"

#include "flash_mode.h"

extern const lv_font_t mono_opposans_18;
extern const lv_font_t mono_opposans_14;

#define SETTINGS_UI_MAX_FLOPPY 16
#define SETTINGS_UI_MAX_NAME 64

typedef struct {
	lv_obj_t *status[SETTINGS_UI_MAX_FLOPPY];
	lv_obj_t *name[SETTINGS_UI_MAX_FLOPPY];
	lv_obj_t *info_box[SETTINGS_UI_MAX_FLOPPY];
	lv_obj_t *insert_action_btn;
	char file_name[SETTINGS_UI_MAX_FLOPPY][SETTINGS_UI_MAX_NAME];
	lv_obj_t *toast_box;
	lv_obj_t *toast_label;
	lv_timer_t *toast_timer;
	machine_backend_t *backend;
	uint16_t count;
	int16_t inserted;
	int16_t selected;
	bool exit_request;
} settings_ui_state_t;

static settings_ui_state_t g_settings_ui;

void mach_s3_settings_ui_bind_backend(machine_backend_t *backend)
{
	assert(backend != NULL);
	g_settings_ui.backend = backend;
}

static void settings_ui_sync_inserted(void)
{
	if (g_settings_ui.backend->get_inserted == NULL) {
		return;
	}
	const int16_t idx = g_settings_ui.backend->get_inserted(g_settings_ui.backend);
	if (idx >= 0 && (uint16_t)idx < g_settings_ui.count) {
		g_settings_ui.inserted = idx;
	} else if (idx == -2) {
		g_settings_ui.inserted = -2;
	} else {
		g_settings_ui.inserted = -1;
	}
}

static void settings_ui_reload_entries(void)
{
	g_settings_ui.count = 0;
	if (g_settings_ui.backend->get_count == NULL || g_settings_ui.backend->get_name == NULL) {
		g_settings_ui.inserted = -1;
		return;
	}

	uint16_t n = g_settings_ui.backend->get_count(g_settings_ui.backend);
	if (n > SETTINGS_UI_MAX_FLOPPY) {
		n = SETTINGS_UI_MAX_FLOPPY;
	}
	for (uint16_t i = 0; i < n; i++) {
		const char *name = g_settings_ui.backend->get_name(g_settings_ui.backend, i);
		if (name == NULL || name[0] == '\0') {
			(void)snprintf(g_settings_ui.file_name[i], sizeof(g_settings_ui.file_name[i]), "fd%02u.img", (unsigned)(i + 1));
		} else {
			(void)snprintf(g_settings_ui.file_name[i], sizeof(g_settings_ui.file_name[i]), "%s", name);
		}
	}
	g_settings_ui.count = n;
	settings_ui_sync_inserted();
	if (g_settings_ui.count == 0) {
		g_settings_ui.selected = -1;
	} else if (g_settings_ui.inserted >= 0 && (uint16_t)g_settings_ui.inserted < g_settings_ui.count) {
		g_settings_ui.selected = g_settings_ui.inserted;
	} else {
		g_settings_ui.selected = 0;
	}
}

static void settings_ui_toast_hide(void)
{
	if (g_settings_ui.toast_box != NULL) {
		lv_obj_delete(g_settings_ui.toast_box);
		g_settings_ui.toast_box = NULL;
		g_settings_ui.toast_label = NULL;
	}
	if (g_settings_ui.toast_timer != NULL) {
		lv_timer_delete(g_settings_ui.toast_timer);
		g_settings_ui.toast_timer = NULL;
	}
}

static void settings_ui_toast_timer_cb(lv_timer_t *t)
{
	(void)t;
	settings_ui_toast_hide();
}

static void settings_ui_show_toast(const char *msg, bool is_error)
{
	if (msg == NULL || msg[0] == '\0') {
		return;
	}
	settings_ui_toast_hide();

	lv_obj_t *layer = lv_layer_top();
	const int32_t sw = lv_obj_get_width(layer);

	g_settings_ui.toast_box = lv_obj_create(layer);
	lv_obj_clear_flag(g_settings_ui.toast_box, LV_OBJ_FLAG_SCROLLABLE);
	// lv_obj_set_style_border_width(g_settings_ui.toast_box, 1, 0);
	// lv_obj_set_style_border_color(g_settings_ui.toast_box, lv_color_white(), 0);
	lv_obj_set_style_bg_color(g_settings_ui.toast_box, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(g_settings_ui.toast_box, LV_OPA_COVER, 0);
	lv_obj_set_style_radius(g_settings_ui.toast_box, 14, 0);
	lv_obj_set_style_pad_left(g_settings_ui.toast_box, 12, 0);
	lv_obj_set_style_pad_right(g_settings_ui.toast_box, 12, 0);
	lv_obj_set_style_pad_top(g_settings_ui.toast_box, 5, 0);
	lv_obj_set_style_pad_bottom(g_settings_ui.toast_box, 5, 0);

	g_settings_ui.toast_label = lv_label_create(g_settings_ui.toast_box);
	lv_label_set_text(g_settings_ui.toast_label, msg);
	lv_obj_set_style_text_font(g_settings_ui.toast_label, &mono_opposans_18, 0);
	lv_obj_set_style_text_color(g_settings_ui.toast_label, lv_color_white(), 0);
	lv_obj_set_style_text_align(g_settings_ui.toast_label, LV_TEXT_ALIGN_CENTER, 0);

	// Content-aware toast size: fit text first, clamp if too wide.
	lv_obj_update_layout(g_settings_ui.toast_label);
	int32_t text_w = lv_obj_get_width(g_settings_ui.toast_label);
	int32_t text_h = lv_obj_get_height(g_settings_ui.toast_label);
	const int32_t pad_x = 24;
	const int32_t max_w = sw - 24;
	int32_t box_w = text_w + pad_x;
	if (box_w > max_w) {
		lv_obj_set_width(g_settings_ui.toast_label, max_w - pad_x);
		lv_label_set_long_mode(g_settings_ui.toast_label, LV_LABEL_LONG_DOT);
		lv_obj_update_layout(g_settings_ui.toast_label);
		text_h = lv_obj_get_height(g_settings_ui.toast_label);
		box_w = max_w;
	}
	lv_obj_set_width(g_settings_ui.toast_box, box_w);
	lv_obj_set_height(g_settings_ui.toast_box, text_h + 10);
	lv_obj_center(g_settings_ui.toast_label);

	lv_obj_align(g_settings_ui.toast_box, LV_ALIGN_BOTTOM_MID, 0, -52);
	lv_obj_move_foreground(g_settings_ui.toast_box);

	const uint32_t ms = is_error ? 1800 : 1200;
	g_settings_ui.toast_timer = lv_timer_create(settings_ui_toast_timer_cb, ms, NULL);
}

static void settings_ui_update_row_state(uint16_t idx)
{
	if (idx >= g_settings_ui.count) {
		return;
	}

	const bool active = (g_settings_ui.inserted >= 0) && ((uint16_t)g_settings_ui.inserted == idx);
	const bool selected = (g_settings_ui.selected >= 0) && ((uint16_t)g_settings_ui.selected == idx);
	lv_obj_t *status = g_settings_ui.status[idx];
	lv_obj_t *name = g_settings_ui.name[idx];
	if (status != NULL) {
		if (active) {
			lv_label_set_text(status, LV_SYMBOL_OK);
		} else {
			lv_label_set_text(status, "");
		}
		lv_obj_set_style_text_color(status, lv_color_black(), 0);
	}
	lv_obj_t *info = g_settings_ui.info_box[idx];
	if (info != NULL) {
		const lv_color_t bg = active ? lv_color_black() : lv_color_white();
		lv_obj_set_style_bg_color(info, bg, 0);
		lv_obj_set_style_bg_opa(info, LV_OPA_COVER, 0);
		lv_obj_set_style_border_width(info, (active || selected) ? 1 : 0, 0);
		lv_obj_set_style_border_color(info, lv_color_black(), 0);
		lv_obj_set_style_border_side(info, (active || selected) ? LV_BORDER_SIDE_FULL : LV_BORDER_SIDE_NONE, 0);
		if (status != NULL) {
			lv_obj_set_style_text_color(status, active ? lv_color_white() : lv_color_black(), 0);
		}
		if (name != NULL) {
			lv_obj_set_style_text_color(name, active ? lv_color_white() : lv_color_black(), 0);
		}
	}
}

static void settings_ui_refresh_all_rows(void)
{
	settings_ui_sync_inserted();
	if (g_settings_ui.count > 0 && (g_settings_ui.selected < 0 || (uint16_t)g_settings_ui.selected >= g_settings_ui.count)) {
		g_settings_ui.selected = 0;
	}
	for (uint16_t i = 0; i < g_settings_ui.count; i++) {
		settings_ui_update_row_state(i);
	}
	if (g_settings_ui.insert_action_btn != NULL) {
		if (g_settings_ui.selected >= 0 && (uint16_t)g_settings_ui.selected < g_settings_ui.count) {
			lv_obj_clear_state(g_settings_ui.insert_action_btn, LV_STATE_DISABLED);
		} else {
			lv_obj_add_state(g_settings_ui.insert_action_btn, LV_STATE_DISABLED);
		}
	}
}

static void settings_ui_redraw_rows_no_sync(void)
{
	if (g_settings_ui.count > 0 && (g_settings_ui.selected < 0 || (uint16_t)g_settings_ui.selected >= g_settings_ui.count)) {
		g_settings_ui.selected = 0;
	}
	for (uint16_t i = 0; i < g_settings_ui.count; i++) {
		settings_ui_update_row_state(i);
	}
	if (g_settings_ui.insert_action_btn != NULL) {
		if (g_settings_ui.selected >= 0 && (uint16_t)g_settings_ui.selected < g_settings_ui.count) {
			lv_obj_clear_state(g_settings_ui.insert_action_btn, LV_STATE_DISABLED);
		} else {
			lv_obj_add_state(g_settings_ui.insert_action_btn, LV_STATE_DISABLED);
		}
	}
}

static void on_floppy_entry_clicked(lv_event_t *e)
{
	const uint16_t idx = (uint16_t)(uintptr_t)lv_event_get_user_data(e);
	if (idx >= g_settings_ui.count) {
		return;
	}
	g_settings_ui.selected = (int16_t)idx;
	settings_ui_redraw_rows_no_sync();
}

static void on_insert_clicked(lv_event_t *e)
{
	(void)e;
	const uint16_t idx = (uint16_t)g_settings_ui.selected;
	if (idx >= g_settings_ui.count) {
		settings_ui_show_toast(UI_STR_SELECT_FLOPPY_FIRST, true);
		return;
	}
	if (g_settings_ui.backend->insert == NULL) {
		settings_ui_show_toast(UI_STR_INSERT_UNAVAILABLE, true);
		return;
	}

	char msg[96];
	msg[0] = '\0';
	const bool ok = g_settings_ui.backend->insert(g_settings_ui.backend, idx, msg, sizeof(msg));
	if (ok) {
		g_settings_ui.inserted = (int16_t)idx;
		settings_ui_redraw_rows_no_sync();
	} else {
		settings_ui_refresh_all_rows();
	}
	if (!ok) {
		settings_ui_show_toast((msg[0] != '\0') ? msg : UI_STR_INSERT_FAILED, true);
		return;
	}
	settings_ui_show_toast((msg[0] != '\0') ? msg : UI_STR_INSERTED, false);
}

static void on_back_clicked(lv_event_t *e)
{
	(void)e;
	g_settings_ui.exit_request = true;
}

static void on_reboot_clicked(lv_event_t *e)
{
	(void)e;
	if (g_settings_ui.backend->reboot == NULL) {
		settings_ui_show_toast(UI_STR_REBOOT_UNAVAILABLE, true);
		return;
	}
	settings_ui_show_toast(UI_STR_REBOOTING, false);
	g_settings_ui.backend->reboot(g_settings_ui.backend);
}

static void on_recover_update_clicked(lv_event_t *e)
{
	(void)e;
	/* Sets the one-shot flag and reboots; next boot skips USB Host so the
	 * USB-Serial-JTAG controller is exposed for browser flashing. Used for
	 * both firmware update and recovery. No toast: esp_restart() disables
	 * all interrupts first, so UI never gets a chance to render. */
	mach_s3_flash_mode_enter();
}

static lv_obj_t *create_action_btn(lv_obj_t *parent, const char *text)
{
	lv_obj_t *btn = lv_btn_create(parent);
	lv_obj_set_style_radius(btn, 8, 0);
	lv_obj_set_style_border_width(btn, 1, 0);
	lv_obj_set_style_border_color(btn, lv_color_black(), 0);
	lv_obj_set_style_bg_color(btn, lv_color_white(), 0);
	lv_obj_set_style_text_color(btn, lv_color_black(), 0);
	lv_obj_set_style_bg_color(btn, lv_color_black(), LV_PART_MAIN | LV_STATE_PRESSED);
	lv_obj_set_style_text_color(btn, lv_color_white(), LV_PART_MAIN | LV_STATE_PRESSED);
	lv_obj_set_style_bg_color(btn, lv_color_black(), LV_PART_MAIN | LV_STATE_DISABLED);
	lv_obj_set_style_text_color(btn, lv_color_white(), LV_PART_MAIN | LV_STATE_DISABLED);

	lv_obj_t *label = lv_label_create(btn);
	lv_label_set_text(label, text);
	lv_obj_set_style_text_font(label, &mono_opposans_18, 0);
	lv_obj_center(label);
	return btn;
}

static lv_obj_t *create_section_panel(lv_obj_t *screen, int32_t x, int32_t y, int32_t w, int32_t h)
{
	lv_obj_t *panel = lv_obj_create(screen);
	lv_obj_set_pos(panel, x, y);
	lv_obj_set_size(panel, w, h);
	lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_radius(panel, 14, 0);
	lv_obj_set_style_border_width(panel, 1, 0);
	lv_obj_set_style_border_color(panel, lv_color_black(), 0);
	lv_obj_set_style_bg_color(panel, lv_color_white(), 0);
	lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
	lv_obj_set_style_pad_all(panel, 10, 0);
	return panel;
}

static void style_quick_slider(lv_obj_t *slider)
{
	if (slider == NULL) {
		return;
	}

	lv_obj_set_height(slider, 14);
	lv_obj_set_style_radius(slider, 999, LV_PART_MAIN);
	lv_obj_set_style_bg_color(slider, lv_color_white(), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_MAIN);
	lv_obj_set_style_border_width(slider, 1, LV_PART_MAIN);
	lv_obj_set_style_border_color(slider, lv_color_black(), LV_PART_MAIN);
	lv_obj_set_style_pad_all(slider, 0, LV_PART_MAIN);

	lv_obj_set_style_radius(slider, 999, LV_PART_INDICATOR);
	lv_obj_set_style_bg_color(slider, lv_color_black(), LV_PART_INDICATOR);
	lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_INDICATOR);

	lv_obj_set_style_bg_color(slider, lv_color_white(), LV_PART_KNOB);
	lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_KNOB);
	lv_obj_set_style_border_width(slider, 1, LV_PART_KNOB);
	lv_obj_set_style_border_color(slider, lv_color_black(), LV_PART_KNOB);
	lv_obj_set_style_radius(slider, 999, LV_PART_KNOB);
}

static void on_backlight_slider_value_changed(lv_event_t *e)
{
	lv_obj_t *slider = lv_event_get_target_obj(e);
	if (g_settings_ui.backend->set_backlight != NULL) {
		const int value = lv_slider_get_value(slider);
		g_settings_ui.backend->set_backlight(g_settings_ui.backend, (uint8_t)value);
	}
	lv_obj_t *label = (lv_obj_t *)lv_obj_get_user_data(slider);
	if (label) {
		char buf[8];
		snprintf(buf, sizeof(buf), "%d%%", (int)lv_slider_get_value(slider));
		lv_label_set_text(label, buf);
	}
}

static void on_backlight_slider_released(lv_event_t *e)
{
	if (g_settings_ui.backend->commit_backlight == NULL) {
		return;
	}
	lv_obj_t *slider = lv_event_get_target_obj(e);
	const int value = lv_slider_get_value(slider);
	g_settings_ui.backend->commit_backlight(g_settings_ui.backend, (uint8_t)value);
}

static void on_volume_slider_value_changed(lv_event_t *e)
{
	lv_obj_t *slider = lv_event_get_target_obj(e);
	if (g_settings_ui.backend->set_volume != NULL) {
		const int value = lv_slider_get_value(slider);
		g_settings_ui.backend->set_volume(g_settings_ui.backend, (uint8_t)value);
	}
	lv_obj_t *label = (lv_obj_t *)lv_obj_get_user_data(slider);
	if (label) {
		char buf[8];
		snprintf(buf, sizeof(buf), "%d", (int)lv_slider_get_value(slider));
		lv_label_set_text(label, buf);
	}
}

static void on_volume_slider_released(lv_event_t *e)
{
	if (g_settings_ui.backend->commit_volume == NULL) {
		return;
	}
	lv_obj_t *slider = lv_event_get_target_obj(e);
	const int value = lv_slider_get_value(slider);
	g_settings_ui.backend->commit_volume(g_settings_ui.backend, (uint8_t)value);
}

bool mach_s3_settings_ui_take_exit_request(void)
{
	const bool r = g_settings_ui.exit_request;
	g_settings_ui.exit_request = false;
	return r;
}

void mach_s3_settings_ui_show(void)
{
	settings_ui_toast_hide();
	/* One insert per pause-menu session: reset the lock on entry. */
	if (g_settings_ui.backend != NULL)
		machine_backend_reset_insert_selected(g_settings_ui.backend);
	memset(g_settings_ui.status, 0, sizeof(g_settings_ui.status));
	memset(g_settings_ui.name, 0, sizeof(g_settings_ui.name));
	memset(g_settings_ui.info_box, 0, sizeof(g_settings_ui.info_box));
	g_settings_ui.insert_action_btn = NULL;
	settings_ui_toast_hide();
	settings_ui_reload_entries();
	g_settings_ui.exit_request = false;

	lv_obj_t *screen = lv_screen_active();
	lv_obj_clean(screen);
	lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

	const int32_t sw = lv_obj_get_width(screen);
	const int32_t sh = lv_obj_get_height(screen);

	const int32_t inset = 10;
	const int32_t gap = 10;
	const int32_t bottom_h = 52;
	const int32_t section_title_h = 22;

	const int32_t quick_h_target = 108;
	const int32_t floppy_reduce_h = 40;
	const int32_t floppy_min_h = 120;

	const int32_t row_h = 32;
	const int32_t floppy_split_gap = 8;
	const int32_t floppy_action_w = 100;
	const int32_t floppy_action_h = 28;

	const int32_t qs_top = 4;
	const int32_t qs_row_h = 34;
	const int32_t qs_label_w = 48;  /* "背光"/"音量" = 2 full-width chars (36px) + 12px slack */
	const int32_t qs_gap = 12;
	const int32_t qs_slider_y_offset = 7;


	const int32_t content_w = sw - inset * 2;
	/* Left column: Floppy Manager + Quick Settings. Right column: WiFi remote. */
	const int32_t left_w = content_w * 3 / 5;
	const int32_t right_w = content_w - left_w - gap;
	const int32_t left_x = inset;
	const int32_t right_x = inset + left_w + gap;
	const int32_t panel_w = left_w;
	const int32_t panel_x = left_x;
	int32_t quick_h = quick_h_target;
	int32_t floppy_h = sh - bottom_h - gap - quick_h - inset * 2 - floppy_reduce_h;
	if (floppy_h < floppy_min_h) {
		floppy_h = floppy_min_h;
		quick_h = sh - bottom_h - gap - floppy_h - inset * 2;
	}

	const int32_t total_h = floppy_h + gap + quick_h;
	const int32_t content_y = (sh - bottom_h - total_h) / 2 + 10;

	const int32_t floppy_title_y = content_y;
	const int32_t floppy_panel_y = content_y + section_title_h;
	const int32_t floppy_panel_h = floppy_h - section_title_h;

	const int32_t quick_settings_title_y = content_y + floppy_h + gap;
	const int32_t quick_settings_panel_y = quick_settings_title_y + section_title_h;
	const int32_t quick_settings_panel_h = quick_h - section_title_h;

	lv_obj_t *title = lv_label_create(screen);
	lv_label_set_text(title, UI_STR_PAUSE_MENU);
	lv_obj_set_style_text_color(title, lv_color_black(), 0);
	lv_obj_set_style_text_font(title, &mono_opposans_18, 0);
	lv_obj_set_style_transform_zoom(title, 320, 0);
	lv_obj_align(title, LV_ALIGN_TOP_MID, 0, inset);

	/* Top-left: firmware version + boot partition (factory/ota_0/ota_1) */
	{
		const esp_app_desc_t *app = esp_app_get_description();
		const esp_partition_t *run = esp_ota_get_running_partition();
		char meta[64];
		snprintf(meta, sizeof(meta), "v%s %s",
		         (app != NULL && app->version[0] != '\0') ? app->version : "?",
		         (run != NULL) ? run->label : "?");
		lv_obj_t *meta_label = lv_label_create(screen);
		lv_label_set_text(meta_label, meta);
		lv_obj_set_style_text_color(meta_label, lv_color_black(), 0);
		lv_obj_set_style_text_font(meta_label, &mono_opposans_14, 0);
		lv_obj_align(meta_label, LV_ALIGN_TOP_LEFT, inset, inset);
	}

	lv_obj_t *floppy_title = lv_label_create(screen);
	lv_label_set_text(floppy_title, UI_STR_FLOPPY_MANAGER);
	lv_obj_set_style_text_color(floppy_title, lv_color_black(), 0);
	lv_obj_set_style_text_font(floppy_title, &mono_opposans_18, 0);
	lv_obj_set_pos(floppy_title, panel_x + 2, floppy_title_y);

	lv_obj_t *floppy_panel = create_section_panel(screen, panel_x, floppy_panel_y, panel_w, floppy_panel_h);

	lv_obj_set_style_pad_top(floppy_panel, 4, 0);
	lv_obj_set_style_pad_right(floppy_panel, 4, 0);
	lv_obj_set_style_pad_left(floppy_panel, 4, 0);
	lv_obj_set_style_pad_bottom(floppy_panel, 4, 0);
	lv_obj_set_layout(floppy_panel, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(floppy_panel, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(floppy_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
	lv_obj_set_style_pad_column(floppy_panel, floppy_split_gap, 0);

	lv_obj_t *list = lv_list_create(floppy_panel);
	lv_obj_set_size(list, 10, lv_pct(100));
	lv_obj_set_flex_grow(list, 1);
	lv_obj_set_style_border_width(list, 1, 0);
	lv_obj_set_style_border_color(list, lv_color_black(), 0);
	lv_obj_set_style_radius(list, 0, 0);
	lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
	lv_obj_set_style_margin_top(list, 8, 0);
	lv_obj_set_style_margin_bottom(list, 8, 0);
	lv_obj_set_style_pad_row(list, 0, 0);
	lv_obj_set_style_margin_left(list, 8, 0);
	lv_obj_set_style_pad_top(list, 0, 0);
	lv_obj_set_style_pad_bottom(list, 0, 0);
	lv_obj_set_style_pad_left(list, 0, 0);
	lv_obj_set_scroll_dir(list, LV_DIR_VER);
	lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO);
	lv_obj_set_style_pad_right(list, 0, 0);
	lv_obj_set_style_pad_right(list, 0, LV_PART_SCROLLBAR);
	lv_obj_set_style_width(list, 4, LV_PART_SCROLLBAR);
	lv_obj_set_style_bg_color(list, lv_color_black(), LV_PART_SCROLLBAR);
	lv_obj_set_style_bg_opa(list, LV_OPA_COVER, LV_PART_SCROLLBAR);
	lv_obj_set_style_radius(list, 6, LV_PART_SCROLLBAR);

	lv_obj_t *action_col = lv_obj_create(floppy_panel);
	lv_obj_set_size(action_col, floppy_action_w, lv_pct(100));
	lv_obj_clear_flag(action_col, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_border_width(action_col, 0, 0);
	lv_obj_set_style_bg_opa(action_col, LV_OPA_TRANSP, 0);
	lv_obj_set_style_pad_left(action_col, 8, 0);
	lv_obj_set_style_pad_right(action_col, 14, 0);
	lv_obj_set_style_pad_top(action_col, 8, 0);
	lv_obj_set_style_pad_bottom(action_col, 0, 0);

	g_settings_ui.insert_action_btn = create_action_btn(action_col, UI_STR_INSERT);
	lv_obj_set_width(g_settings_ui.insert_action_btn, lv_pct(100));
	lv_obj_set_height(g_settings_ui.insert_action_btn, floppy_action_h);
	lv_obj_align(g_settings_ui.insert_action_btn, LV_ALIGN_TOP_MID, 0, 0);
	lv_obj_move_foreground(g_settings_ui.insert_action_btn);
	lv_obj_add_event_cb(g_settings_ui.insert_action_btn, on_insert_clicked, LV_EVENT_CLICKED, NULL);

	for (uint16_t i = 0; i < g_settings_ui.count; i++) {
		lv_obj_t *row = lv_list_add_button(list, NULL, "");
		lv_obj_clear_flag(row, LV_OBJ_FLAG_CHECKABLE);
		lv_obj_set_width(row, lv_pct(100));
		lv_obj_set_height(row, row_h);
		lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_set_style_radius(row, 0, 0);
		lv_obj_set_style_border_width(row, 1, LV_PART_MAIN);
		lv_obj_set_style_border_color(row, lv_color_white(), LV_PART_MAIN);
		lv_obj_set_style_border_side(row, LV_BORDER_SIDE_FULL, LV_PART_MAIN);
		lv_obj_set_style_bg_color(row, lv_color_white(), 0);
		lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
		lv_obj_set_style_pad_left(row, 6, 0);
		lv_obj_set_style_pad_right(row, 6, 0);
		lv_obj_set_style_pad_top(row, 3, 0);
		lv_obj_set_style_pad_bottom(row, 3, 0);
		lv_obj_set_style_pad_column(row, 6, 0);
		lv_obj_set_style_shadow_width(row, 0, 0);
		lv_obj_set_style_bg_color(row, lv_color_white(), LV_PART_MAIN | LV_STATE_PRESSED);
		lv_obj_set_style_bg_opa(row, LV_OPA_COVER, LV_STATE_PRESSED);
		lv_obj_set_style_border_width(row, 1, LV_PART_MAIN | LV_STATE_PRESSED);
		lv_obj_set_style_border_color(row, lv_color_black(), LV_PART_MAIN | LV_STATE_PRESSED);
		lv_obj_set_style_border_side(row, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_PRESSED);
		lv_obj_set_style_radius(row, 0, LV_PART_MAIN | LV_STATE_PRESSED);
		lv_obj_set_layout(row, LV_LAYOUT_FLEX);
		lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
		lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		lv_obj_set_style_transform_width(row, 0, LV_PART_MAIN | LV_STATE_PRESSED);
		lv_obj_add_event_cb(row, on_floppy_entry_clicked, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
		lv_obj_add_event_cb(row, on_floppy_entry_clicked, LV_EVENT_SHORT_CLICKED, (void *)(uintptr_t)i);
		g_settings_ui.info_box[i] = row;

		lv_obj_t *name = lv_obj_get_child(row, 0);
		if (name == NULL) {
			name = lv_obj_get_child(row, -1);
		}
		if (name == NULL) {
			name = lv_label_create(row);
		}
		lv_label_set_text(name, g_settings_ui.file_name[i]);
		lv_obj_set_style_text_color(name, lv_color_black(), 0);
		lv_obj_set_style_text_font(name, &mono_opposans_18, 0);
		lv_obj_set_width(name, lv_pct(75));
		lv_label_set_long_mode(name, LV_LABEL_LONG_DOT);
		lv_obj_set_flex_grow(name, 1);
		g_settings_ui.name[i] = name;

		lv_obj_t *status = lv_label_create(row);
		lv_label_set_text(status, "");
		lv_obj_set_style_text_font(status, &mono_opposans_18, 0);
		g_settings_ui.status[i] = status;
	}

	lv_obj_t *quick_settings_title = lv_label_create(screen);
	lv_label_set_text(quick_settings_title, UI_STR_QUICK_SETTINGS);
	lv_obj_set_style_text_color(quick_settings_title, lv_color_black(), 0);
	lv_obj_set_style_text_font(quick_settings_title, &mono_opposans_18, 0);
	lv_obj_set_pos(quick_settings_title, panel_x + 2, quick_settings_title_y);

	/* WiFi remote panel: right column, spans the full left-column height. */
	const int32_t wifi_panel_y = floppy_panel_y;
	const int32_t wifi_panel_h = (quick_settings_panel_y + quick_settings_panel_h) - floppy_panel_y;
	wifi_panel_create(screen, right_x, wifi_panel_y, right_w, wifi_panel_h);

	lv_obj_t *quick_settings_panel = create_section_panel(screen, panel_x, quick_settings_panel_y, panel_w, quick_settings_panel_h);
	/* Row: [title(96)] gap(12) [slider] gap(16) [val_label(~40)] margin(8) */
	const int32_t qs_control_x = qs_label_w + qs_gap;               /* =108 */
	const int32_t qs_val_x = panel_w - 20 - 48;                     /* value label left edge: panel content - 48px */
	int32_t qs_control_w = qs_val_x - qs_control_x - 16;            /* slider fills between with 16px gap (clear of knob) */

	lv_obj_t *bl_label = lv_label_create(quick_settings_panel);
	lv_label_set_text(bl_label, UI_STR_BACKLIGHT);
	lv_obj_set_style_text_color(bl_label, lv_color_black(), 0);
	lv_obj_set_style_text_font(bl_label, &mono_opposans_18, 0);
	lv_obj_set_width(bl_label, qs_label_w);
	lv_obj_set_style_text_align(bl_label, LV_TEXT_ALIGN_LEFT, 0);
	lv_obj_set_pos(bl_label, 0, qs_top + qs_slider_y_offset - 4);

	lv_obj_t *bl_slider = lv_slider_create(quick_settings_panel);
	lv_obj_set_width(bl_slider, qs_control_w);
	lv_obj_set_pos(bl_slider, qs_control_x, qs_top + qs_slider_y_offset);
	style_quick_slider(bl_slider);
	lv_slider_set_range(bl_slider, 0, 100);
	int bl_value_i = (int)SETTINGS_UI_BACKLIGHT_DEFAULT;
	if (g_settings_ui.backend->get_backlight != NULL) {
		bl_value_i = (int)g_settings_ui.backend->get_backlight(g_settings_ui.backend);
		if (bl_value_i < 0) {
			bl_value_i = 0;
		} else if (bl_value_i > 100) {
			bl_value_i = 100;
		}
	}
	lv_slider_set_value(bl_slider, bl_value_i, LV_ANIM_OFF);
	lv_obj_add_event_cb(bl_slider, on_backlight_slider_value_changed, LV_EVENT_VALUE_CHANGED, NULL);
	lv_obj_add_event_cb(bl_slider, on_backlight_slider_released, LV_EVENT_RELEASED, NULL);
	lv_obj_add_event_cb(bl_slider, on_backlight_slider_released, LV_EVENT_PRESS_LOST, NULL);
	/* Backlight value label */
	lv_obj_t *bl_val_label = lv_label_create(quick_settings_panel);
	{
		char buf[8];
		snprintf(buf, sizeof(buf), "%d%%", bl_value_i);
		lv_label_set_text(bl_val_label, buf);
	}
	lv_obj_set_style_text_color(bl_val_label, lv_color_black(), 0);
	lv_obj_set_style_text_font(bl_val_label, &mono_opposans_18, 0);
	lv_obj_set_pos(bl_val_label, qs_val_x, qs_top + qs_slider_y_offset - 4);
	lv_obj_set_user_data(bl_slider, bl_val_label);

	lv_obj_t *vol_label = lv_label_create(quick_settings_panel);
	lv_label_set_text(vol_label, UI_STR_VOLUME);
	lv_obj_set_style_text_color(vol_label, lv_color_black(), 0);
	lv_obj_set_style_text_font(vol_label, &mono_opposans_18, 0);
	lv_obj_set_width(vol_label, qs_label_w);
	lv_obj_set_style_text_align(vol_label, LV_TEXT_ALIGN_LEFT, 0);
	lv_obj_set_pos(vol_label, 0, qs_top + qs_row_h + qs_slider_y_offset - 4);

	lv_obj_t *vol_slider = lv_slider_create(quick_settings_panel);
	lv_obj_set_width(vol_slider, qs_control_w);
	lv_obj_set_pos(vol_slider, qs_control_x, qs_top + qs_row_h + qs_slider_y_offset);
	style_quick_slider(vol_slider);
	lv_slider_set_range(vol_slider, 0, (int32_t)SETTINGS_UI_VOLUME_MAX);
	int vol_value_i = (int)SETTINGS_UI_VOLUME_DEFAULT;
	if (g_settings_ui.backend->get_volume != NULL) {
		vol_value_i = (int)g_settings_ui.backend->get_volume(g_settings_ui.backend);
		if (vol_value_i < 0) {
			vol_value_i = 0;
		} else if (vol_value_i > (int)SETTINGS_UI_VOLUME_MAX) {
			vol_value_i = (int)SETTINGS_UI_VOLUME_MAX;
		}
	}
	lv_slider_set_value(vol_slider, vol_value_i, LV_ANIM_OFF);
	lv_obj_add_event_cb(vol_slider, on_volume_slider_value_changed, LV_EVENT_VALUE_CHANGED, NULL);
	lv_obj_add_event_cb(vol_slider, on_volume_slider_released, LV_EVENT_RELEASED, NULL);
	lv_obj_add_event_cb(vol_slider, on_volume_slider_released, LV_EVENT_PRESS_LOST, NULL);
	/* Volume value label */
	lv_obj_t *vol_val_label = lv_label_create(quick_settings_panel);
	{
		char buf[8];
		snprintf(buf, sizeof(buf), "%d", vol_value_i);
		lv_label_set_text(vol_val_label, buf);
	}
	lv_obj_set_style_text_color(vol_val_label, lv_color_black(), 0);
	lv_obj_set_style_text_font(vol_val_label, &mono_opposans_18, 0);
	lv_obj_set_pos(vol_val_label, qs_val_x, qs_top + qs_row_h + qs_slider_y_offset - 4);
	lv_obj_set_user_data(vol_slider, vol_val_label);

	const int32_t btn_w = 140;
	const int32_t btn_gap = 12;
	const int32_t btn_h = 38;
	/* Bottom row: Resume | Reboot */
	lv_obj_t *resume_btn = create_action_btn(screen, UI_STR_RESUME);
	lv_obj_set_size(resume_btn, btn_w, btn_h);
	lv_obj_align(resume_btn, LV_ALIGN_BOTTOM_MID, -(btn_w + btn_gap) / 2, -10);
	lv_obj_add_event_cb(resume_btn, on_back_clicked, LV_EVENT_CLICKED, NULL);

	lv_obj_t *reboot_btn = create_action_btn(screen, UI_STR_REBOOT);
	lv_obj_set_size(reboot_btn, btn_w, btn_h);
	lv_obj_align(reboot_btn, LV_ALIGN_BOTTOM_MID, (btn_w + btn_gap) / 2, -10);
	lv_obj_add_event_cb(reboot_btn, on_reboot_clicked, LV_EVENT_CLICKED, NULL);

	/* Top-right: Recover / Update — placed away from the common actions to
	 * prevent accidental taps. Same style as the other buttons, kept small
	 * and unobtrusive (14px font). */
	lv_obj_t *recover_btn = create_action_btn(screen, UI_STR_RECOVER_UPDATE);
	lv_obj_set_size(recover_btn, 100, 26);
	lv_obj_set_style_pad_all(recover_btn, 0, 0);
	lv_obj_set_style_text_font(lv_obj_get_child(recover_btn, 0), &mono_opposans_14, 0);
	lv_obj_align(recover_btn, LV_ALIGN_TOP_RIGHT, -inset, inset);
	lv_obj_add_event_cb(recover_btn, on_recover_update_clicked, LV_EVENT_CLICKED, NULL);

	settings_ui_refresh_all_rows();
}

