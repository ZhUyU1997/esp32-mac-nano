#include "upgrade_ui.h"

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "upgrade_ui";

extern const lv_font_t mono_opposans_18;

/* Singleton UI elements */
static lv_obj_t *ui_screen = NULL;
static lv_obj_t *ui_bar = NULL;
static lv_obj_t *ui_stage_label = NULL;
static lv_obj_t *ui_error_label = NULL;

/* ── Styles ────────────────────────────────────────────────────────── */
static lv_style_t style_bar_main;
static lv_style_t style_bar_indicator;

static void upgrade_ui_styles_init(void)
{
	lv_style_init(&style_bar_main);
	lv_style_set_bg_color(&style_bar_main, lv_color_white());
	lv_style_set_border_color(&style_bar_main, lv_color_black());
	lv_style_set_border_width(&style_bar_main, 2);
	lv_style_set_radius(&style_bar_main, 10);
	lv_style_set_pad_all(&style_bar_main, 3);
	lv_style_set_width(&style_bar_main, 260);
	lv_style_set_height(&style_bar_main, 28);

	lv_style_init(&style_bar_indicator);
	lv_style_set_bg_color(&style_bar_indicator, lv_color_black());
	lv_style_set_radius(&style_bar_indicator, 8);
}

/* ── Public API ──────────────────────────────────────────────────────── */

void upgrade_ui_show(void)
{
	if (ui_screen != NULL) {
		upgrade_ui_close();
	}

	ESP_LOGI(TAG, "Showing upgrade UI");

	/* Full-screen white background */
	ui_screen = lv_obj_create(NULL);
	lv_scr_load(ui_screen);
	lv_obj_set_style_bg_color(ui_screen, lv_color_white(), 0);
	lv_obj_set_style_pad_all(ui_screen, 0, 0);

	/* Hide all mouse cursors during upgrade */
	lv_indev_t *indev = lv_indev_get_next(NULL);
	while (indev != NULL) {
		lv_obj_t *cursor = lv_indev_get_cursor(indev);
		if (cursor != NULL) {
			lv_obj_add_flag(cursor, LV_OBJ_FLAG_HIDDEN);
		}
		indev = lv_indev_get_next(indev);
	}

	/* ── Title (black on white) ───────────────────────────────────── */
	lv_obj_t *title = lv_label_create(ui_screen);
	lv_label_set_text(title, "System Upgrade");
	lv_obj_set_style_text_color(title, lv_color_black(), 0);
	lv_obj_set_style_text_font(title, &mono_opposans_18, 0);
	lv_obj_align(title, LV_ALIGN_CENTER, 0, -70);

	/* ── Progress bar (white track, black fill) ───────────────────── */
	upgrade_ui_styles_init();

	ui_bar = lv_bar_create(ui_screen);
	lv_obj_add_style(ui_bar, &style_bar_main, 0);
	lv_obj_add_style(ui_bar, &style_bar_indicator, LV_PART_INDICATOR);
	lv_bar_set_range(ui_bar, 0, 100);
	lv_bar_set_value(ui_bar, 0, LV_ANIM_OFF);
	lv_obj_align(ui_bar, LV_ALIGN_CENTER, 0, 0);

	/* ── Stage label (black text below bar) ───────────────────────── */
	ui_stage_label = lv_label_create(ui_screen);
	lv_label_set_text(ui_stage_label, "Preparing...");
	lv_obj_set_style_text_color(ui_stage_label, lv_color_black(), 0);
	lv_obj_set_style_text_font(ui_stage_label, &mono_opposans_18, 0);
	lv_obj_align(ui_stage_label, LV_ALIGN_CENTER, 0, 45);

	/* ── Error label (hidden initially) ───────────────────────────── */
	ui_error_label = lv_label_create(ui_screen);
	lv_label_set_text(ui_error_label, "");
	lv_obj_set_style_text_color(ui_error_label, lv_color_black(), 0);
	lv_obj_set_style_text_font(ui_error_label, &mono_opposans_18, 0);
	lv_obj_align(ui_error_label, LV_ALIGN_CENTER, 0, 75);
	lv_obj_add_flag(ui_error_label, LV_OBJ_FLAG_HIDDEN);

	lv_timer_handler();
}

void upgrade_ui_update(int percent, const char *stage)
{
	if (percent == -1) {
		/* Error state: just show the error text */
		if (ui_error_label != NULL && stage != NULL) {
			lv_label_set_text(ui_error_label, stage);
			lv_obj_remove_flag(ui_error_label, LV_OBJ_FLAG_HIDDEN);
		}
		lv_timer_handler();
		return;
	}

	if (percent < 0)
		percent = 0;
	if (percent > 100)
		percent = 100;

	if (ui_bar != NULL) {
		lv_bar_set_value(ui_bar, percent, LV_ANIM_OFF);
	}

	if (ui_stage_label != NULL && stage != NULL) {
		lv_label_set_text(ui_stage_label, stage);
	}

	lv_timer_handler();
}

void upgrade_ui_close(void)
{
	if (ui_screen != NULL) {
		/* Restore mouse cursors */
		lv_indev_t *indev = lv_indev_get_next(NULL);
		while (indev != NULL) {
			lv_obj_t *cursor = lv_indev_get_cursor(indev);
			if (cursor != NULL) {
				lv_obj_remove_flag(cursor, LV_OBJ_FLAG_HIDDEN);
			}
			indev = lv_indev_get_next(indev);
		}
		lv_obj_del(ui_screen);
		ui_screen = NULL;
		ui_bar = NULL;
		ui_stage_label = NULL;
		ui_error_label = NULL;
	}
}
