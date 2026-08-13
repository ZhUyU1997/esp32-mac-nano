/*
 * vterm_telnet.c — Telnet client: the ESP32 connects to the host's
 * telnet server (PC running telnetd or `nc -l`), displays the host's
 * terminal output on the vterm screen, and forwards the USB keyboard
 * input to the host.
 *
 * Data flow:
 *   host output -> recv -> ring buffer -> vterm_esp32 loop -> vterm display
 *   USB keyboard -> vterm_hid_map -> vterm_telnet_send() -> host
 */
#include <string.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"

#include "vterm_telnet.h"

/* host to connect to — edit these for your LAN */
#define TELNET_HOST_IP "192.168.31.173"
#define TELNET_PORT 2324 /* >1024: no root needed on the host */
#define TELNET_BUF 8192
#define TELNET_RETRY_MS 5000

static const char *TAG = "vterm-telnet";

/* ---- input ring buffer (host -> vterm loop) ---------------------------- */

static uint8_t s_ring[TELNET_BUF];
static size_t s_ring_head;
static size_t s_ring_tail;
static SemaphoreHandle_t s_ring_mutex;
static int s_fd = -1;
static SemaphoreHandle_t s_fd_mutex;

void vterm_telnet_pop(uint8_t *out, size_t *len, size_t cap)
{
	*len = 0;
	if (s_ring_mutex == NULL)
		return;
	xSemaphoreTake(s_ring_mutex, portMAX_DELAY);
	while (s_ring_head != s_ring_tail && *len < cap && *len < TELNET_BUF) {
		out[(*len)++] = s_ring[s_ring_tail];
		s_ring_tail = (s_ring_tail + 1) % TELNET_BUF;
	}
	xSemaphoreGive(s_ring_mutex);
}

static void telnet_push(const uint8_t *data, size_t len)
{
	if (s_ring_mutex == NULL)
		return;
	for (size_t i = 0; i < len; i++) {
		for (;;) {
			xSemaphoreTake(s_ring_mutex, portMAX_DELAY);
			size_t next = (s_ring_head + 1) % TELNET_BUF;
			if (next != s_ring_tail) {
				s_ring[s_ring_head] = data[i];
				s_ring_head = next;
				xSemaphoreGive(s_ring_mutex);
				break;
			}
			xSemaphoreGive(s_ring_mutex);
			/* ring full: wait for the consumer instead of dropping —
			 * stops recv() and lets TCP backpressure throttle the host */
			vTaskDelay(pdMS_TO_TICKS(1));
		}
	}
}

/* forward keyboard bytes to the host; false when not connected */
bool vterm_telnet_send(const uint8_t *data, size_t len)
{
	if (s_fd_mutex == NULL)
		return false;
	bool ok = false;
	xSemaphoreTake(s_fd_mutex, portMAX_DELAY);
	if (s_fd >= 0) {
		ok = send(s_fd, data, len, 0) == (int)len;
	}
	xSemaphoreGive(s_fd_mutex);
	return ok;
}

/* ---- IAC negotiation (client side) ------------------------------------- */

static void telnet_reply(int fd, uint8_t cmd, uint8_t opt)
{
	uint8_t reply[3] = { 0xff, cmd, opt };
	(void)send(fd, reply, 3, 0);
}

/* Parse a telnet byte stream: plain data bytes are pushed to the ring,
 * IAC negotiation is answered (or skipped). A trailing byte that starts an
 * incomplete IAC sequence is held in s_iac_pending and resumed on the next
 * recv() so a split sequence is not mistaken for data. */
static uint8_t s_iac_pending[2];
static size_t s_iac_pending_len;

static void telnet_process(int fd, const uint8_t *data, size_t len)
{
	/* The pending prefix is at most 2 bytes (0xff or 0xff <cmd>); the recv
	 * buffer is 512 bytes, so the combined buffer below is enough. */
	static uint8_t full[512 + 2];
	const uint8_t *src;
	size_t total;

	if (s_iac_pending_len) {
		memcpy(full, s_iac_pending, s_iac_pending_len);
		memcpy(full + s_iac_pending_len, data, len);
		total = s_iac_pending_len + len;
		src = full;
	} else {
		src = data;
		total = len;
	}

	size_t i = 0;
	while (i < total) {
		if (src[i] != 0xff) {
			telnet_push(&src[i], 1);
			i++;
			continue;
		}
		/* IAC */
		if (i + 1 >= total) {
			/* lone trailing 0xff: hold it for the next recv */
			break;
		}
		uint8_t cmd = src[i + 1];
		if (cmd == 0xff) {
			/* escaped 0xff -> literal data byte */
			telnet_push(&src[i + 1], 1);
			i += 2;
			continue;
		}
		if (cmd == 0xfa) { /* SB: skip until IAC SE */
			size_t j = i + 2;
			while (j + 1 < total && !(src[j] == 0xff && src[j + 1] == 0xf0))
				j++;
			if (j + 1 >= total) {
				/* incomplete SB: drop it (not used by our server) */
				i = total;
				break;
			}
			i = j + 2;
			continue;
		}
		if (i + 2 >= total) {
			/* 0xff <cmd> split across recv: hold both bytes */
			break;
		}
		uint8_t opt = src[i + 2];
		switch (cmd) {
		case 0xfb: /* WILL */
			if (opt == 0x01)      /* ECHO: let the host echo */
				telnet_reply(fd, 0xfd, opt); /* DO */
			else if (opt == 0x03) /* SUPPRESS-GO-AHEAD */
				telnet_reply(fd, 0xfd, opt);
			else
				telnet_reply(fd, 0xfe, opt); /* DONT */
			break;
		case 0xfd: /* DO */
			if (opt == 0x01) /* ECHO: we do not echo */
				telnet_reply(fd, 0xfc, opt); /* WONT */
			else if (opt == 0x03)
				telnet_reply(fd, 0xfb, opt); /* WILL */
			else
				telnet_reply(fd, 0xfc, opt);
			break;
		default:
			break; /* DONT/WONT/other: ignore */
		}
		i += 3;
	}

	/* save the unconsumed tail (a partial IAC sequence) for the next call */
	s_iac_pending_len = total - i;
	if (s_iac_pending_len)
		memcpy(s_iac_pending, src + i, s_iac_pending_len);
}

/* ---- connection loop (reconnect on drop) -------------------------------- */

static void telnet_client_task(void *arg)
{
	(void)arg;
	uint8_t buf[512];

	while (true) {
		int fd = socket(AF_INET, SOCK_STREAM, 0);
		if (fd < 0) {
			vTaskDelay(pdMS_TO_TICKS(TELNET_RETRY_MS));
			continue;
		}
		struct sockaddr_in addr = {
		        .sin_family = AF_INET,
		        .sin_port = htons(TELNET_PORT),
		};
		addr.sin_addr.s_addr = inet_addr(TELNET_HOST_IP);

		ESP_LOGI(TAG, "connecting to %s:%d...", TELNET_HOST_IP, TELNET_PORT);
		if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
			ESP_LOGW(TAG, "connect failed");
			close(fd);
			vTaskDelay(pdMS_TO_TICKS(TELNET_RETRY_MS));
			continue;
		}

		ESP_LOGI(TAG, "connected to host");
		s_iac_pending_len = 0; /* fresh connection: no partial IAC carry-over */
		xSemaphoreTake(s_fd_mutex, portMAX_DELAY);
		s_fd = fd;
		xSemaphoreGive(s_fd_mutex);

		while (true) {
			int n = recv(fd, buf, sizeof(buf), 0);
			if (n <= 0)
				break;
			telnet_process(fd, buf, (size_t)n);
		}

		ESP_LOGW(TAG, "disconnected, retrying");
		xSemaphoreTake(s_fd_mutex, portMAX_DELAY);
		s_fd = -1;
		xSemaphoreGive(s_fd_mutex);
		close(fd);
		vTaskDelay(pdMS_TO_TICKS(TELNET_RETRY_MS));
	}
}

void vterm_telnet_start(void)
{
	s_ring_mutex = xSemaphoreCreateMutex();
	s_fd_mutex = xSemaphoreCreateMutex();
	xTaskCreate(telnet_client_task, "telnet-cli", 4096, NULL, 5, NULL);
	ESP_LOGI(TAG, "telnet client task started (host %s:%d)", TELNET_HOST_IP, TELNET_PORT);
}
