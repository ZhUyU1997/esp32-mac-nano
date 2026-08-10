#ifndef WEB_CONTROL_H
#define WEB_CONTROL_H

#include "esp_err.h"

/* WiFi state machine (design doc: docs/wifi-apsta-design.md). */
typedef enum {
	WIFI_STATE_OFF = 0,         /* disabled (web_control_disable) */
	WIFI_STATE_PROVISIONING,    /* provisioning: APSTA (P2) */
	WIFI_STATE_CONNECTING,      /* connecting after boot/creds: APSTA */
	WIFI_STATE_CONNECTED,       /* connected: STA-only after 60s grace */
	WIFI_STATE_RECONNECTING,    /* silent reconnect: STA-only, no AP */
	WIFI_STATE_AP_ONLY,         /* no creds: AP-only + 60s auto-off */
} wifi_state_t;

/* Start/stop the web control stack (wifi + http server). Idempotent. */
esp_err_t web_control_enable(void);
void web_control_disable(void);
bool web_control_is_enabled(void);
wifi_state_t web_control_state(void);

/* Long-press F12 / panel button entry (T14): any state -> PROVISIONING —
 * hotspot + DNS up, 5min no-op timeout (T15). STA stays disconnected
 * (pure provisioning mode). */
esp_err_t web_control_reprovision(void);

/* Hotspot identity shown on the panel while provisioning (P4). */
#define WEB_AP_SSID "MacNano-ESP32"
#define WEB_AP_PASS "mac-nano"
#define WEB_AP_IP   "192.168.4.1"

/* STA info for the LVGL panel (P4): configured home-network SSID + current
 * STA IPv4 (empty string when not connected). */
void web_control_sta_info(char *ssid, size_t ssid_len, char *ip, size_t ip_len);

#endif
