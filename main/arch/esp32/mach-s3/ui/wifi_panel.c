/*
 * Pause-menu Network panel (1-bit LVGL) — provisioning hub (P4).
 *
 * Five elements driven by wifi_state_t (docs/wifi-apsta-design.md P4):
 *   status row / info rows (WiFi: Key: IP:) / guide line / button / switch.
 *   OFF       hides the info rows: guide + [Provision] button only.
 *   PROVISION hotspot identity (SSID/Key/IP), no guide line — the captive
 *             portal auto-opens the config page on the connected phone.
 *   CONNECTED home SSID + LAN IP + mDNS: macnano.local.
 *   CONNECTING/RECONNECTING home SSID, IP "-" until GOT_IP.
 *   AP_ONLY   hotspot identity (no entry point today; kept for T8).
 * Button = [Provision] on OFF, [Re-provision] otherwise, hidden while
 * provisioning — same path as long-press F12 (web_control_reprovision).
 * Switch is a status indicator (control unbound from the 3-way switch).
 *
 * Text is English: panel fonts are ASCII-only (mono_opposans_18),
 * matching the rest of the pause menu. 1s lv_timer drives refresh().
 */
#include <stdio.h>
#include <string.h>
#include "lvgl.h"
#include "wifi_panel.h"
#include "web-control.h"
#include "ui_strings.h"

typedef struct {
	lv_obj_t *panel;
	lv_obj_t *status;
	lv_obj_t *keys;
	lv_obj_t *vals;
	lv_obj_t *hint;
	lv_obj_t *btn;
	lv_obj_t *btn_label;
	lv_obj_t *sw;
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
static lv_timer_t *s_timer; /* 1s refresh; created once, survives show/hide */

static void wifi_panel_timer_cb(lv_timer_t *t);

/* The pause menu rebuilds all objects on show (settings_ui lv_obj_clean);
 * mark the panel dead so refresh()/hold-hint from outside the menu cannot
 * touch destroyed labels. */
static void on_panel_delete(lv_event_t *e)
{
	(void)e;
	s_panel.panel = NULL;
}

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

/* [Provision] / [Re-provision]: same path as long-press F12 (T14). */
static void on_provision_btn(lv_event_t *e)
{
	(void)e;
	web_control_reprovision();
}

/* Switch is a status indicator only (P4: 3-way switch unbound); the
 * callback is kept for API completeness. */
static void on_switch_changed(lv_event_t *e)
{
	(void)e;
	wifi_panel_set_enabled(!web_control_is_enabled());
}

void wifi_panel_create(lv_obj_t *screen, int32_t x, int32_t y, int32_t w, int32_t h)
{
	lv_obj_t *title = lv_label_create(screen);
	lv_label_set_text(title, UI_STR_NETWORK);
	lv_obj_set_style_text_color(title, lv_color_black(), 0);
	lv_obj_set_style_text_font(title, &mono_opposans_18, 0);
	lv_obj_set_pos(title, x + 2, y - 22);

	s_panel.panel = create_panel(screen, x, y, w, h);
	lv_obj_add_event_cb(s_panel.panel, on_panel_delete, LV_EVENT_DELETE, NULL);

	/* Switch, top-right inside the panel (status indicator). */
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

	/* Status text, same row as the switch (top line). */
	s_panel.status = lv_label_create(s_panel.panel);
	lv_obj_set_style_text_color(s_panel.status, lv_color_black(), 0);
	lv_obj_set_style_text_font(s_panel.status, panel_font(), 0);
	lv_obj_set_pos(s_panel.status, 0, 12);

	/* Two-column layout: key labels left-aligned, values left-aligned.
	 * Keys are dynamic per state (refresh sets rows to match vals):
	 *   hotspot (3) / connected (4, +mDNS) / connecting, reconnecting (2). */
	s_panel.keys = lv_label_create(s_panel.panel);
	lv_label_set_text(s_panel.keys, UI_STR_HOTSPOT_KEYS);
	lv_obj_set_pos(s_panel.keys, 0, 48);
	/* Auto width: no fixed width, so the label hugs its longest row
	 * ("mDNS：" 71.8px > any fixed 70px would wrap). */
	lv_obj_set_style_text_color(s_panel.keys, lv_color_black(), 0);
	lv_obj_set_style_text_font(s_panel.keys, panel_font(), 0);
	lv_obj_set_style_text_align(s_panel.keys, LV_TEXT_ALIGN_LEFT, 0);
	lv_obj_set_style_text_line_space(s_panel.keys, 6, 0);

	s_panel.vals = lv_label_create(s_panel.panel);
	lv_obj_align_to(s_panel.vals, s_panel.keys, LV_ALIGN_OUT_RIGHT_TOP, 8, 0);
	lv_obj_set_width(s_panel.vals, lv_pct(100));
	lv_label_set_long_mode(s_panel.vals, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_color(s_panel.vals, lv_color_black(), 0);
	lv_obj_set_style_text_font(s_panel.vals, panel_font(), 0);
	lv_obj_set_style_text_line_space(s_panel.vals, 6, 0);

	/* Provision / Re-provision button (hidden while provisioning). The
	 * panel is a narrow column (~238px wide): the guide line wraps to two
	 * lines below the button — keep the button clear of it. */
	s_panel.btn = lv_btn_create(s_panel.panel);
	lv_obj_set_size(s_panel.btn, 130, 32);
	lv_obj_align(s_panel.btn, LV_ALIGN_BOTTOM_MID, 0, -70);
	lv_obj_set_style_bg_color(s_panel.btn, lv_color_white(), 0);
	lv_obj_set_style_bg_opa(s_panel.btn, LV_OPA_COVER, 0);
	lv_obj_set_style_border_width(s_panel.btn, 1, 0);
	lv_obj_set_style_border_color(s_panel.btn, lv_color_black(), 0);
	lv_obj_set_style_radius(s_panel.btn, 8, 0);
	lv_obj_set_style_text_color(s_panel.btn, lv_color_black(), 0);
	lv_obj_set_style_text_font(s_panel.btn, panel_font(), 0);
	lv_obj_add_event_cb(s_panel.btn, on_provision_btn, LV_EVENT_CLICKED, NULL);
	s_panel.btn_label = lv_label_create(s_panel.btn);
	lv_label_set_text(s_panel.btn_label, UI_STR_PROVISION);
	lv_obj_center(s_panel.btn_label);

	/* Guide line (bottom). */
	s_panel.hint = lv_label_create(s_panel.panel);
	lv_obj_set_width(s_panel.hint, w - 20);
	lv_label_set_long_mode(s_panel.hint, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_color(s_panel.hint, lv_color_black(), 0);
	lv_obj_set_style_text_font(s_panel.hint, panel_font(), 0);
	lv_obj_set_style_text_align(s_panel.hint, LV_TEXT_ALIGN_CENTER, 0);

	/* 1s live refresh (P4): wifi state can change at any time (connect,
	 * drop, provisioning…). Created once — the pause menu rebuilds objects
	 * on every show, a fresh timer each time would leak. */
	if (s_timer == NULL)
		s_timer = lv_timer_create(wifi_panel_timer_cb, 1000, NULL);

	wifi_panel_refresh();
}

void wifi_panel_set_enabled(bool enabled)
{
	esp_err_t err = ESP_OK;
	if (enabled)
		err = web_control_enable();
	else
		web_control_disable();
	(void)err;
	wifi_panel_refresh();
}

static const char *state_text(wifi_state_t st)
{
	switch (st) {
	case WIFI_STATE_OFF: return UI_STR_STATE_OFF;
	case WIFI_STATE_PROVISIONING: return UI_STR_STATE_PROVISIONING;
	case WIFI_STATE_CONNECTING: return UI_STR_STATE_CONNECTING;
	case WIFI_STATE_CONNECTED: return UI_STR_STATE_CONNECTED;
	case WIFI_STATE_RECONNECTING: return UI_STR_STATE_RECONNECTING;
	case WIFI_STATE_AP_ONLY: return UI_STR_STATE_AP_ONLY;
	}
	return "?";
}

static void wifi_panel_timer_cb(lv_timer_t *t)
{
	(void)t;
	wifi_panel_refresh();
}

void wifi_panel_refresh(void)
{
	if (s_panel.panel == NULL)
		return; /* panel not created / menu closed — nothing to draw */

	const wifi_state_t st = web_control_state();
	const bool enabled = web_control_is_enabled();

	/* switch: status indicator */
	if (enabled) {
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

	/* status row */
	lv_label_set_text(s_panel.status, state_text(st));

	/* info rows: hidden on OFF */
	char sta_ssid[33], sta_ip[17];
	web_control_sta_info(sta_ssid, sizeof(sta_ssid), sta_ip, sizeof(sta_ip));
	if (st == WIFI_STATE_OFF) {
		lv_obj_add_flag(s_panel.keys, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(s_panel.vals, LV_OBJ_FLAG_HIDDEN);
	} else {
		lv_obj_clear_flag(s_panel.keys, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(s_panel.vals, LV_OBJ_FLAG_HIDDEN);
		if (st == WIFI_STATE_PROVISIONING || st == WIFI_STATE_AP_ONLY) {
			/* hotspot info: SSID / key / AP IP (3 rows) */
			lv_label_set_text(s_panel.keys, UI_STR_HOTSPOT_KEYS);
			char buf[96];
			snprintf(buf, sizeof(buf), "%s\n" WEB_AP_PASS "\n" WEB_AP_IP, WEB_AP_SSID);
			lv_label_set_text(s_panel.vals, buf);
		} else if (st == WIFI_STATE_CONNECTED) {
			/* connected: LAN IP + mDNS name (4 rows) */
			lv_label_set_text(s_panel.keys, UI_STR_WIFI_KEYS);
			char buf[96];
			snprintf(buf, sizeof(buf), "%s\n-\n%s\nmacnano.local",
			         sta_ssid[0] ? sta_ssid : "-",
			         sta_ip[0] ? sta_ip : "-");
			lv_label_set_text(s_panel.vals, buf);
		} else {
			/* connecting / reconnecting: no IP/mDNS rows until connected */
			lv_label_set_text(s_panel.keys, UI_STR_WIFI_KEYS_SHORT);
			char buf[96];
			snprintf(buf, sizeof(buf), "%s\n-", sta_ssid[0] ? sta_ssid : "-");
			lv_label_set_text(s_panel.vals, buf);
		}
	}

	/* guide line */
	if (st == WIFI_STATE_OFF || st == WIFI_STATE_CONNECTED)
		lv_label_set_text(s_panel.hint, UI_STR_HINT_HOLD_PROVISION);
	else if (st == WIFI_STATE_PROVISIONING)
		lv_label_set_text(s_panel.hint, UI_STR_HINT_CONNECT_HOTSPOT);
	else if (st == WIFI_STATE_CONNECTING || st == WIFI_STATE_RECONNECTING)
		lv_label_set_text(s_panel.hint, UI_STR_HINT_HOLD_BUTTON);
	else
		lv_label_set_text(s_panel.hint, "");

	/* button: hidden while provisioning */
	if (st == WIFI_STATE_PROVISIONING) {
		lv_obj_add_flag(s_panel.btn, LV_OBJ_FLAG_HIDDEN);
	} else {
		lv_obj_clear_flag(s_panel.btn, LV_OBJ_FLAG_HIDDEN);
		lv_label_set_text(s_panel.btn_label,
		                  st == WIFI_STATE_OFF ? UI_STR_PROVISION : UI_STR_REPROVISION);
	}

	/* set text first, then align so the wrapped height is placed correctly */
	lv_obj_align(s_panel.hint, LV_ALIGN_BOTTOM_MID, 0, -10);
}
