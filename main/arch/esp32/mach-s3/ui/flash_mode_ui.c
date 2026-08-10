#include "flash_mode_ui.h"

#include "esp_log.h"
#include "lvgl.h"
#include "ui_strings.h"

static const char *TAG = "flash_mode_ui";

extern const lv_font_t mono_opposans_18;

static lv_obj_t *ui_screen = NULL;

void flash_mode_ui_show(void)
{
	if (ui_screen != NULL) {
		lv_obj_del(ui_screen);
	}

	ESP_LOGI(TAG, "Showing flash mode UI");

	/* Full-screen white background (same palette as upgrade UI) */
	ui_screen = lv_obj_create(NULL);
	lv_scr_load(ui_screen);
	lv_obj_set_style_bg_color(ui_screen, lv_color_white(), 0);
	lv_obj_set_style_pad_all(ui_screen, 0, 0);

	/* Hide mouse cursors: no input in flash mode */
	lv_indev_t *indev = lv_indev_get_next(NULL);
	while (indev != NULL) {
		lv_obj_t *cursor = lv_indev_get_cursor(indev);
		if (cursor != NULL) {
			lv_obj_add_flag(cursor, LV_OBJ_FLAG_HIDDEN);
		}
		indev = lv_indev_get_next(indev);
	}

	/* ── Title ─────────────────────────────────────────────────── */
	lv_obj_t *title = lv_label_create(ui_screen);
	lv_label_set_text(title, UI_STR_RECOVER_UPDATE);
	lv_obj_set_style_text_color(title, lv_color_black(), 0);
	lv_obj_set_style_text_font(title, &mono_opposans_18, 0);
	lv_obj_align(title, LV_ALIGN_CENTER, 0, -80);

	/* ── Steps ────────────────────────────────────────────────── */
	static const char *k_steps[] = {
	        UI_STR_FLASH_STEP1,
	        UI_STR_FLASH_STEP2,
	        UI_STR_FLASH_STEP3,
	        UI_STR_FLASH_EXIT,
	};
	for (int i = 0; i < 4; i++) {
		lv_obj_t *step = lv_label_create(ui_screen);
		lv_label_set_text(step, k_steps[i]);
		lv_obj_set_style_text_color(step, lv_color_black(), 0);
		lv_obj_set_style_text_font(step, &mono_opposans_18, 0);
		lv_obj_align(step, LV_ALIGN_CENTER, 0, -20 + i * 32);
	}

	lv_timer_handler();
}
