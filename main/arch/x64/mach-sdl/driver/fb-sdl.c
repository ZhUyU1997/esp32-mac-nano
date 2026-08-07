#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "SDL.h"

#include "driver.h"
#include "dt.h"
#include "framebuffer.h"

class(sdl_fb_t, framebuffer_t)
{
	SDL_Window *win;
	SDL_Renderer *renderer;
	SDL_Texture *tex;
	uint32_t *pixels;
	int pitch_bytes;
	int scale;
	int brightness;
	int borderless;
};

class_impl(sdl_fb_t, framebuffer_t){};

destructor(sdl_fb_t)
{
	if (this->tex != NULL) {
		SDL_DestroyTexture(this->tex);
		this->tex = NULL;
	}
	if (this->renderer != NULL) {
		SDL_DestroyRenderer(this->renderer);
		this->renderer = NULL;
	}
	if (this->win != NULL) {
		SDL_DestroyWindow(this->win);
		this->win = NULL;
	}
	free(this->pixels);
	this->pixels = NULL;
}

static sdl_fb_t *sdl_fb_priv(framebuffer_t *fb)
{
	return dynamic_cast(sdl_fb_t)(fb);
}

static void sdl_fb_setbl(framebuffer_t *fb, int brightness)
{
	sdl_fb_t *p = sdl_fb_priv(fb);
	if (p == NULL) {
		return;
	}
	if (brightness < 0) {
		brightness = 0;
	}
	if (brightness > 100) {
		brightness = 100;
	}
	p->brightness = brightness;
}

static int sdl_fb_getbl(framebuffer_t *fb)
{
	sdl_fb_t *p = sdl_fb_priv(fb);
	if (p == NULL) {
		return -1;
	}
	return p->brightness;
}

static void *sdl_fb_getfb(framebuffer_t *fb)
{
	sdl_fb_t *p = sdl_fb_priv(fb);
	if (p == NULL) {
		return NULL;
	}
	return p->pixels;
}

static int sdl_fb_restart(framebuffer_t *fb)
{
	sdl_fb_t *p = sdl_fb_priv(fb);
	if (p == NULL) {
		return 0;
	}

	if (SDL_WasInit(SDL_INIT_VIDEO) == 0) {
		if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
			return 0;
		}
	}

	if (p->scale <= 0) {
		p->scale = 1;
	}

	if (p->win == NULL) {
		int ww = fb->width * p->scale;
		int wh = fb->height * p->scale;
		uint32_t flags = SDL_WINDOW_SHOWN;
		if (p->borderless) {
			flags |= SDL_WINDOW_BORDERLESS;
		}

		p->win = SDL_CreateWindow("mini-mac (SDL)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, ww, wh, flags);
		if (p->win == NULL) {
			return 0;
		}
		SDL_SetWindowResizable(p->win, SDL_FALSE);
		SDL_SetWindowMinimumSize(p->win, ww, wh);
		SDL_SetWindowMaximumSize(p->win, ww, wh);
	}

	if (p->renderer == NULL) {
		p->renderer = SDL_CreateRenderer(p->win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
		if (p->renderer == NULL) {
			p->renderer = SDL_CreateRenderer(p->win, -1, 0);
			if (p->renderer == NULL) {
				return 0;
			}
		}
		(void)SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	}

	if (p->tex == NULL) {
		p->tex = SDL_CreateTexture(p->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, fb->width, fb->height);
		if (p->tex == NULL) {
			return 0;
		}
	}

	if (p->pixels == NULL) {
		size_t n = (size_t)fb->width * (size_t)fb->height;
		p->pitch_bytes = fb->width * 4;
		p->pixels = (uint32_t *)calloc(n, sizeof(uint32_t));
		if (p->pixels == NULL) {
			return 0;
		}
	}

	return 1;
}

static int sdl_fb_wait_vsync(framebuffer_t *fb, uint32_t timeout_ms)
{
	(void)timeout_ms;

	sdl_fb_t *p = sdl_fb_priv(fb);
	if (p == NULL) {
		return 0;
	}

	if (p->renderer == NULL || p->tex == NULL || p->pixels == NULL) {
		return 0;
	}

	(void)SDL_UpdateTexture(p->tex, NULL, p->pixels, p->pitch_bytes);
	(void)SDL_RenderClear(p->renderer);
	(void)SDL_RenderCopy(p->renderer, p->tex, NULL, NULL);
	SDL_RenderPresent(p->renderer);
	return 1;
}

static device_t *fb_sdl_probe(driver_t *drv, dtnode_t *n)
{
	if (drv == NULL || n == NULL) {
		return NULL;
	}

	int w = dt_read_int(n, "width", 640);
	int h = dt_read_int(n, "height", 480);
	int scale = dt_read_int(n, "scale", 1);
	int brightness = dt_read_int(n, "brightness", 100);
	int borderless = dt_read_bool(n, "borderless", 0);

	if (w <= 0 || h <= 0) {
		return NULL;
	}

	sdl_fb_t *obj = new (sdl_fb_t);
	if (obj == NULL) {
		return NULL;
	}

	framebuffer_t *fb = dynamic_cast(framebuffer_t)(obj);
	if (fb == NULL) {
		delete (obj);
		return NULL;
	}

	obj->scale = scale;
	obj->brightness = brightness;
	obj->borderless = borderless ? 1 : 0;

	fb->width = w;
	fb->height = h;
	fb->pwidth = 0;
	fb->pheight = 0;
	fb->setbl = sdl_fb_setbl;
	fb->getbl = sdl_fb_getbl;
	fb->getfb = sdl_fb_getfb;
	fb->wait_vsync = sdl_fb_wait_vsync;
	fb->restart = sdl_fb_restart;
	fb->priv = NULL;

	if (!sdl_fb_restart(fb)) {
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

impl(fb_sdl, driver_t){
        .name = "fb-sdl",
        .probe = fb_sdl_probe,
};
