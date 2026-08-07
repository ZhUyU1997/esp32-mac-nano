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
#include "dns_server.h"
#include "input.h"
#include "macplus.h"
#include "msg.h"
#include "flash_mode.h"
#include "machine_backend.h"
#include "settings_persist.h"
#include "web-control.h"

static const char *TAG = "web";

#define WEB_AP_SSID     "MacNano-ESP32"
#define WEB_AP_PASS     "mac-nano"

extern const uint8_t _binary_index_html_start[];
extern const uint8_t _binary_index_html_end[];

static httpd_handle_t s_server = NULL;
static dns_server_handle_t s_dns = NULL;
static esp_netif_t *s_ap_netif = NULL;
static bool s_ap_started = false;

/* Floppy presence is read live from the guest DISKINPLACE state (a
 * volatile byte — safe to read from the httpd task). No duplicated flag
 * to keep in sync. */

/* Screenshot: latest 1-bit Mac frame. DISP_WIDTH/HEIGHT come from
 * CMakeLists (DISP_WIDTH=640, DISP_HEIGHT=480). The current display
 * buffer is vbuf1/vbuf2 selected by via_port_a bit 6 — read straight
 * from the macplus instance (no duplicated pointer). */
#define MAC_SCREEN_BYTES (DISP_WIDTH * DISP_HEIGHT / 8)

/* Auto-off: WiFi stays on while there is activity (HTTP/WS); after
 * WEB_AUTO_OFF_US of idle the stack is disabled automatically. */
#define WEB_AUTO_OFF_US (60u * 1000000u)
static volatile int64_t s_last_activity_us;
static esp_timer_handle_t s_auto_off_timer;

static void web_control_touch(void)
{
	s_last_activity_us = esp_timer_get_time();
}

static void auto_off_cb(void *arg)
{
	(void)arg;
	if (s_ap_started && esp_timer_get_time() - s_last_activity_us > WEB_AUTO_OFF_US)
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
		ESP_LOGI(TAG, "ws connected");
		return ESP_OK;
	}
	httpd_ws_frame_t ws = { .type = HTTPD_WS_TYPE_BINARY };
	esp_err_t ret = httpd_ws_recv_frame(req, &ws, 0);
	if (ret != ESP_OK)
		return ret;
	if (ws.len == 0 || ws.len > WS_FRAME_MAX)
		return ESP_OK;
	uint8_t buf[WS_FRAME_MAX];
	ws.payload = buf;
	ret = httpd_ws_recv_frame(req, &ws, ws.len);
	if (ret != ESP_OK)
		return ret;
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
	const char *json = inserted ? "{\"floppy\":true}" : "{\"floppy\":false}";
	return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
}

/* Android connectivity probe: 204 with empty body = "this network has internet". */
static esp_err_t handle_generate_204(httpd_req_t *req)
{
	web_control_touch();
	httpd_resp_set_status(req, "204 No Content");
	return httpd_resp_send(req, NULL, 0);
}

/* iOS connectivity probe: 200 with Success body. */
static esp_err_t handle_hotspot_detect(httpd_req_t *req)
{
	web_control_touch();
	httpd_resp_set_type(req, "text/html");
	return httpd_resp_send(req, "<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>", HTTPD_RESP_USE_STRLEN);
}

/* 静态资源已合并进 index.html（web/dist）——不再单独嵌入 */

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

	/* 写盘：先 SD，失败 fallback RAMFS（/ram，无 SD 时）——再失败报错 */
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

	/* insert + persist 用实际写入的路径（/sdcard 或 /ram） */
	mac_msg_submit("floppy.insert", path);
	(void)mach_s3_settings_persist_set_floppy_path(path);
	httpd_resp_set_type(req, "application/json");
	return httpd_resp_send(req, "{\"ok\":true}", HTTPD_RESP_USE_STRLEN);
}

static void start_http_server(void)
{
	httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
	cfg.server_port = 80;
	/* lwIP MAX_SOCKETS=10, httpd holds 3 system sockets (listen/ctrl/msg) →
	 * 7 connection slots is the aligned ceiling. Written explicitly so a
	 * change to HTTPD_DEFAULT_CONFIG cannot silently shrink it. */
	cfg.max_open_sockets = 7;
	/* HTTP conns die after 1s idle (< web's 2s /api/status poll) → polling
	 * connections can't pile up and exhaust the socket pool ("error in accept
	 * (23)" = ENFILE). Each poll reconnects (LAN handshake is negligible). */
	cfg.recv_wait_timeout = 1;
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
/* Soft AP                                                            */
/* ------------------------------------------------------------------ */
static esp_err_t start_softap(void)
{
	ensure_nvs_init();

	esp_err_t err = esp_netif_init();
	if (err != ESP_OK)
		ESP_LOGE(TAG, "esp_netif_init: %s", esp_err_to_name(err));

	/* Event loop may already exist; invalid state is fine. */
	err = esp_event_loop_create_default();
	if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
		ESP_LOGE(TAG, "esp_event_loop_create_default: %s", esp_err_to_name(err));

	/* Netif is created once and reused across enable/disable cycles. */
	if (s_ap_netif == NULL) {
		s_ap_netif = esp_netif_create_default_wifi_ap();
		if (s_ap_netif == NULL)
			return ESP_ERR_NO_MEM;
	}

	wifi_init_config_t wic = WIFI_INIT_CONFIG_DEFAULT();
	err = esp_wifi_init(&wic);
	if (err != ESP_OK)
		return err;

	wifi_config_t wc = { .ap = {
	        .ssid_len = 0,
	        .max_connection = 4,
	        .authmode = WIFI_AUTH_WPA2_PSK,
	        .channel = 1,
	} };
	strcpy((char *)wc.ap.ssid, WEB_AP_SSID);
	strcpy((char *)wc.ap.password, WEB_AP_PASS);

	err = esp_wifi_set_mode(WIFI_MODE_AP);
	if (err != ESP_OK)
		return err;
	err = esp_wifi_set_config(WIFI_IF_AP, &wc);
	if (err != ESP_OK)
		return err;
	err = esp_wifi_start();
	if (err != ESP_OK)
		return err;

	ESP_LOGI(TAG, "softap '%s' started", WEB_AP_SSID);
	return ESP_OK;
}

/* Start the web control stack (softAP + http server). Idempotent. */
esp_err_t web_control_enable(void)
{
	if (s_ap_started)
		return ESP_OK;

	web_control_touch();
	esp_err_t err = start_softap();
	if (err != ESP_OK) {
		/* best-effort cleanup of whatever partially started */
		esp_wifi_stop();
		esp_wifi_deinit();
		ESP_LOGE(TAG, "softap start failed: %s", esp_err_to_name(err));
		return err;
	}
	/* Mark started before the httpd step so a later failure cleans up fully. */
	s_ap_started = true;

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

	/* Idle watchdog: 1s tick, auto-off after 60s without activity. */
	if (s_auto_off_timer == NULL) {
		esp_timer_create_args_t args = {
		        .callback = auto_off_cb, .name = "web-auto-off",
		};
		esp_timer_create(&args, &s_auto_off_timer);
	}
	esp_timer_start_periodic(s_auto_off_timer, 1000000);

	ESP_LOGI(TAG, "web control enabled");
	return ESP_OK;
}

/* Stop the web control stack and release its RAM. Idempotent. */
void web_control_disable(void)
{
	if (!s_ap_started)
		return;

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

	s_ap_started = false;
	ESP_LOGI(TAG, "web control disabled");
}

bool web_control_is_enabled(void)
{
	return s_ap_started;
}
