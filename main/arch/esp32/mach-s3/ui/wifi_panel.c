/*
 * Pause-menu WiFi remote panel (1-bit LVGL).
 *
 * Two states, different user focus:
 *  - disabled: guide — how to turn on (long-press the rear menu button)
 *  - enabled:  info — SSID/password/connection status, how to turn off
 * State is driven by wifi_panel_set_enabled(); refresh() re-reads live info.
 */
#include <stdio.h>
#include "lvgl.h"
#include "wifi_panel.h"
#include "web-control.h"

#define WIFI_SSID "MacNano-ESP32"
#define WIFI_PASS "mac-nano"
#define WIFI_IP   "192.168.4.1"

typedef struct {
	lv_obj_t *panel;
	lv_obj_t *status;
	lv_obj_t *keys;
	lv_obj_t *vals;
	lv_obj_t *hint;
	lv_obj_t *sw;
	bool enabled;
} wifi_panel_t;

extern const lv_font_t mono_opposans_18;
extern const lv_font_t lv_font_montserrat_14;

/* Composite font: mono_opposans_18 everywhere, arrow glyphs (LV_SYMBOL_*)
 * resolved via fallback to the LVGL symbol font. */
static lv_font_t s_panel_font;
static const lv_font_t *panel_font(void)
{
	if (s_panel_font.fallback == NULL) {
		s_panel_font = mono_opposans_18;
		s_panel_font.fallback = &lv_font_montserrat_14;
	}
	return &s_panel_font;
}

static wifi_panel_t s_panel;

static lv_obj_t *create_panel(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h)
{
	lv_obj_t *panel = lv_obj_create(parent);
	lv_obj_set_pos(panel, x, y);
	lv_obj_set_size(panel, w, h);
	lv_obj_set_style_radius(panel, 14, 0);
	lv_obj_set_style_border_width(panel, 1, 0);
	lv_obj_set_style_border_color(panel, lv_color_black(), 0);
	lv_obj_set_style_bg_color(panel, lv_color_white(), 0);
	lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
	lv_obj_set_style_pad_all(panel, 10, 0);
	return panel;
}

/* Placeholder until the Wi-Fi backend is wired up: the switch flips the
 * panel state so the UI can be reviewed. Replaced by real toggle later. */
static void on_switch_changed(lv_event_t *e)
{
	(void)e;
	wifi_panel_set_enabled(!s_panel.enabled);
}

void wifi_panel_create(lv_obj_t *screen, int32_t x, int32_t y, int32_t w, int32_t h)
{
	lv_obj_t *title = lv_label_create(screen);
	lv_label_set_text(title, "WiFi Keyboard/Mouse");
	lv_obj_set_style_text_color(title, lv_color_black(), 0);
	lv_obj_set_style_text_font(title, &mono_opposans_18, 0);
	lv_obj_set_pos(title, x + 2, y - 22);

	s_panel.panel = create_panel(screen, x, y, w, h);

	/* Switch, top-right inside the panel (ble-pause-menu style). */
	const int32_t sw_w = 44;
	const int32_t sw_h = 24;
	s_panel.sw = lv_switch_create(s_panel.panel);
	lv_obj_set_size(s_panel.sw, sw_w, sw_h);
	lv_obj_align(s_panel.sw, LV_ALIGN_TOP_RIGHT, -10, 8);
	lv_obj_set_style_bg_color(s_panel.sw, lv_color_white(), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(s_panel.sw, LV_OPA_COVER, LV_PART_MAIN);
	lv_obj_set_style_border_width(s_panel.sw, 1, LV_PART_MAIN);
	lv_obj_set_style_border_color(s_panel.sw, lv_color_black(), LV_PART_MAIN);
	/* NOTE: no LV_PART_INDICATOR style here — with no LVGL theme applied,
	 * the switch draws the indicator over the whole content area regardless
	 * of the checked state, so any indicator color would stay visible
	 * permanently. State is shown via track color + knob instead. */
	lv_obj_set_style_bg_color(s_panel.sw, lv_color_white(), LV_PART_KNOB);
	lv_obj_set_style_bg_opa(s_panel.sw, LV_OPA_COVER, LV_PART_KNOB);
	lv_obj_set_style_border_width(s_panel.sw, 1, LV_PART_KNOB);
	lv_obj_set_style_border_color(s_panel.sw, lv_color_black(), LV_PART_KNOB);
	lv_obj_set_style_pad_all(s_panel.sw, 3, LV_PART_KNOB);
	lv_obj_add_event_cb(s_panel.sw, on_switch_changed, LV_EVENT_VALUE_CHANGED, NULL);

	/* Status text (Off/On), same row as the switch (top line). */
	s_panel.status = lv_label_create(s_panel.panel);
	lv_obj_set_style_text_color(s_panel.status, lv_color_black(), 0);
	lv_obj_set_style_text_font(s_panel.status, panel_font(), 0);
	lv_obj_set_pos(s_panel.status, 0, 12);

	/* Two-column layout: key labels left-aligned, values left-aligned.
	 * The font is not monospace, so space padding cannot align values. */
	s_panel.keys = lv_label_create(s_panel.panel);
	lv_obj_set_pos(s_panel.keys, 0, 48);
	lv_obj_set_width(s_panel.keys, 60);
	lv_label_set_long_mode(s_panel.keys, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_color(s_panel.keys, lv_color_black(), 0);
	lv_obj_set_style_text_font(s_panel.keys, panel_font(), 0);
	lv_obj_set_style_text_align(s_panel.keys, LV_TEXT_ALIGN_LEFT, 0);
	lv_obj_set_style_text_line_space(s_panel.keys, 6, 0);

	s_panel.vals = lv_label_create(s_panel.panel);
	lv_obj_set_pos(s_panel.vals, 70, 48);
	lv_obj_set_width(s_panel.vals, lv_pct(100));
	lv_label_set_long_mode(s_panel.vals, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_color(s_panel.vals, lv_color_black(), 0);
	lv_obj_set_style_text_font(s_panel.vals, panel_font(), 0);
	lv_obj_set_style_text_line_space(s_panel.vals, 6, 0);

	/* Hint row: arrow glyphs from the LVGL built-in symbol font. */
	s_panel.hint = lv_label_create(s_panel.panel);
	lv_obj_set_width(s_panel.hint, w - 20);
	lv_label_set_long_mode(s_panel.hint, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_color(s_panel.hint, lv_color_black(), 0);
	lv_obj_set_style_text_font(s_panel.hint, panel_font(), 0);

	s_panel.enabled = web_control_is_enabled();
	wifi_panel_refresh();
}

void wifi_panel_set_enabled(bool enabled)
{
	esp_err_t err = ESP_OK;
	if (enabled)
		err = web_control_enable();
	else
		web_control_disable();
	if (err == ESP_OK)
		s_panel.enabled = enabled;
	wifi_panel_refresh();
}

void wifi_panel_refresh(void)
{
	/* Truth is web_control_is_enabled(): auto-off may have disabled the
	 * stack without the panel knowing. */
	s_panel.enabled = web_control_is_enabled();
	if (s_panel.enabled) {
		lv_obj_add_state(s_panel.sw, LV_STATE_CHECKED);
		/* on: black track + white knob */
		lv_obj_set_style_bg_color(s_panel.sw, lv_color_black(), LV_PART_MAIN);
		lv_obj_set_style_bg_color(s_panel.sw, lv_color_white(), LV_PART_KNOB);
	} else {
		lv_obj_clear_state(s_panel.sw, LV_STATE_CHECKED);
		/* off: white track + black knob */
		lv_obj_set_style_bg_color(s_panel.sw, lv_color_white(), LV_PART_MAIN);
		lv_obj_set_style_bg_color(s_panel.sw, lv_color_black(), LV_PART_KNOB);
	}

	lv_label_set_text(s_panel.status, s_panel.enabled ? "On" : "Off");

	lv_label_set_text(s_panel.keys, "WiFi:\nKey:\nIP:");

	if (!s_panel.enabled) {
		lv_label_set_text(s_panel.vals, "");
		/* Off: hide the key/value labels — nothing to show. */
		lv_obj_add_flag(s_panel.keys, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(s_panel.vals, LV_OBJ_FLAG_HIDDEN);
	} else {
		lv_obj_clear_flag(s_panel.keys, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(s_panel.vals, LV_OBJ_FLAG_HIDDEN);
		lv_label_set_text(s_panel.vals,
		                  WIFI_SSID "\n" WIFI_PASS "\n" WIFI_IP);
	}
	/* set text first, then align so the wrapped height is placed correctly */
	lv_label_set_text(s_panel.hint,
	                  "Rear button:\n  Push " LV_SYMBOL_RIGHT " = On\n  Push " LV_SYMBOL_LEFT " = Off");
	lv_obj_align(s_panel.hint, LV_ALIGN_BOTTOM_LEFT, 10, -10);
}
