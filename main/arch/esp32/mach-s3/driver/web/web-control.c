/*
 * Web control stage 1: Wi-Fi SoftAP + static HTTP page.
 * Minimal bring-up — no DNS/captive-portal/WebSocket yet.
 */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_http_server.h"
#include "nvs_flash.h"
#include "mdns.h"
#include "dns_server.h"
#define JSMN_HEADER /* declarations only; impl compiled once by jsmn_impl.c */
#include "jsmn.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lwip/priv/tcp_priv.h" /* tcp_active_pcbs / tcp_tw_pcbs for conn state */
#include "lwip/sockets.h"    /* close() in ws_close_cb */
#include "input.h"
#include "macplus.h"
#include "msg.h"
#include "flash_mode.h"
#include "machine_backend.h"
#include "settings_persist.h"
#include "web-control.h"

static const char *TAG = "web";

static const char *state_name(wifi_state_t st);
static void set_state(wifi_state_t st);
static void schedule_reconnect(void);
static void start_grace_timer(void);
static void start_provision_timer(void);

extern const uint8_t _binary_index_html_start[];
extern const uint8_t _binary_index_html_end[];

static httpd_handle_t s_server = NULL;
static dns_server_handle_t s_dns = NULL;
static esp_netif_t *s_ap_netif = NULL;
static esp_netif_t *s_sta_netif = NULL;
static bool s_web_enabled = false;
static bool s_ap_on = false;

/* WiFi state machine (P1 subset of docs/wifi-apsta-design.md). */
static wifi_state_t s_state = WIFI_STATE_OFF;
static char s_sta_ssid[33];
static char s_sta_pass[65];
static bool s_had_ip = false;
static esp_timer_handle_t s_grace_timer;
static bool s_events_registered = false;

/* WS state push (0x09): set_state() broadcasts the new wifi state to every
 * connected page; new connections get the current state on open. No HTTP
 * polling needed on the page side. */
#define WS_STATE_PUSH 0x09
#define WS_MAX_CLIENTS 8
static int s_ws_fds[WS_MAX_CLIENTS];
static int s_ws_fd_count;
static portMUX_TYPE s_ws_lock = portMUX_INITIALIZER_UNLOCKED;

/* Last provisioning fail reason, pushed in 0x09 frame byte 3:
 * 0=none 1=bad password 2=AP not found 3=other */
static uint8_t s_fail_reason = 0;

static void ws_register_fd(int fd)
{
	taskENTER_CRITICAL(&s_ws_lock);
	for (int i = 0; i < s_ws_fd_count; i++) {
		if (s_ws_fds[i] == fd) {
			taskEXIT_CRITICAL(&s_ws_lock);
			return;
		}
	}
	if (s_ws_fd_count < WS_MAX_CLIENTS)
		s_ws_fds[s_ws_fd_count++] = fd;
	taskEXIT_CRITICAL(&s_ws_lock);
}

static void ws_unregister_fd(int fd)
{
	taskENTER_CRITICAL(&s_ws_lock);
	for (int i = 0; i < s_ws_fd_count; i++) {
		if (s_ws_fds[i] == fd) {
			s_ws_fds[i] = s_ws_fds[s_ws_fd_count - 1];
			s_ws_fd_count--;
			break;
		}
	}
	taskEXIT_CRITICAL(&s_ws_lock);
}

static void ws_broadcast_state(void)
{
	if (s_server == NULL)
		return;
	int fds[WS_MAX_CLIENTS];
	int n;
	taskENTER_CRITICAL(&s_ws_lock);
	memcpy(fds, s_ws_fds, sizeof(fds));
	n = s_ws_fd_count;
	taskEXIT_CRITICAL(&s_ws_lock);
	uint8_t f[3] = { WS_STATE_PUSH, (uint8_t)s_state, s_fail_reason };
	httpd_ws_frame_t out = {
	        .type = HTTPD_WS_TYPE_BINARY, .payload = f, .len = sizeof(f),
	};
	for (int i = 0; i < n; i++) {
		const esp_err_t r = httpd_ws_send_frame_async(s_server, fds[i], &out);
		if (r != ESP_OK)
			ESP_LOGW(TAG, "ws push fd=%d failed: %s", fds[i], esp_err_to_name(r));
	}
}

/* Reconnect backoff (D3): 1s doubling up to 60s, self-managed. The driver's
 * default autoconnect stops on some disconnect reasons (e.g. 4WAY handshake
 * timeout), which leaves STA dead after a router reboot — so we own retries. */
#define WEB_RECONNECT_MAX_US (60u * 1000000u)
static esp_timer_handle_t s_reconnect_timer;
static int s_reconnect_retry = 0;

/* Provisioning (T14/T15): long-press F12 enters, 5min no-op timeout
 * closes the hotspot and returns to the pre-provision state. */
#define WEB_PROVISION_TIMEOUT_US (5u * 60u * 1000000u)
static esp_timer_handle_t s_provision_timer;

/* WiFi scan cache (P2c): SCAN_DONE fills it, GET /api/wifi/scan serves it. */
#define MAX_SCAN_RESULTS 16
typedef struct {
	char ssid[33];
	int8_t rssi;
	uint8_t authmode;
} wifi_scan_entry_t;
static wifi_scan_entry_t s_scan[MAX_SCAN_RESULTS];
static int s_scan_count;
static bool s_scan_ready;

/* mDNS: fixed hostname "macnano" -> http://macnano.local (D9). Init once
 * (esp_mdns is netif-scoped, not wifi-mode-scoped): survives enable/disable
 * cycles. Multi-device hostname collisions are a known limit of the fixed
 * name (P6 item, not addressed yet). */
static bool s_mdns_started = false;

static void start_mdns(void)
{
	if (s_mdns_started)
		return;
	esp_err_t err = mdns_init();
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "mdns init failed: %s", esp_err_to_name(err));
		return;
	}
	err = mdns_hostname_set("macnano");
	if (err != ESP_OK)
		ESP_LOGW(TAG, "mdns hostname set failed: %s", esp_err_to_name(err));
	err = mdns_service_add("MacNano Web", "_http", "_tcp", 80, NULL, 0);
	if (err != ESP_OK)
		ESP_LOGW(TAG, "mdns http service add failed: %s", esp_err_to_name(err));
	s_mdns_started = true;
	ESP_LOGI(TAG, "mdns ready: http://macnano.local");
}

/* Floppy presence is read live from the guest DISKINPLACE state (a
 * volatile byte — safe to read from the httpd task). No duplicated flag
 * to keep in sync. */

/* Screenshot: latest 1-bit Mac frame. DISP_WIDTH/HEIGHT come from
 * CMakeLists (DISP_WIDTH=640, DISP_HEIGHT=480). The current display
 * buffer is vbuf1/vbuf2 selected by via_port_a bit 6 — read straight
 * from the macplus instance (no duplicated pointer). */
#define MAC_SCREEN_BYTES (DISP_WIDTH * DISP_HEIGHT / 8)

/* Auto-off: WiFi stays on while there is activity (HTTP/WS); after
 * WEB_AUTO_OFF_US of idle the stack is disabled automatically. Kept for
 * AP_ONLY (T8 skip-provision); currently no state enters AP_ONLY, so the
 * timer is never armed — retained for future use. */
#define WEB_AUTO_OFF_US (60u * 1000000u)
static volatile int64_t s_last_activity_us;
static esp_timer_handle_t s_auto_off_timer;

static void web_control_touch(void)
{
	s_last_activity_us = esp_timer_get_time();
}

static void __attribute__((unused)) auto_off_cb(void *arg)
{
	(void)arg;
	/* auto-off applies to AP_ONLY (skipped provisioning); connected states
	 * must stay online for LAN remote control (D6). */
	if (s_state == WIFI_STATE_AP_ONLY && s_web_enabled &&
	    esp_timer_get_time() - s_last_activity_us > WEB_AUTO_OFF_US)
		web_control_disable();
}

/* ------------------------------------------------------------------ */
/* WebSocket: binary key/mouse frames                                 */
/*   [0]=0x01 keyboard: [1]=mod [2..7]=HID usage x6 (0=empty)         */
/*   [0]=0x02 mouse move: [1..2]=dx i16 BE [3..4]=dy i16 BE           */
/*   [0]=0x03 mouse button: [1]=button [2]=pressed(0/1)               */
/* ------------------------------------------------------------------ */
#define WS_FRAME_MAX 16

static void handle_ws_frame(httpd_req_t *req, const uint8_t *f, size_t len)
{
	web_control_touch();
	switch (f[0]) {
	case 0x01: {
		if (len < 8)
			return;
		uint8_t keys[6];
		for (int i = 0; i < 6; i++)
			keys[i] = f[2 + i];
		input_report_keyboard(f[1], keys);
		break;
	}
	case 0x02: {
		if (len < 5)
			return;
		int16_t dx = (int16_t)((f[1] << 8) | f[2]);
		int16_t dy = (int16_t)((f[3] << 8) | f[4]);
		input_post_mouse_move_rel(dx, dy);
		break;
	}
	case 0x03: {
		if (len < 3)
			return;
		if (f[2])
			input_post_mouse_down((input_mouse_button_t)f[1]);
		else
			input_post_mouse_up((input_mouse_button_t)f[1]);
		break;
	}
	case 0x04: {
		/* system key: keycode u16 BE + value (brightness/volume/etc.) */
		if (len < 4)
			return;
		input_keycode_t code = (input_keycode_t)((f[1] << 8) | f[2]);
		input_post_key(code, f[3]);
		break;
	}
	case 0x06: {
		/* status query (replaces HTTP /api/status polling):
		 * [1]=query_id, 0xFF=all. Reply 0x05 frames synchronously.
		 * 0x01=floppy, future: 0x02 volume, 0x03 backlight, 0x04 wifi... */
		if (len < 2)
			return;
		const uint8_t qid = f[1];
		macplus_t *s = macplus_instance();
		const bool inserted = (s != NULL) && mac_sony_disk_in_place(&s->sony, 1);
		if (qid == 0x01 || qid == 0xFF) {
			uint8_t resp[3] = { 0x05, 0x01, inserted ? 1u : 0u };
			httpd_ws_frame_t out = {
			        .type = HTTPD_WS_TYPE_BINARY,
			        .payload = resp,
			        .len = sizeof(resp),
			};
			httpd_ws_send_frame(req, &out);
		}
		break;
	}
	case 0x07: {
		/* enter Recover/Update mode: set one-shot flag + reboot.
		 * Never returns (esp_restart). */
		if (len < 1)
			return;
		ESP_LOGW(TAG, "ws command: enter flash mode from web");
		mach_s3_flash_mode_enter();
		break;
	}
	case 0x08: {
		/* reboot device. Never returns (esp_restart). */
		if (len < 1)
			return;
		ESP_LOGW(TAG, "ws command: reboot from web");
		machine_backend_reboot();
		break;
	}
	default:
		ESP_LOGI(TAG, "ws unknown: type=0x%02x len=%u", f[0], (unsigned)len);
		break;
	}
}

static esp_err_t handle_ws(httpd_req_t *req)
{
	web_control_touch();
	if (req->method == HTTP_GET) {
		/* register + push current state so the page routes immediately */
		ws_register_fd(httpd_req_to_sockfd(req));
		uint8_t f[3] = { WS_STATE_PUSH, (uint8_t)s_state, s_fail_reason };
		httpd_ws_frame_t out = {
		        .type = HTTPD_WS_TYPE_BINARY, .payload = f, .len = sizeof(f),
		};
		httpd_ws_send_frame(req, &out);
		ESP_LOGI(TAG, "ws connected");
		return ESP_OK;
	}
	httpd_ws_frame_t ws = { .type = HTTPD_WS_TYPE_BINARY };
	esp_err_t ret = httpd_ws_recv_frame(req, &ws, 0);
	if (ret != ESP_OK)
		return ret;
	if (ws.len == 0 || ws.len > WS_FRAME_MAX)
		return ESP_OK; /* close/ping frames: let httpd handle */
	uint8_t buf[WS_FRAME_MAX];
	ws.payload = buf;
	ret = httpd_ws_recv_frame(req, &ws, ws.len);
	if (ret != ESP_OK)
		return ret;
	/* CLOSE frame: explicit handling, return ESP_OK for httpd to finish up */
	if (ws.type == HTTPD_WS_TYPE_CLOSE) {
		ESP_LOGI(TAG, "ws close frame");
		return ESP_OK;
	}
	handle_ws_frame(req, buf, ws.len);
	return ESP_OK;
}

/* ------------------------------------------------------------------ */
/* HTTP handlers                                                      */
/* ------------------------------------------------------------------ */
static esp_err_t handle_root(httpd_req_t *req)
{
	web_control_touch();
	const size_t len = (size_t)(_binary_index_html_end - _binary_index_html_start);
	ESP_LOGI(TAG, "GET / -> %u bytes", (unsigned)len);
	httpd_resp_set_type(req, "text/html");
	return httpd_resp_send(req, (const char *)_binary_index_html_start, len);
}

static esp_err_t handle_status(httpd_req_t *req)
{
	web_control_touch();
	macplus_t *s = macplus_instance();
	const bool inserted = (s != NULL) && mac_sony_disk_in_place(&s->sony, 1);
	httpd_resp_set_type(req, "application/json");
	char buf[192];
	if (s_state == WIFI_STATE_CONNECTED) {
		/* STA info: SSID + LAN IP, for the success page IP fallback
		 * (only meaningful while connected; other states send null). */
		char ssid[33] = "", ip[16] = "";
		web_control_sta_info(ssid, sizeof(ssid), ip, sizeof(ip));
		char esc[sizeof(ssid) * 2 + 1];
		size_t j = 0;
		for (size_t i = 0; ssid[i] != '\0' && j < sizeof(esc) - 2; i++) {
			if (ssid[i] == '"' || ssid[i] == '\\') {
				esc[j++] = '\\';
			} else if ((unsigned char)ssid[i] < 0x20) {
				esc[j++] = '?'; /* strip control chars */
				continue;
			}
			esc[j++] = ssid[i];
		}
		esc[j] = '\0';
		snprintf(buf, sizeof(buf),
		         "{\"floppy\":%s,\"state\":\"%s\",\"sta\":{\"ssid\":\"%s\",\"ip\":\"%s\"}}",
		         inserted ? "true" : "false", state_name(s_state), esc, ip);
	} else {
		snprintf(buf, sizeof(buf), "{\"floppy\":%s,\"state\":\"%s\",\"sta\":null}",
		         inserted ? "true" : "false", state_name(s_state));
	}
	return httpd_resp_send(req, buf, HTTPD_RESP_USE_STRLEN);
}

/* Captive-portal probes: while provisioning/connecting, redirect to the
 * config page (design: 302 triggers iOS/Android portal popup). Remote
 * states keep 204/Success so the LAN control page is never bothered. */
static bool probe_redirect_active(void)
{
	return s_state == WIFI_STATE_PROVISIONING || s_state == WIFI_STATE_CONNECTING;
}

/* Android connectivity probe: 204 with empty body = "this network has internet". */
static esp_err_t handle_generate_204(httpd_req_t *req)
{
	web_control_touch();
	if (probe_redirect_active()) {
		httpd_resp_set_status(req, "302 Found");
		httpd_resp_set_hdr(req, "Location", "/");
		return httpd_resp_send(req, NULL, 0);
	}
	httpd_resp_set_status(req, "204 No Content");
	return httpd_resp_send(req, NULL, 0);
}

/* iOS connectivity probe: 200 with Success body. */
static esp_err_t handle_hotspot_detect(httpd_req_t *req)
{
	web_control_touch();
	if (probe_redirect_active()) {
		httpd_resp_set_status(req, "302 Found");
		httpd_resp_set_hdr(req, "Location", "/");
		return httpd_resp_send(req, NULL, 0);
	}
	httpd_resp_set_type(req, "text/html");
	return httpd_resp_send(req, "<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>", HTTPD_RESP_USE_STRLEN);
}

/* Static assets are inlined into index.html (web/dist) — no separate embedding */

/* Screenshot: latest 1-bit Mac frame (640x480 = 38400 bytes). */
static esp_err_t handle_screenshot(httpd_req_t *req)
{
	web_control_touch();
	macplus_t *s = macplus_instance();
	const uint8_t *f = (s != NULL) ? ((s->via_port_a & 0x40) ? s->vbuf1 : s->vbuf2) : NULL;
	if (f == NULL)
		return httpd_resp_send_404(req);
	/* Copy into a PSRAM buffer (internal RAM is scarce — wifi buffers need
	 * it); a frame switch mid-send cannot tear the payload. */
	uint8_t *shot = heap_caps_malloc(MAC_SCREEN_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
	if (shot == NULL) {
		httpd_resp_set_status(req, "500 Internal Server Error");
		return httpd_resp_send(req, "no memory", HTTPD_RESP_USE_STRLEN);
	}
	memcpy(shot, f, MAC_SCREEN_BYTES);
	httpd_resp_set_type(req, "application/octet-stream");
	esp_err_t err = httpd_resp_send(req, (const char *)shot, MAC_SCREEN_BYTES);
	heap_caps_free(shot);
	return err;
}

/* Upload a browser-built 400K MFS floppy image (raw binary body).
 * Write it to SD, then queue a writable insert on the emulator thread. */
static esp_err_t handle_floppy_upload(httpd_req_t *req)
{
	web_control_touch();
	const size_t max_image = 400u * 1024u + 1024u;
	size_t total = req->content_len;
	uint8_t *buf;
	FILE *fp;
	size_t got = 0;

	if (total == 0 || total > max_image) {
		httpd_resp_set_status(req, "400 Bad Request");
		return httpd_resp_send(req, "image too large", HTTPD_RESP_USE_STRLEN);
	}
	buf = heap_caps_malloc(total, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
	if (buf == NULL) {
		httpd_resp_set_status(req, "500 Internal Server Error");
		return httpd_resp_send(req, "no memory", HTTPD_RESP_USE_STRLEN);
	}
	while (got < total) {
		int r = httpd_req_recv(req, (char *)(buf + got), total - got);
		if (r <= 0) {
			heap_caps_free(buf);
			httpd_resp_set_status(req, "400 Bad Request");
			return httpd_resp_send(req, "recv failed", HTTPD_RESP_USE_STRLEN);
		}
		got += (size_t)r;
	}

	/* Write: try SD first, fall back to RAMFS (/ram when no SD), report error if both fail */
	const char *path = "/sdcard/upload.dsk";
	fp = fopen(path, "wb");
	if (fp == NULL) {
		path = "/ram/upload.dsk";
		fp = fopen(path, "wb");
	}
	if (fp == NULL) {
		heap_caps_free(buf);
		httpd_resp_set_status(req, "500 Internal Server Error");
		return httpd_resp_send(req, "sd write failed", HTTPD_RESP_USE_STRLEN);
	}
	size_t wr = fwrite(buf, 1, total, fp);
	fclose(fp);
	heap_caps_free(buf);
	if (wr != total) {
		httpd_resp_set_status(req, "500 Internal Server Error");
		return httpd_resp_send(req, "sd write failed", HTTPD_RESP_USE_STRLEN);
	}

	/* insert + persist use the actually written path (/sdcard or /ram) */
	mac_msg_submit("floppy.insert", path);
	(void)mach_s3_settings_persist_set_floppy_path(path);
	httpd_resp_set_type(req, "application/json");
	return httpd_resp_send(req, "{\"ok\":true}", HTTPD_RESP_USE_STRLEN);
}

/* ------------------------------------------------------------------ */
/* WiFi provisioning API (P2c)                                        */
/* ------------------------------------------------------------------ */

/* GET /api/wifi/scan: returns cached scan (from SCAN_DONE), or kicks a
 * scan and returns {"scanning":true}. */
static esp_err_t handle_wifi_scan(httpd_req_t *req)
{
	web_control_touch();
	if (!s_scan_ready) {
		wifi_scan_config_t sc = {
		        .ssid = NULL, .bssid = NULL, .channel = 0, .show_hidden = true,
		};
		const esp_err_t err = esp_wifi_scan_start(&sc, false);
		if (err != ESP_OK) {
			httpd_resp_set_type(req, "application/json");
			/* ESP_ERR_WIFI_STATE: scanning while connecting — retry shortly. */
			return httpd_resp_send(req, "{\"scanning\":true,\"err\":\"busy\"}", HTTPD_RESP_USE_STRLEN);
		}
		httpd_resp_set_type(req, "application/json");
		return httpd_resp_send(req, "{\"scanning\":true}", HTTPD_RESP_USE_STRLEN);
	}
	/* serve cached results, then mark stale so the next poll re-scans */
	s_scan_ready = false;
	char buf[32 + MAX_SCAN_RESULTS * 96];
	size_t off = 0;
	off += (size_t)snprintf(buf + off, sizeof(buf) - off, "[\n");
	for (int i = 0; i < s_scan_count && off < sizeof(buf) - 96; i++) {
		/* minimal JSON escaping for " and \ */
		char esc[33];
		size_t j = 0;
		for (size_t k = 0; s_scan[i].ssid[k] && j < sizeof(esc) - 2; k++) {
			if (s_scan[i].ssid[k] == '"' || s_scan[i].ssid[k] == '\\')
				esc[j++] = '\\';
			esc[j++] = s_scan[i].ssid[k];
		}
		esc[j] = '\0';
		off += (size_t)snprintf(buf + off, sizeof(buf) - off,
		                        "  {\"ssid\":\"%s\",\"rssi\":%d,\"auth\":%u}%s\n",
		                        esc, s_scan[i].rssi, s_scan[i].authmode,
		                        (i + 1 < s_scan_count) ? "," : "");
	}
	if (off + 2 < sizeof(buf))
		off += (size_t)snprintf(buf + off, sizeof(buf) - off, "]");
	httpd_resp_set_type(req, "application/json");
	return httpd_resp_send(req, buf, HTTPD_RESP_USE_STRLEN);
}

/* Extract ssid/pass from a small JSON object. */
static bool wifi_json_extract(const char *json, size_t json_len,
                              char *ssid, size_t ssid_len,
                              char *pass, size_t pass_len)
{
	jsmn_parser p;
	jsmn_init(&p);
	enum { MAX_TOK = 32 };
	jsmntok_t toks[MAX_TOK];
	const int r = jsmn_parse(&p, json, json_len, toks, MAX_TOK);
	if (r <= 0 || toks[0].type != JSMN_OBJECT)
		return false;
	ssid[0] = '\0';
	pass[0] = '\0';
	for (int i = 1; i < r - 1; i++) {
		if (toks[i].type != JSMN_STRING || toks[i + 1].type != JSMN_STRING)
			continue;
		const size_t klen = (size_t)(toks[i].end - toks[i].start);
		const size_t vlen = (size_t)(toks[i + 1].end - toks[i + 1].start);
		if (klen == 4 && strncmp(json + toks[i].start, "ssid", 4) == 0) {
			const size_t n = vlen < ssid_len - 1 ? vlen : ssid_len - 1;
			memcpy(ssid, json + toks[i + 1].start, n);
			ssid[n] = '\0';
		} else if (klen == 4 && strncmp(json + toks[i].start, "pass", 4) == 0) {
			const size_t n = vlen < pass_len - 1 ? vlen : pass_len - 1;
			memcpy(pass, json + toks[i + 1].start, n);
			pass[n] = '\0';
		}
		i++; /* skip the value token */
	}
	return ssid[0] != '\0';
}

/* POST /api/wifi/config: {"ssid":"..","pass":".."} — persist + connect (T5). */
static esp_err_t handle_wifi_config(httpd_req_t *req)
{
	web_control_touch();
	char body[320];
	const int total = req->content_len;
	if (total <= 0 || total >= (int)sizeof(body)) {
		httpd_resp_set_status(req, "400 Bad Request");
		return httpd_resp_send(req, "{\"error\":\"bad request\"}", HTTPD_RESP_USE_STRLEN);
	}
	int got = 0;
	while (got < total) {
		const int r = httpd_req_recv(req, body + got, (size_t)(total - got));
		if (r <= 0) {
			httpd_resp_set_status(req, "400 Bad Request");
			return httpd_resp_send(req, "{\"error\":\"recv\"}", HTTPD_RESP_USE_STRLEN);
		}
		got += r;
	}
	body[got] = '\0';

	char ssid[33], pass[65];
	if (!wifi_json_extract(body, (size_t)got, ssid, sizeof(ssid), pass, sizeof(pass))) {
		httpd_resp_set_status(req, "400 Bad Request");
		return httpd_resp_send(req, "{\"error\":\"json\"}", HTTPD_RESP_USE_STRLEN);
	}
	ESP_LOGI(TAG, "wifi config: ssid='%s' pass_len=%u", ssid, (unsigned)strlen(pass));

	/* Persist (source of truth for boot) and apply (T5). */
	(void)mach_s3_settings_persist_set_wifi_ssid(ssid);
	(void)mach_s3_settings_persist_set_wifi_pass(pass);
	strlcpy(s_sta_ssid, ssid, sizeof(s_sta_ssid));
	strlcpy(s_sta_pass, pass, sizeof(s_sta_pass));

	wifi_config_t sta_cfg = { 0 };
	strlcpy((char *)sta_cfg.sta.ssid, ssid, sizeof(sta_cfg.sta.ssid));
	strlcpy((char *)sta_cfg.sta.password, pass, sizeof(sta_cfg.sta.password));
	if (s_web_enabled && esp_wifi_set_config(WIFI_IF_STA, &sta_cfg) != ESP_OK) {
		httpd_resp_set_status(req, "500 Internal Server Error");
		return httpd_resp_send(req, "{\"error\":\"set_config\"}", HTTPD_RESP_USE_STRLEN);
	}
	set_state(WIFI_STATE_CONNECTING);
	if (s_web_enabled)
		esp_wifi_connect();

	httpd_resp_set_type(req, "application/json");
	return httpd_resp_send(req, "{\"ok\":true}", HTTPD_RESP_USE_STRLEN);
}

/* POST /api/wifi/done: success page "copy & close hotspot" button — end the
 * grace period immediately (AP off, DNS off, stay CONNECTED). */
static void end_grace(void);
static esp_err_t handle_wifi_done(httpd_req_t *req)
{
	web_control_touch();
	if (s_state != WIFI_STATE_CONNECTED) {
		httpd_resp_set_status(req, "409 Conflict");
		return httpd_resp_send(req, "{\"error\":\"not_in_grace\"}", HTTPD_RESP_USE_STRLEN);
	}
	if (s_grace_timer != NULL)
		esp_timer_stop(s_grace_timer);
	end_grace();
	httpd_resp_set_type(req, "application/json");
	return httpd_resp_send(req, "{\"ok\":true}", HTTPD_RESP_USE_STRLEN);
}

/* httpd connection events: per-connection log so accept(23) failures can be
 * correlated with session count in the log stream. */
/* Connection state snapshot, printed on httpd events (no timer):
 * sessions/ws/heap/state/tcp PCB — correlates accept(23) with session count. */
static void log_conn_state(const char *evt, int fd)
{
	if (s_server == NULL)
		return;
	size_t n = 8;
	int fds[8];
	const int sessions = (httpd_get_client_list(s_server, &n, fds) == ESP_OK) ? (int)n : -1;
	int ws_n;
	taskENTER_CRITICAL(&s_ws_lock);
	ws_n = s_ws_fd_count;
	taskEXIT_CRITICAL(&s_ws_lock);
	int tcp_active = 0, tcp_tw = 0;
	for (struct tcp_pcb *p = tcp_active_pcbs; p != NULL; p = p->next)
		tcp_active++;
	for (struct tcp_pcb *p = tcp_tw_pcbs; p != NULL; p = p->next)
		tcp_tw++;
	ESP_LOGI(TAG, "http %s fd=%d sessions=%d ws=%d heap=%u state=%s tcp(active=%d tw=%d/%d)",
	         evt, fd, sessions, ws_n,
	         (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
	         state_name(s_state),
	         tcp_active, tcp_tw, CONFIG_LWIP_MAX_ACTIVE_TCP);
}

static void http_evt_cb(void *arg, esp_event_base_t base, int32_t id, void *data)
{
	(void)arg;
	(void)base;
	switch (id) {
	case HTTP_SERVER_EVENT_ON_CONNECTED: {
		const int fd = data ? *(const int *)data : -1;
		log_conn_state("+", fd);
		break;
	}
	case HTTP_SERVER_EVENT_DISCONNECTED: {
		const int fd = data ? *(const int *)data : -1;
		log_conn_state("-", fd);
		break;
	}
	case HTTP_SERVER_EVENT_ERROR:
		log_conn_state("err", -1);
		break;
	default:
		break;
	}
}

static void ws_close_cb(httpd_handle_t h, int fd)
{
	(void)h;
	ws_unregister_fd(fd);
	/* close_fn owns the fd: httpd won't close it. Missing close() leaves the
	 * PCB stuck in CLOSE_WAIT until the pool exhausts (accept 113). */
	close(fd);
}

static void start_http_server(void)
{
	httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
	cfg.server_port = 80;
	/* Defaults + max_open_sockets only (matches xiaozhi production use). */
	cfg.max_open_sockets = 7; /* default, explicit */
	/* Drop stale WS fds on disconnect. */
	cfg.close_fn = ws_close_cb;
	cfg.max_uri_handlers = 16; /* root/status/ws/204/hotspot/floppy */
	if (httpd_start(&s_server, &cfg) != ESP_OK) {
		ESP_LOGE(TAG, "httpd start failed");
		return;
	}
	static const httpd_uri_t root = {
	        .uri = "/", .method = HTTP_GET, .handler = handle_root,
	};
	static const httpd_uri_t status = {
	        .uri = "/api/status", .method = HTTP_GET, .handler = handle_status,
	};
	static const httpd_uri_t ws = {
	        .uri = "/ws", .method = HTTP_GET, .handler = handle_ws, .is_websocket = true,
	};
	static const httpd_uri_t uri_204 = {
	        .uri = "/generate_204", .method = HTTP_GET, .handler = handle_generate_204,
	};
	static const httpd_uri_t uri_hotspot = {
	        .uri = "/hotspot-detect.html", .method = HTTP_GET, .handler = handle_hotspot_detect,
	};
	static const httpd_uri_t uri_floppy = {
	        .uri = "/api/floppy", .method = HTTP_POST, .handler = handle_floppy_upload,
	};
	httpd_register_uri_handler(s_server, &root);
	httpd_register_uri_handler(s_server, &status);
	httpd_register_uri_handler(s_server, &ws);
	httpd_register_uri_handler(s_server, &uri_204);
	httpd_register_uri_handler(s_server, &uri_hotspot);
	static const httpd_uri_t uri_shot = {
	        .uri = "/api/screenshot", .method = HTTP_GET, .handler = handle_screenshot,
	};
	httpd_register_uri_handler(s_server, &uri_shot);
	httpd_register_uri_handler(s_server, &uri_floppy);
	static const httpd_uri_t uri_wifi_scan = {
	        .uri = "/api/wifi/scan", .method = HTTP_GET, .handler = handle_wifi_scan,
	};
	httpd_register_uri_handler(s_server, &uri_wifi_scan);
	static const httpd_uri_t uri_wifi_config = {
	        .uri = "/api/wifi/config", .method = HTTP_POST, .handler = handle_wifi_config,
	};
	httpd_register_uri_handler(s_server, &uri_wifi_config);
	static const httpd_uri_t uri_wifi_done = {
	        .uri = "/api/wifi/done", .method = HTTP_POST, .handler = handle_wifi_done,
	};
	httpd_register_uri_handler(s_server, &uri_wifi_done);

	ESP_LOGI(TAG, "http server ready: http://192.168.4.1");
}

/* ------------------------------------------------------------------ */
/* NVS (required by esp_wifi_init; project inits it lazily elsewhere)  */
/* ------------------------------------------------------------------ */
static void ensure_nvs_init(void)
{
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_LOGW(TAG, "nvs init requires erase: %s", esp_err_to_name(err));
		(void)nvs_flash_erase();
		err = nvs_flash_init();
	}
	if (err != ESP_OK)
		ESP_LOGE(TAG, "nvs init failed: %s", esp_err_to_name(err));
	else
		ESP_LOGI(TAG, "nvs init ok");
}

/* ------------------------------------------------------------------ */
/* WiFi init + state machine (P1)                                      */
/* ------------------------------------------------------------------ */

static const char *state_name(wifi_state_t st)
{
	switch (st) {
	case WIFI_STATE_OFF: return "OFF";
	case WIFI_STATE_PROVISIONING: return "PROVISIONING";
	case WIFI_STATE_CONNECTING: return "CONNECTING";
	case WIFI_STATE_CONNECTED: return "CONNECTED";
	case WIFI_STATE_RECONNECTING: return "RECONNECTING";
	case WIFI_STATE_AP_ONLY: return "AP_ONLY";
	}
	return "?";
}

static void set_state(wifi_state_t st)
{
	if (s_state == st)
		return;
	ESP_LOGI(TAG, "state %s -> %s (heap=%u)", state_name(s_state), state_name(st),
	         (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
	s_state = st;
	ws_broadcast_state();
}

/* All wifi mode switches go through here (design: ap_set_mode). Logs the
 * AP_START/AP_STOP/STA_STOP event sequence so runtime set_mode behavior
 * can be verified on hardware (esp_wifi.h is silent on whether started
 * interfaces are restarted by set_mode). */
static esp_err_t ap_set_mode(wifi_mode_t mode)
{
	esp_err_t err = esp_wifi_set_mode(mode);
	if (err != ESP_OK)
		ESP_LOGE(TAG, "set_mode(%d) failed: %s", (int)mode, esp_err_to_name(err));
	return err;
}

/* 20s grace after first GOT_IP: AP stays up so a phone on the hotspot can
 * see the success page, then drop to STA-only (T4). The success page shows
 * a 20s countdown and a "copy & close hotspot" button (POST /api/wifi/done). */
#define WEB_GRACE_US (20u * 1000000u)

/* Atomically take ownership of the AP and DNS server: both are cleared so
 * concurrent callers (grace timer, POST /api/wifi/done, disconnect event)
 * are idempotent — the second caller gets NULL and does nothing. The caller
 * must run ap_set_mode/stop_dns_server *outside* the critical section:
 * stop_dns_server blocks waiting for its task (vTaskDelay), which must not
 * run with interrupts disabled. */
static dns_server_handle_t take_ap_dns(bool *ap_was_on)
{
	dns_server_handle_t dns;
	taskENTER_CRITICAL(&s_ws_lock);
	*ap_was_on = s_ap_on;
	s_ap_on = false;
	dns = s_dns;
	s_dns = NULL;
	taskEXIT_CRITICAL(&s_ws_lock);
	return dns;
}

/* End the grace period: AP off, DNS off, stay CONNECTED (STA-only). */
static void end_grace(void)
{
	if (s_state != WIFI_STATE_CONNECTED)
		return;
	bool ap_was_on = false;
	dns_server_handle_t dns = take_ap_dns(&ap_was_on);
	ESP_LOGI(TAG, "grace over -> STA-only");
	if (ap_was_on)
		ap_set_mode(WIFI_MODE_STA);
	if (dns != NULL)
		stop_dns_server(dns);
}

static void grace_timeout_cb(void *arg)
{
	(void)arg;
	end_grace();
}

static void schedule_reconnect(void);
static void reconnect_tick(void *arg);

static void enter_reconnecting(void)
{
	if (s_state == WIFI_STATE_RECONNECTING)
		return;
	set_state(WIFI_STATE_RECONNECTING);
	if (s_grace_timer != NULL)
		esp_timer_stop(s_grace_timer);
	bool ap_was_on = false;
	dns_server_handle_t dns = take_ap_dns(&ap_was_on);
	if (ap_was_on)
		ap_set_mode(WIFI_MODE_STA);
	if (dns != NULL)
		stop_dns_server(dns);
	ESP_LOGI(TAG, "reconnecting silently (AP off)");
	schedule_reconnect();
}

static void schedule_reconnect(void)
{
	uint64_t delay = (uint64_t)1000000u << (s_reconnect_retry > 6 ? 6 : s_reconnect_retry);
	if (delay > WEB_RECONNECT_MAX_US)
		delay = WEB_RECONNECT_MAX_US;
	if (s_reconnect_retry < 30)
		s_reconnect_retry++;
	if (s_reconnect_timer == NULL) {
		esp_timer_create_args_t args = {
		        .callback = reconnect_tick, .name = "web-reconnect",
		};
		esp_timer_create(&args, &s_reconnect_timer);
	}
	esp_timer_stop(s_reconnect_timer);
	esp_timer_start_once(s_reconnect_timer, delay);
	ESP_LOGI(TAG, "reconnect attempt in %llu s (retry=%d)", (unsigned long long)(delay / 1000000), s_reconnect_retry);
}

static void reconnect_tick(void *arg)
{
	(void)arg;
	esp_err_t err = esp_wifi_connect();
	if (err == ESP_OK) {
		ESP_LOGI(TAG, "reconnect connect issued (retry=%d)", s_reconnect_retry);
	} else if (err == ESP_ERR_WIFI_CONN) {
		/* already connecting — wait for the disconnect event to re-arm */
	} else {
		ESP_LOGW(TAG, "reconnect connect rc=%s, backing off", esp_err_to_name(err));
		schedule_reconnect();
	}
}

static void start_grace_timer(void)
{
	if (s_grace_timer == NULL) {
		esp_timer_create_args_t args = {
		        .callback = grace_timeout_cb, .name = "web-grace",
		};
		esp_timer_create(&args, &s_grace_timer);
	}
	esp_timer_start_once(s_grace_timer, WEB_GRACE_US);
}

static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
	(void)arg;
	(void)base;
	switch (id) {
	case WIFI_EVENT_STA_START:
		ESP_LOGI(TAG, "sta start");
		/* connect is driven here so enable() never races the wifi task */
		if (s_state == WIFI_STATE_CONNECTING || s_state == WIFI_STATE_RECONNECTING)
			esp_wifi_connect();
		break;
	case WIFI_EVENT_STA_CONNECTED: {
		wifi_event_sta_connected_t *e = data;
		ESP_LOGI(TAG, "sta connected to '%s' ch=%d", e->ssid, e->channel);
		/* stay in CONNECTING until GOT_IP */
		break;
	}
	case WIFI_EVENT_STA_DISCONNECTED: {
		wifi_event_sta_disconnected_t *e = data;
		ESP_LOGI(TAG, "sta disconnected reason=%d (%s)", e->reason,
		         e->reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT ? "handshake timeout" :
		         e->reason == WIFI_REASON_AUTH_FAIL ? "auth fail" :
		         e->reason == WIFI_REASON_NO_AP_FOUND ? "no ap found" : "");
		if (e->reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT || e->reason == WIFI_REASON_AUTH_FAIL)
			ESP_LOGW(TAG, "likely wrong password");
		/* T10 (dropped after connected) / T19 (boot connect failed): silent
		 * reconnect with backoff; RECONNECTING failures double the backoff.
		 * T6/T7: a *provisioning* connect failure (wrong password / no AP)
		 * goes back to the form instead — never a silent retry loop. */
		if (s_state == WIFI_STATE_CONNECTING || s_state == WIFI_STATE_CONNECTED) {
			bool prov_pending = false;
			mach_s3_settings_persist_get_provisioning(&prov_pending);
			if (prov_pending && s_state == WIFI_STATE_CONNECTING) {
				/* fail reason detail (0x09 byte 3) */
				if (e->reason == WIFI_REASON_AUTH_FAIL || e->reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT)
					s_fail_reason = 1; /* bad password */
				else if (e->reason == WIFI_REASON_NO_AP_FOUND)
					s_fail_reason = 2; /* AP not found */
				else
					s_fail_reason = 3; /* other */
				/* back to form: stop retry, re-enter PROVISIONING */
				if (s_reconnect_timer != NULL)
					esp_timer_stop(s_reconnect_timer);
				s_reconnect_retry = 0;
				set_state(WIFI_STATE_PROVISIONING);
				start_provision_timer();
				ESP_LOGW(TAG, "provisioning connect failed reason=%d -> back to form", e->reason);
			} else {
				enter_reconnecting();
			}
		} else if (s_state == WIFI_STATE_RECONNECTING) {
			schedule_reconnect();
		}
		break;
	}
	case WIFI_EVENT_AP_START:
		ESP_LOGI(TAG, "ap start");
		break;
	case WIFI_EVENT_AP_STOP:
		ESP_LOGI(TAG, "ap stop");
		break;
	case WIFI_EVENT_STA_STOP:
		ESP_LOGI(TAG, "sta stop");
		break;
	case WIFI_EVENT_SCAN_DONE: {
		uint16_t num = 0;
		esp_wifi_scan_get_ap_num(&num);
		if (num > MAX_SCAN_RESULTS)
			num = MAX_SCAN_RESULTS;
		if (num > 0) {
			wifi_ap_record_t *recs = heap_caps_malloc((size_t)num * sizeof(*recs), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
			if (recs != NULL) {
				if (esp_wifi_scan_get_ap_records(&num, recs) == ESP_OK) {
					for (int i = 0; i < num; i++) {
						strlcpy(s_scan[i].ssid, (const char *)recs[i].ssid, sizeof(s_scan[i].ssid));
						s_scan[i].rssi = recs[i].rssi;
						s_scan[i].authmode = (uint8_t)recs[i].authmode;
					}
					s_scan_count = num;
					s_scan_ready = true;
					ESP_LOGI(TAG, "scan done: %d APs", num);
				}
				heap_caps_free(recs);
			}
		} else {
			s_scan_count = 0;
			s_scan_ready = true;
		}
		break;
	}
	default:
		break;
	}
}

static void ip_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
	(void)arg;
	(void)base;
	if (id != IP_EVENT_STA_GOT_IP)
		return;
	ip_event_got_ip_t *e = data;
	ESP_LOGI(TAG, "got ip " IPSTR " (state=%s)", IP2STR(&e->ip_info.ip), state_name(s_state));
	s_had_ip = true;
	s_fail_reason = 0; /* connected: clear fail reason */
	if (s_state == WIFI_STATE_CONNECTING) {
		set_state(WIFI_STATE_CONNECTED); /* T3: grace window, AP stays up */
		start_grace_timer();
		/* provisioning finished: clear the persistent flag */
		mach_s3_settings_persist_set_provisioning(false);
	} else if (s_state == WIFI_STATE_RECONNECTING) {
		if (s_reconnect_timer != NULL)
			esp_timer_stop(s_reconnect_timer);
		s_reconnect_retry = 0;
		set_state(WIFI_STATE_CONNECTED); /* T11: AP already off */
	}
}

static void register_event_handlers(void)
{
	if (s_events_registered)
		return;
	esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
	esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL, NULL);
	esp_event_handler_instance_register(ESP_HTTP_SERVER_EVENT, ESP_EVENT_ANY_ID, &http_evt_cb, NULL, NULL);
	s_events_registered = true;
}

/* One-time wifi infrastructure (netif/event loop/wifi init); idempotent. */
static esp_err_t init_wifi(void)
{
	ensure_nvs_init();

	esp_err_t err = esp_netif_init();
	if (err != ESP_OK)
		ESP_LOGE(TAG, "esp_netif_init: %s", esp_err_to_name(err));

	/* Event loop may already exist; invalid state is fine. */
	err = esp_event_loop_create_default();
	if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
		ESP_LOGE(TAG, "esp_event_loop_create_default: %s", esp_err_to_name(err));

	/* Netifs are created once and reused across enable/disable cycles. */
	if (s_ap_netif == NULL) {
		s_ap_netif = esp_netif_create_default_wifi_ap();
		if (s_ap_netif == NULL)
			return ESP_ERR_NO_MEM;
	}
	if (s_sta_netif == NULL) {
		s_sta_netif = esp_netif_create_default_wifi_sta();
		if (s_sta_netif == NULL)
			return ESP_ERR_NO_MEM;
		esp_netif_set_hostname(s_sta_netif, "macnano");
	}

	register_event_handlers();

	wifi_init_config_t wic = WIFI_INIT_CONFIG_DEFAULT();
	err = esp_wifi_init(&wic);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "esp_wifi_init: %s", esp_err_to_name(err));
		return err;
	}
	/* Reconnect is fully self-managed (backoff above): in IDF v5.5
	 * esp_wifi_connect() attempts once, the driver never retries on its
	 * own (esp_wifi_set_auto_connect no longer exists) — the disconnect
	 * event + backoff timer is the only reconnect path. */
	return ESP_OK;
}

/* Start the web control stack. Idempotent.
 * P1: with credentials -> APSTA + STA connect (CONNECTING); without ->
 * wifi stays off (AP_ONLY deferred; state/code kept for T8 skip-provision).
 * Provisioning entry (long-press F12) is P2. */
esp_err_t web_control_enable(void)
{
	if (s_web_enabled)
		return ESP_OK;

	web_control_touch();

	/* Reboot during provisioning: stay in provisioning mode (hotspot up,
	 * STA never connects) instead of auto-reconnecting. */
	bool prov_pending = false;
	mach_s3_settings_persist_get_provisioning(&prov_pending);
	if (prov_pending) {
		ESP_LOGI(TAG, "provisioning flag set on boot -> provisioning mode");
		return web_control_reprovision();
	}

	/* Credentials: NVS is the source of truth. No fallback — without creds
	 * the stack stays off (no hotspot; long-press F12 provisions). */
	bool has_cred = mach_s3_settings_persist_get_wifi_ssid(s_sta_ssid, sizeof(s_sta_ssid)) &&
	                mach_s3_settings_persist_get_wifi_pass(s_sta_pass, sizeof(s_sta_pass));

	esp_err_t err = init_wifi();
	if (err != ESP_OK)
		return err;

	wifi_config_t ap_cfg = { .ap = {
	        .ssid_len = 0,
	        .max_connection = 4,
	        .authmode = WIFI_AUTH_WPA2_PSK,
	        /* channel 0: APSTA follows STA channel; AP-only picks default */
	        .channel = 0,
	} };
	strcpy((char *)ap_cfg.ap.ssid, WEB_AP_SSID);
	strcpy((char *)ap_cfg.ap.password, WEB_AP_PASS);

	if (has_cred) {
		/* T2: APSTA (hotspot stays up through the grace window) */
		err = ap_set_mode(WIFI_MODE_APSTA);
		if (err != ESP_OK)
			goto fail;
		err = esp_wifi_set_config(WIFI_IF_AP, &ap_cfg);
		if (err != ESP_OK)
			goto fail;
		wifi_config_t sta_cfg = { 0 };
		strlcpy((char *)sta_cfg.sta.ssid, s_sta_ssid, sizeof(sta_cfg.sta.ssid));
		strlcpy((char *)sta_cfg.sta.password, s_sta_pass, sizeof(sta_cfg.sta.password));
		err = esp_wifi_set_config(WIFI_IF_STA, &sta_cfg);
		if (err != ESP_OK)
			goto fail;
		err = esp_wifi_start();
		if (err != ESP_OK)
			goto fail;
		s_ap_on = true;
		set_state(WIFI_STATE_CONNECTING); /* STA_START handler calls connect */
		ESP_LOGI(TAG, "wifi up (APSTA), sta target '%s'", s_sta_ssid);
	} else {
		/* No credentials: wifi stays off (AP_ONLY deferred as the default;
		 * state/code kept for T8 "skip provisioning"). Panel shows Off;
		 * provisioning is entered via long-press F12 only (T1/T14). */
		ESP_LOGI(TAG, "no credentials: wifi stays off; long-press F12 to provision");
		return ESP_OK;
	}

	s_web_enabled = true;

	dns_server_config_t dns_cfg = DNS_SERVER_CONFIG_SINGLE("*", "WIFI_AP_DEF");
	s_dns = start_dns_server(&dns_cfg);
	if (s_dns == NULL)
		ESP_LOGE(TAG, "dns server start failed");
	else
		ESP_LOGI(TAG, "dns server: all queries -> softAP IP");

	start_http_server();
	if (s_server == NULL) {
		ESP_LOGE(TAG, "httpd start failed; stopping");
		web_control_disable();
		return ESP_ERR_NO_MEM;
	}

	/* mDNS hostname/service must exist before the first page load (P0). */
	start_mdns();

	ESP_LOGI(TAG, "web control enabled (state=%s)", state_name(s_state));
	return ESP_OK;

fail:
	/* best-effort cleanup of whatever partially started */
	esp_wifi_stop();
	esp_wifi_deinit();
	ESP_LOGE(TAG, "wifi start failed: %s", esp_err_to_name(err));
	return err;
}

/* Stop the web control stack and release its RAM. Idempotent. */
void web_control_disable(void)
{
	if (!s_web_enabled)
		return;

	if (s_grace_timer != NULL)
		esp_timer_stop(s_grace_timer);
	if (s_reconnect_timer != NULL)
		esp_timer_stop(s_reconnect_timer);
	s_reconnect_retry = 0;
	if (s_provision_timer != NULL)
		esp_timer_stop(s_provision_timer);

	if (s_server != NULL) {
		httpd_stop(s_server);
		s_server = NULL;
	}
	if (s_dns != NULL) {
		stop_dns_server(s_dns);
		s_dns = NULL;
	}
	esp_wifi_stop();
	esp_wifi_deinit();

	if (s_auto_off_timer != NULL) {
		esp_timer_stop(s_auto_off_timer);
	}

	s_web_enabled = false;
	s_ap_on = false;
	s_had_ip = false;
	set_state(WIFI_STATE_OFF);
	ESP_LOGI(TAG, "web control disabled");
}

bool web_control_is_enabled(void)
{
	return s_web_enabled;
}

wifi_state_t web_control_state(void)
{
	return s_state;
}

void web_control_sta_info(char *ssid, size_t ssid_len, char *ip, size_t ip_len)
{
	if (ssid != NULL && ssid_len > 0)
		strlcpy(ssid, s_sta_ssid, ssid_len);
	if (ip != NULL && ip_len > 0) {
		ip[0] = '\0';
		if (s_sta_netif != NULL) {
			esp_netif_ip_info_t info;
			if (esp_netif_get_ip_info(s_sta_netif, &info) == ESP_OK &&
			    info.ip.addr != 0)
				snprintf(ip, ip_len, IPSTR, IP2STR(&info.ip));
		}
	}
}

/* ------------------------------------------------------------------ */
/* Provisioning (T14/T15)                                             */
/* ------------------------------------------------------------------ */

static void provision_timeout_cb(void *arg)
{
	(void)arg;
	if (s_state != WIFI_STATE_PROVISIONING)
		return;
	/* T15: 5min no-op — close everything. Never auto-connect afterwards
	 * (pure provisioning mode); re-provisioning = long-press F12 again. */
	ESP_LOGI(TAG, "provisioning timeout -> OFF");
	web_control_disable();
}

static void start_provision_timer(void)
{
	if (s_provision_timer == NULL) {
		esp_timer_create_args_t args = {
		        .callback = provision_timeout_cb, .name = "web-provision",
		};
		esp_timer_create(&args, &s_provision_timer);
	}
	esp_timer_start_once(s_provision_timer, WEB_PROVISION_TIMEOUT_US);
	ESP_LOGI(TAG, "provisioning: 5min timeout armed");
}

/* T14: long-press F12 (any state) -> PROVISIONING. Pure provisioning mode:
 * hotspot + DNS up, and STA is NOT connected — even with creds in NVS we
 * never auto-connect while provisioning (user requirement). On timeout,
 * saved creds resume (T15). */
esp_err_t web_control_reprovision(void)
{
	web_control_touch();

	if (!s_web_enabled) {
		/* OFF: pure bring-up — APSTA so the STA interface exists for
		 * scanning, but no STA creds set and no connect. */
		esp_err_t err = init_wifi();
		if (err != ESP_OK)
			return err;
		wifi_config_t ap_cfg = { .ap = {
		        .ssid_len = 0,
		        .max_connection = 4,
		        .authmode = WIFI_AUTH_WPA2_PSK,
		        .channel = 0,
		} };
		strcpy((char *)ap_cfg.ap.ssid, WEB_AP_SSID);
		strcpy((char *)ap_cfg.ap.password, WEB_AP_PASS);
		err = ap_set_mode(WIFI_MODE_APSTA);
		if (err != ESP_OK)
			goto fail;
		err = esp_wifi_set_config(WIFI_IF_AP, &ap_cfg);
		if (err != ESP_OK)
			goto fail;
		err = esp_wifi_start();
		if (err != ESP_OK)
			goto fail;
		s_ap_on = true;
		s_web_enabled = true;

		dns_server_config_t dns_cfg = DNS_SERVER_CONFIG_SINGLE("*", "WIFI_AP_DEF");
		s_dns = start_dns_server(&dns_cfg);
		if (s_dns == NULL)
			ESP_LOGE(TAG, "dns server start failed");

		start_http_server();
		if (s_server == NULL) {
			ESP_LOGE(TAG, "httpd start failed; stopping");
			web_control_disable();
			return ESP_ERR_NO_MEM;
		}
		start_mdns();

		mach_s3_settings_persist_set_provisioning(true);
		set_state(WIFI_STATE_PROVISIONING);
		start_provision_timer();
		ESP_LOGI(TAG, "provisioning up (APSTA, STA idle)");
		return ESP_OK;

fail:
		esp_wifi_stop();
		esp_wifi_deinit();
		ESP_LOGE(TAG, "provisioning bring-up failed: %s", esp_err_to_name(err));
		return err;
	}

	/* Already up: hotspot + DNS on; STA fully disconnected — pure
	 * provisioning, never keep/establish a STA link even with creds. */
	if (!s_ap_on) {
		esp_err_t err = ap_set_mode(WIFI_MODE_APSTA);
		if (err != ESP_OK)
			return err;
		s_ap_on = true;
	}
	if (s_dns == NULL) {
		dns_server_config_t dns_cfg = DNS_SERVER_CONFIG_SINGLE("*", "WIFI_AP_DEF");
		s_dns = start_dns_server(&dns_cfg);
	}
	/* stop silent reconnect, then drop any STA link. State first so the
	 * disconnect event cannot divert us into RECONNECTING. */
	if (s_reconnect_timer != NULL)
		esp_timer_stop(s_reconnect_timer);
	s_reconnect_retry = 0;
	/* persist: a reboot during provisioning must stay in provisioning mode */
	mach_s3_settings_persist_set_provisioning(true);
	set_state(WIFI_STATE_PROVISIONING);
	esp_wifi_disconnect();
	start_provision_timer();
	return ESP_OK;
}
