#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "driver.h"
#include "dt.h"
#include "framebuffer.h"

#include "fast_attr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_check.h"
#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_io_additions.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_st7701.h"
#include "driver/ledc.h"

#define ARRAY_8_WITH_SIZE(...) (uint8_t[]){__VA_ARGS__}, sizeof((uint8_t[]){__VA_ARGS__})

static const st7701_lcd_init_cmd_t lcd_init_cmds[] = {
        {0xFF, ARRAY_8_WITH_SIZE(0x77, 0x01, 0x00, 0x00, 0x13), 0},
        {0xEF, ARRAY_8_WITH_SIZE(0x08), 0},
        {0xFF, ARRAY_8_WITH_SIZE(0x77, 0x01, 0x00, 0x00, 0x10), 0},
        {0xC0, ARRAY_8_WITH_SIZE(0x4F, 0x00), 0},
        {0xC1, ARRAY_8_WITH_SIZE(0x11, 0x0C), 0},
        {0xC2, ARRAY_8_WITH_SIZE(0x07, 0x0A), 0},
        {0xC3, ARRAY_8_WITH_SIZE(0x83, 0x33, 0x1b), 0},
        {0xCC, ARRAY_8_WITH_SIZE(0x10), 0},
        {0xB0, ARRAY_8_WITH_SIZE(0x00, 0x0F, 0x18, 0x0D, 0x12, 0x07, 0x05, 0x08, 0x07, 0x21, 0x03, 0x10, 0x0F, 0x26, 0x2F, 0x1F), 0},
        {0xB1, ARRAY_8_WITH_SIZE(0x00, 0x1B, 0x20, 0x0C, 0x0E, 0x03, 0x08, 0x08, 0x08, 0x22, 0x05, 0x11, 0x0F, 0x2A, 0x32, 0x1F), 0},
        {0xFF, ARRAY_8_WITH_SIZE(0x77, 0x01, 0x00, 0x00, 0x11), 0},
        {0xB0, ARRAY_8_WITH_SIZE(0x35), 0},
        {0xB1, ARRAY_8_WITH_SIZE(0x6a), 0},
        {0xB2, ARRAY_8_WITH_SIZE(0x81), 0},
        {0xB3, ARRAY_8_WITH_SIZE(0x80), 0},
        {0xB5, ARRAY_8_WITH_SIZE(0x4E), 0},
        {0xB7, ARRAY_8_WITH_SIZE(0x85), 0},
        {0xB8, ARRAY_8_WITH_SIZE(0x21), 0},
        {0xC0, ARRAY_8_WITH_SIZE(0x09), 0},
        {0xC1, ARRAY_8_WITH_SIZE(0x78), 0},
        {0xC2, ARRAY_8_WITH_SIZE(0x78), 0},
        {0xD0, ARRAY_8_WITH_SIZE(0x88), 0},
        {0xE0, ARRAY_8_WITH_SIZE(0x00, 0xA0, 0x02), 0},
        {0xE1, ARRAY_8_WITH_SIZE(0x06, 0xA0, 0x08, 0xA0, 0x05, 0xA0, 0x07, 0xA0, 0x00, 0x44, 0x44), 0},
        {0xE2, ARRAY_8_WITH_SIZE(0x20, 0x20, 0x40, 0x40, 0x96, 0xA0, 0x00, 0x00, 0x96, 0xA0, 0x00, 0x00, 0x00), 0},
        {0xE3, ARRAY_8_WITH_SIZE(0x00, 0x00, 0x22, 0x22), 0},
        {0xE4, ARRAY_8_WITH_SIZE(0x44, 0x44), 0},
        {0xE5, ARRAY_8_WITH_SIZE(0x0E, 0x97, 0x10, 0xA0, 0x10, 0x99, 0x10, 0xA0, 0x0A, 0x93, 0x10, 0xA0, 0x0C, 0x95, 0x10, 0xA0), 0},
        {0xE6, ARRAY_8_WITH_SIZE(0x00, 0x00, 0x22, 0x22), 0},
        {0xE7, ARRAY_8_WITH_SIZE(0x44, 0x44), 0},
        {0xE8, ARRAY_8_WITH_SIZE(0x0D, 0x96, 0x10, 0xA0, 0x0F, 0x98, 0x10, 0xA0, 0x09, 0x92, 0x10, 0xA0, 0x0B, 0x94, 0x10, 0xA0), 0},
        {0xEB, ARRAY_8_WITH_SIZE(0x00, 0x01, 0x4E, 0x4E, 0x44, 0x88, 0x40), 0},
        {0xEC, ARRAY_8_WITH_SIZE(0x78, 0x00), 0},
        {0xED, ARRAY_8_WITH_SIZE(0xFF, 0xFA, 0x2F, 0x89, 0x76, 0x54, 0x01, 0xFF, 0xFF, 0x10, 0x45, 0x67, 0x98, 0xF2, 0xAF, 0xFF), 0},
        {0xEF, ARRAY_8_WITH_SIZE(0x08, 0x08, 0x08, 0x45, 0x3F, 0x54), 0},
        {0xFF, ARRAY_8_WITH_SIZE(0x77, 0x01, 0x00, 0x00, 0x13), 0},
        {0xE8, ARRAY_8_WITH_SIZE(0x00, 0x0E), 0},
        {0xE8, ARRAY_8_WITH_SIZE(0x00, 0x0C), 10},
        {0xE8, ARRAY_8_WITH_SIZE(0x00, 0x00), 0},
        {0xFF, ARRAY_8_WITH_SIZE(0x77, 0x01, 0x00, 0x00, 0x00), 0},
        {0x3A, ARRAY_8_WITH_SIZE(0x55), 0},
        {0x29, ARRAY_8_WITH_SIZE(0x00), 0},
        {0x11, (uint8_t[]){0x00}, 0, 120},
};

class(st7701_fb_t, framebuffer_t)
{
	esp_lcd_panel_handle_t panel;
	esp_lcd_panel_io_handle_t io;
	SemaphoreHandle_t vsync_sem;
	void *fb;
	int brightness;
	int backlight_gpio;
};

class_impl(st7701_fb_t, framebuffer_t){};

destructor(st7701_fb_t)
{
	if (this->panel != NULL) {
		esp_lcd_panel_del(this->panel);
		this->panel = NULL;
	}
	if (this->io != NULL) {
		esp_lcd_panel_io_del(this->io);
		this->io = NULL;
	}
	if (this->vsync_sem != NULL) {
		vSemaphoreDelete(this->vsync_sem);
		this->vsync_sem = NULL;
	}
}

#define st7701_fb_priv(fb) dynamic_cast(st7701_fb_t)(fb)

static bool FAST_FUNC_ATTR st7701_on_vsync(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *event_data, void *user_ctx)
{
	(void)panel;
	(void)event_data;
	st7701_fb_t *p = (st7701_fb_t *)user_ctx;
	if (p != NULL && p->vsync_sem != NULL) {
		BaseType_t higher_priority_woken = pdFALSE;
		xSemaphoreGiveFromISR(p->vsync_sem, &higher_priority_woken);
	}
	return true;
}

static esp_err_t st7701_backlight_pwm_init(int gpio_num)
{
	ledc_timer_config_t ledc_timer = {
	        .speed_mode = LEDC_LOW_SPEED_MODE,
	        .duty_resolution = LEDC_TIMER_13_BIT,
	        .timer_num = LEDC_TIMER_0,
	        .freq_hz = 5000,
	        .clk_cfg = LEDC_AUTO_CLK,
	};
	ESP_RETURN_ON_ERROR(ledc_timer_config(&ledc_timer), "fb-st7701", "ledc_timer_config failed");

	ledc_channel_config_t ledc_channel = {
	        .gpio_num = gpio_num,
	        .speed_mode = LEDC_LOW_SPEED_MODE,
	        .channel = LEDC_CHANNEL_0,
	        .intr_type = LEDC_INTR_DISABLE,
	        .timer_sel = LEDC_TIMER_0,
	        .duty = 0,
	        .hpoint = 0,
	};
	ESP_RETURN_ON_ERROR(ledc_channel_config(&ledc_channel), "fb-st7701", "ledc_channel_config failed");
	return ESP_OK;
}

static void st7701_backlight_set_percent(uint32_t percent)
{
	if (percent > 100) {
		percent = 100;
	}
	uint32_t duty_max = (1U << LEDC_TIMER_13_BIT) - 1U;
	uint32_t duty = (percent * duty_max) / 100U;
	ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty));
	ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
}

static void st7701_fb_setbl(framebuffer_t *fb, int brightness)
{
	st7701_fb_t *p = st7701_fb_priv(fb);
	if (p == NULL) {
		return;
	}
	if (brightness < 0) {
		brightness = 0;
	} else if (brightness > 100) {
		brightness = 100;
	}
	p->brightness = brightness;
	st7701_backlight_set_percent((uint32_t)brightness);
}

static int st7701_fb_getbl(framebuffer_t *fb)
{
	st7701_fb_t *p = st7701_fb_priv(fb);
	if (p == NULL) {
		return -1;
	}
	return p->brightness;
}

static void *st7701_fb_getfb(framebuffer_t *fb)
{
	st7701_fb_t *p = st7701_fb_priv(fb);
	if (p == NULL) {
		return NULL;
	}
	return p->fb;
}

static int st7701_fb_wait_vsync(framebuffer_t *fb, uint32_t timeout_ms)
{
	st7701_fb_t *p = st7701_fb_priv(fb);
	if (p == NULL || p->vsync_sem == NULL) {
		return 0;
	}
	return xSemaphoreTake(p->vsync_sem, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

static int st7701_fb_restart(framebuffer_t *fb)
{
	st7701_fb_t *p = st7701_fb_priv(fb);
	if (p == NULL || p->panel == NULL) {
		return 0;
	}
	esp_lcd_rgb_panel_restart(p->panel);
	return 1;
}

typedef struct {
	int spi_cs_gpio;
	int spi_scl_gpio;
	int spi_sda_gpio;
	int rgb_de_gpio;
	int rgb_pclk_gpio;
	int rgb_vsync_gpio;
	int rgb_hsync_gpio;
	int rgb_disp_gpio;
	int rgb_data0_gpio;
	int rgb_data1_gpio;
	int rgb_data2_gpio;
	int rgb_data3_gpio;
	int rgb_data4_gpio;
	int rgb_data5_gpio;
	int rst_gpio;
	int h_res;
	int v_res;
	int fb_bits_per_pixel;
	int panel_bits_per_pixel;
	int bounce_lines;
	uint32_t pclk_hz;
	uint32_t hsync_pulse_width;
	uint32_t hsync_back_porch;
	uint32_t hsync_front_porch;
	uint32_t vsync_pulse_width;
	uint32_t vsync_back_porch;
	uint32_t vsync_front_porch;
	int hsync_idle_low;
	int vsync_idle_low;
	int pclk_active_neg;
	int de_idle_high;
	int brightness;
} st7701_fb_dt_cfg_t;

static st7701_fb_dt_cfg_t st7701_fb_read_cfg(const dtnode_t *n)
{
	st7701_fb_dt_cfg_t c = {
	        .spi_cs_gpio = dt_read_int(n, "spi_cs_gpio", INT32_MIN),
	        .spi_scl_gpio = dt_read_int(n, "spi_scl_gpio", INT32_MIN),
	        .spi_sda_gpio = dt_read_int(n, "spi_sda_gpio", INT32_MIN),
	        .rgb_de_gpio = dt_read_int(n, "rgb_de_gpio", INT32_MIN),
	        .rgb_pclk_gpio = dt_read_int(n, "rgb_pclk_gpio", INT32_MIN),
	        .rgb_vsync_gpio = dt_read_int(n, "rgb_vsync_gpio", INT32_MIN),
	        .rgb_hsync_gpio = dt_read_int(n, "rgb_hsync_gpio", INT32_MIN),
	        .rgb_disp_gpio = dt_read_int(n, "rgb_disp_gpio", INT32_MIN),
	        .rgb_data0_gpio = dt_read_int(n, "rgb_data0_gpio", INT32_MIN),
	        .rgb_data1_gpio = dt_read_int(n, "rgb_data1_gpio", INT32_MIN),
	        .rgb_data2_gpio = dt_read_int(n, "rgb_data2_gpio", INT32_MIN),
	        .rgb_data3_gpio = dt_read_int(n, "rgb_data3_gpio", INT32_MIN),
	        .rgb_data4_gpio = dt_read_int(n, "rgb_data4_gpio", INT32_MIN),
	        .rgb_data5_gpio = dt_read_int(n, "rgb_data5_gpio", INT32_MIN),
	        .rst_gpio = dt_read_int(n, "rst_gpio", INT32_MIN),
	        .h_res = dt_read_int(n, "h_res", INT32_MIN),
	        .v_res = dt_read_int(n, "v_res", INT32_MIN),
	        .fb_bits_per_pixel = dt_read_int(n, "fb_bits_per_pixel", INT32_MIN),
	        .panel_bits_per_pixel = dt_read_int(n, "panel_bits_per_pixel", INT32_MIN),
	        .bounce_lines = dt_read_int(n, "bounce_lines", INT32_MIN),
	        .pclk_hz = dt_read_u32(n, "pclk_hz", UINT32_MAX),
	        .hsync_pulse_width = dt_read_u32(n, "hsync_pulse_width", UINT32_MAX),
	        .hsync_back_porch = dt_read_u32(n, "hsync_back_porch", UINT32_MAX),
	        .hsync_front_porch = dt_read_u32(n, "hsync_front_porch", UINT32_MAX),
	        .vsync_pulse_width = dt_read_u32(n, "vsync_pulse_width", UINT32_MAX),
	        .vsync_back_porch = dt_read_u32(n, "vsync_back_porch", UINT32_MAX),
	        .vsync_front_porch = dt_read_u32(n, "vsync_front_porch", UINT32_MAX),
	        .hsync_idle_low = dt_read_bool(n, "hsync_idle_low", 2),
	        .vsync_idle_low = dt_read_bool(n, "vsync_idle_low", 2),
	        .pclk_active_neg = dt_read_bool(n, "pclk_active_neg", 2),
	        .de_idle_high = dt_read_bool(n, "de_idle_high", 2),
	        .brightness = dt_read_int(n, "brightness", INT32_MIN),
	};
	return c;
}

static int st7701_fb_init_from_cfg(st7701_fb_t *p, const st7701_fb_dt_cfg_t *cfg)
{
	if (p == NULL || cfg == NULL) {
		return -1;
	}
	if (cfg->spi_cs_gpio == INT32_MIN || cfg->spi_scl_gpio == INT32_MIN || cfg->spi_sda_gpio == INT32_MIN) {
		return -1;
	}
	if (cfg->rgb_de_gpio == INT32_MIN || cfg->rgb_pclk_gpio == INT32_MIN || cfg->rgb_vsync_gpio == INT32_MIN || cfg->rgb_hsync_gpio == INT32_MIN) {
		return -1;
	}
	if (cfg->rgb_disp_gpio == INT32_MIN || cfg->rgb_data0_gpio == INT32_MIN || cfg->rgb_data1_gpio == INT32_MIN || cfg->rgb_data2_gpio == INT32_MIN ||
	    cfg->rgb_data3_gpio == INT32_MIN || cfg->rgb_data4_gpio == INT32_MIN || cfg->rgb_data5_gpio == INT32_MIN) {
		return -1;
	}
	if (cfg->rst_gpio == INT32_MIN) {
		return -1;
	}
	if (cfg->h_res == INT32_MIN || cfg->v_res == INT32_MIN || cfg->bounce_lines == INT32_MIN) {
		return -1;
	}
	if (cfg->fb_bits_per_pixel == INT32_MIN || cfg->panel_bits_per_pixel == INT32_MIN) {
		return -1;
	}
	if (cfg->pclk_hz == UINT32_MAX) {
		return -1;
	}
	if (cfg->hsync_pulse_width == UINT32_MAX || cfg->hsync_back_porch == UINT32_MAX || cfg->hsync_front_porch == UINT32_MAX ||
	    cfg->vsync_pulse_width == UINT32_MAX || cfg->vsync_back_porch == UINT32_MAX || cfg->vsync_front_porch == UINT32_MAX) {
		return -1;
	}
	if ((cfg->hsync_idle_low & ~1) || (cfg->vsync_idle_low & ~1) || (cfg->pclk_active_neg & ~1) || (cfg->de_idle_high & ~1)) {
		return -1;
	}
	if (cfg->brightness == INT32_MIN) {
		return -1;
	}
	if (cfg->h_res <= 0 || cfg->v_res <= 0) {
		return -1;
	}
	if (cfg->bounce_lines <= 0) {
		return -1;
	}
	if (cfg->fb_bits_per_pixel != 8) {
		return -1;
	}
	if (cfg->panel_bits_per_pixel != 16) {
		return -1;
	}

	p->vsync_sem = xSemaphoreCreateBinary();
	if (p->vsync_sem == NULL) {
		return -1;
	}

	spi_line_config_t line_config = {
	        .cs_io_type = IO_TYPE_GPIO,
	        .cs_gpio_num = cfg->spi_cs_gpio,
	        .scl_io_type = IO_TYPE_GPIO,
	        .scl_gpio_num = cfg->spi_scl_gpio,
	        .sda_io_type = IO_TYPE_GPIO,
	        .sda_gpio_num = cfg->spi_sda_gpio,
	        .io_expander = NULL,
	};
	esp_lcd_panel_io_3wire_spi_config_t io_config = {
	        .line_config = line_config,
	        .expect_clk_speed = PANEL_IO_3WIRE_SPI_CLK_MAX,
	        .spi_mode = 0,
	        .lcd_cmd_bytes = 1,
	        .lcd_param_bytes = 1,
	        .flags =
	                {
	                        .use_dc_bit = 1,
	                        .dc_zero_on_data = 0,
	                        .lsb_first = 0,
	                        .cs_high_active = 0,
	                        .del_keep_cs_inactive = 1,
	                },
	};
	ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_3wire_spi(&io_config, &p->io), "fb-st7701", "new_panel_io_3wire_spi failed");

	esp_lcd_rgb_panel_config_t rgb_config = {
	        .clk_src = LCD_CLK_SRC_DEFAULT,
	        .bounce_buffer_size_px = cfg->bounce_lines * cfg->h_res,
	        .psram_trans_align = 64,
	        .data_width = 8,
	        .bits_per_pixel = cfg->fb_bits_per_pixel,
	        .num_fbs = 1,
	        .de_gpio_num = cfg->rgb_de_gpio,
	        .pclk_gpio_num = cfg->rgb_pclk_gpio,
	        .vsync_gpio_num = cfg->rgb_vsync_gpio,
	        .hsync_gpio_num = cfg->rgb_hsync_gpio,
	        .disp_gpio_num = -1,
	        .data_gpio_nums =
	                {
	                        -1,
	                        -1,
	                        cfg->rgb_data0_gpio,
	                        cfg->rgb_data1_gpio,
	                        cfg->rgb_data2_gpio,
	                        cfg->rgb_data3_gpio,
	                        cfg->rgb_data4_gpio,
	                        cfg->rgb_data5_gpio,
	                },
	        .timings =
	                {
	                        .pclk_hz = cfg->pclk_hz,
	                        .h_res = cfg->h_res,
	                        .v_res = cfg->v_res,
	                        .hsync_pulse_width = cfg->hsync_pulse_width,
	                        .hsync_back_porch = cfg->hsync_back_porch,
	                        .hsync_front_porch = cfg->hsync_front_porch,
	                        .vsync_pulse_width = cfg->vsync_pulse_width,
	                        .vsync_back_porch = cfg->vsync_back_porch,
	                        .vsync_front_porch = cfg->vsync_front_porch,
	                        .flags =
	                                {
	                                        .hsync_idle_low = cfg->hsync_idle_low ? true : false,
	                                        .vsync_idle_low = cfg->vsync_idle_low ? true : false,
	                                        .pclk_active_neg = cfg->pclk_active_neg ? true : false,
	                                        .de_idle_high = cfg->de_idle_high ? true : false,
	                                },
	                },
	        .flags.fb_in_psram = 1,
	};

	st7701_vendor_config_t vendor_config = {
	        .rgb_config = &rgb_config,
	        .init_cmds = lcd_init_cmds,
	        .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(lcd_init_cmds[0]),
	        .flags =
	                {
	                        .mirror_by_cmd = 1,
	                        .enable_io_multiplex = 0,
	                },
	};
	const esp_lcd_panel_dev_config_t panel_config = {
	        .reset_gpio_num = cfg->rst_gpio,
	        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
	        .bits_per_pixel = cfg->panel_bits_per_pixel,
	        .vendor_config = &vendor_config,
	};

	ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st7701(p->io, &panel_config, &p->panel), "fb-st7701", "new_panel_st7701 failed");
	ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(p->panel), "fb-st7701", "panel_reset failed");
	ESP_RETURN_ON_ERROR(esp_lcd_panel_init(p->panel), "fb-st7701", "panel_init failed");

	esp_lcd_rgb_panel_event_callbacks_t cbs = {
	        .on_vsync = st7701_on_vsync,
	};
	ESP_RETURN_ON_ERROR(esp_lcd_rgb_panel_register_event_callbacks(p->panel, &cbs, p), "fb-st7701", "register_event_callbacks failed");

	void *fb = NULL;
	ESP_RETURN_ON_ERROR(esp_lcd_rgb_panel_get_frame_buffer(p->panel, 1, &fb), "fb-st7701", "get_frame_buffer failed");
	p->fb = fb;
	size_t bytes_per_pixel = (size_t)((cfg->fb_bits_per_pixel + 7) / 8);
	memset(p->fb, 0, (size_t)cfg->h_res * (size_t)cfg->v_res * bytes_per_pixel);
	ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(p->panel, true), "fb-st7701", "disp_on_off failed");

	p->backlight_gpio = cfg->rgb_disp_gpio;
	ESP_RETURN_ON_ERROR(st7701_backlight_pwm_init(p->backlight_gpio), "fb-st7701", "backlight_pwm_init failed");
	st7701_fb_setbl(dynamic_cast(framebuffer_t)(p), cfg->brightness);
	return 0;
}

static device_t *fb_st7701_probe(driver_t *drv, dtnode_t *n)
{
	if (drv == NULL || n == NULL) {
		return NULL;
	}

	st7701_fb_t *obj = new (st7701_fb_t);
	if (obj == NULL) {
		return NULL;
	}
	framebuffer_t *fb = dynamic_cast(framebuffer_t)(obj);
	if (fb == NULL) {
		delete (obj);
		return NULL;
	}

	st7701_fb_dt_cfg_t cfg = st7701_fb_read_cfg(n);
	fb->width = cfg.h_res;
	fb->height = cfg.v_res;
	fb->pwidth = 0;
	fb->pheight = 0;
	fb->setbl = st7701_fb_setbl;
	fb->getbl = st7701_fb_getbl;
	fb->getfb = st7701_fb_getfb;
	fb->wait_vsync = st7701_fb_wait_vsync;
	fb->restart = st7701_fb_restart;
	fb->priv = NULL;

	if (st7701_fb_init_from_cfg(obj, &cfg) != 0) {
		delete (obj);
		return NULL;
	}

	device_t *dev = register_framebuffer(fb, drv, n);
	if (dev == NULL) {
		delete (obj);
		return NULL;
	}
	return dev;
}

impl(fb_st7701, driver_t){
        .name = "fb-st7701",
        .probe = fb_st7701_probe,
};
