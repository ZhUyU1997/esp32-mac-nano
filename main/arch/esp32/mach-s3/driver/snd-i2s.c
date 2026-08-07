/*
 * ESP32 I2S backend for sound_t (device + DT).
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "driver.h"
#include "dt.h"
#include "sound.h"

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_common.h"
#include "driver/i2s_std.h"
#include "freertos/ringbuf.h"

#include "snd.h"

#define SND_FRAME_HZ 60
#define SAMPLE_RATE (MAC_SOUND_SAMPLES_PER_FRAME * SND_FRAME_HZ)

#define SND_CHUNKSZ 128
#define SND_RATE_MEASURE_US 1000000

typedef struct {
	int bclk_gpio;
	int ws_gpio;
	int dout_gpio;
	int din_gpio;
	int mclk_inv;
	int bclk_inv;
	int ws_inv;
	uint32_t ringbuf_bytes;
} snd_i2s_dt_cfg_t;

static snd_i2s_dt_cfg_t snd_i2s_read_cfg(const dtnode_t *n)
{
	snd_i2s_dt_cfg_t c = {
	        .bclk_gpio = dt_read_int(n, "bclk_gpio", INT32_MIN),
	        .ws_gpio = dt_read_int(n, "ws_gpio", INT32_MIN),
	        .dout_gpio = dt_read_int(n, "dout_gpio", INT32_MIN),
	        .din_gpio = dt_read_int(n, "din_gpio", INT32_MIN),
	        .mclk_inv = dt_read_bool(n, "mclk_inv", 2),
	        .bclk_inv = dt_read_bool(n, "bclk_inv", 2),
	        .ws_inv = dt_read_bool(n, "ws_inv", 2),
	        .ringbuf_bytes = dt_read_u32(n, "ringbuf_bytes", UINT32_MAX),
	};
	return c;
}

class(snd_i2s_t, sound_t)
{
	RingbufHandle_t rb;
	i2s_chan_handle_t i2s_tx;
	int64_t rate_t0_us;
	uint64_t rate_frames_acc;
};

class_impl(snd_i2s_t, sound_t){};

destructor(snd_i2s_t)
{
	if (this->rb) {
		vRingbufferDelete(this->rb);
		this->rb = NULL;
	}
	if (this->i2s_tx) {
		i2s_channel_disable(this->i2s_tx);
		i2s_del_channel(this->i2s_tx);
		this->i2s_tx = NULL;
	}
}

#define snd_i2s_priv(snd) dynamic_cast(snd_i2s_t)(snd)

static int snd_i2s_write(sound_t *snd, const uint16_t *buf, unsigned cnt)
{
	snd_i2s_t *p = snd_i2s_priv(snd);
	if (p == NULL || p->rb == NULL || buf == NULL || cnt == 0) {
		return -1;
	}
	if (xRingbufferSend(p->rb, buf, (size_t)cnt * sizeof(uint16_t), portMAX_DELAY) != pdTRUE) {
		return -1;
	}
	return 0;
}

static void snd_i2s_output_task(sound_t *snd)
{
	snd_i2s_t *p = snd_i2s_priv(snd);
	if (p == NULL || p->rb == NULL || p->i2s_tx == NULL) {
		return;
	}
	uint16_t raw[SND_CHUNKSZ];
	size_t w_bytes = 0;

	p->rate_t0_us = esp_timer_get_time();
	p->rate_frames_acc = 0;

	uint16_t last_val = 0x8000u;

	printf("Sound task started.\n");
	while (1) {
		size_t n = 0;
		/*
		 * Fill a chunk from the ring buffer with a two-level timeout:
		 *
		 *  - First read (n == 0): poll with timeout 0.  If the ring buffer
		 *    is completely empty the audio source has paused (e.g. settings
		 *    UI) — write silence to prevent I2S underrun noise.
		 *  - Subsequent reads (n > 0): wait up to 30 ms for the remaining
		 *    bytes.  During normal playback the next VBL arrives within
		 *    ~17 ms, so the wait succeeds; after a pause the timeout fires
		 *    and the tail of the chunk is filled with silence.
		 */
		while (n < sizeof(raw)) {
			size_t sz = 0;
			TickType_t to = (n == 0) ? 0 : pdMS_TO_TICKS(30);
			void *rp = xRingbufferReceiveUpTo(p->rb, &sz, to, sizeof(raw) - n);
			if (rp != NULL && sz > 0) {
				memcpy((uint8_t *)raw + n, rp, sz);
				n += sz;
				vRingbufferReturnItem(p->rb, rp);
			} else {
				break;
			}
		}
		if (n < sizeof(raw)) {
			unsigned filled = n / sizeof(uint16_t);
			uint16_t fill_val = (filled > 0) ? raw[filled - 1] : last_val;
			for (unsigned j = filled; j < SND_CHUNKSZ; j++) {
				raw[j] = fill_val;
			}
		}

		i2s_channel_write(p->i2s_tx, (char *)raw, sizeof(raw), &w_bytes, 1000);
		last_val = raw[SND_CHUNKSZ - 1];
		if (n > 0) {
			p->rate_frames_acc += w_bytes / 2;
			int64_t now = esp_timer_get_time();
			int64_t dt = now - p->rate_t0_us;
			if (dt >= SND_RATE_MEASURE_US) {
				snd->snd_hz = (int)((float)p->rate_frames_acc * 1e6f / (float)dt);
				p->rate_t0_us = now;
				p->rate_frames_acc = 0;
			}
		}
	}
}

static esp_err_t snd_i2s_init(snd_i2s_t *p, const snd_i2s_dt_cfg_t *cfg)
{
	if (p == NULL || cfg == NULL) {
		return ESP_ERR_INVALID_ARG;
	}
	if (cfg->bclk_gpio == INT32_MIN || cfg->ws_gpio == INT32_MIN || cfg->dout_gpio == INT32_MIN || cfg->din_gpio == INT32_MIN) {
		return ESP_ERR_INVALID_ARG;
	}
	if ((cfg->mclk_inv & ~1) || (cfg->bclk_inv & ~1) || (cfg->ws_inv & ~1)) {
		return ESP_ERR_INVALID_ARG;
	}
	if (cfg->ringbuf_bytes == UINT32_MAX || cfg->ringbuf_bytes == 0) {
		return ESP_ERR_INVALID_ARG;
	}

	p->rb = xRingbufferCreate((size_t)cfg->ringbuf_bytes, RINGBUF_TYPE_BYTEBUF);
	if (p->rb == NULL) {
		return ESP_ERR_NO_MEM;
	}

	i2s_chan_config_t tx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
	esp_err_t err = i2s_new_channel(&tx_chan_cfg, &p->i2s_tx, NULL);
	if (err != ESP_OK) {
		return err;
	}

	i2s_std_config_t tx_std_cfg = {
	        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
	        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
	        .gpio_cfg =
	                {
	                        .mclk = I2S_GPIO_UNUSED,
	                        .bclk = cfg->bclk_gpio,
	                        .ws = cfg->ws_gpio,
	                        .dout = cfg->dout_gpio,
	                        .din = (cfg->din_gpio < 0) ? I2S_GPIO_UNUSED : cfg->din_gpio,
	                        .invert_flags =
	                                {
	                                        .mclk_inv = cfg->mclk_inv ? true : false,
	                                        .bclk_inv = cfg->bclk_inv ? true : false,
	                                        .ws_inv = cfg->ws_inv ? true : false,
	                                },
	                },
	};

	err = i2s_channel_init_std_mode(p->i2s_tx, &tx_std_cfg);
	if (err != ESP_OK) {
		return err;
	}
	err = i2s_channel_enable(p->i2s_tx);
	if (err != ESP_OK) {
		return err;
	}

	return ESP_OK;
}

static device_t *snd_i2s_probe(driver_t *drv, dtnode_t *n)
{
	if (drv == NULL || n == NULL) {
		return NULL;
	}
	snd_i2s_dt_cfg_t cfg = snd_i2s_read_cfg(n);

	snd_i2s_t *obj = new (snd_i2s_t);
	if (obj == NULL) {
		return NULL;
	}
	sound_t *snd = dynamic_cast(sound_t)(obj);
	if (snd == NULL) {
		delete (obj);
		return NULL;
	}
	snd->write = snd_i2s_write;
	snd->close = NULL;
	snd->output_task = snd_i2s_output_task;

	if (snd_i2s_init(obj, &cfg) != ESP_OK) {
		delete (obj);
		return NULL;
	}

	device_t *dev = register_sound(snd, drv, n);
	if (dev == NULL) {
		delete (obj);
		return NULL;
	}
	return dev;
}

impl(snd_i2s, driver_t){
        .name = "snd-i2s",
        .probe = snd_i2s_probe,
};
