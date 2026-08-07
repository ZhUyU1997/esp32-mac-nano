#include <stdlib.h>
#include <stdint.h>

#include "SDL.h"

#include "driver.h"
#include "dt.h"
#include "sound.h"

class(sdl_sound_t, sound_t)
{
	SDL_AudioDeviceID dev;
	SDL_AudioSpec obtained;
	int started;
	int channels;
};

class_impl(sdl_sound_t, sound_t){};

destructor(sdl_sound_t)
{
	if (this->dev != 0) {
		SDL_CloseAudioDevice(this->dev);
		this->dev = 0;
	}
}

static sdl_sound_t *sdl_sound_priv(sound_t *snd)
{
	return dynamic_cast(sdl_sound_t)(snd);
}

static int sdl_sound_write(sound_t *snd, const uint16_t *buf, unsigned cnt)
{
	sdl_sound_t *p = sdl_sound_priv(snd);
	if (p == NULL || p->dev == 0 || buf == NULL || cnt == 0) {
		return -1;
	}

	if (p->channels <= 1) {
		int16_t tmp[512];
		int16_t *out = tmp;
		if (cnt > (unsigned)(sizeof(tmp) / sizeof(tmp[0]))) {
			out = (int16_t *)malloc((size_t)cnt * sizeof(int16_t));
			if (out == NULL) {
				return -1;
			}
		}
		for (unsigned i = 0; i < cnt; i++) {
			out[i] = (int16_t)(buf[i] ^ 0x8000u);
		}
		(void)SDL_QueueAudio(p->dev, out, (Uint32)(cnt * sizeof(int16_t)));
		if (out != tmp) {
			free(out);
		}
	} else {
		int16_t tmp[512 * 2];
		int16_t *out = tmp;
		size_t out_cnt = (size_t)cnt * 2u;
		if (out_cnt > (sizeof(tmp) / sizeof(tmp[0]))) {
			out = (int16_t *)malloc(out_cnt * sizeof(int16_t));
			if (out == NULL) {
				return -1;
			}
		}
		for (unsigned i = 0; i < cnt; i++) {
			int16_t v = (int16_t)(buf[i] ^ 0x8000u);
			out[2u * i + 0u] = v;
			out[2u * i + 1u] = v;
		}
		(void)SDL_QueueAudio(p->dev, out, (Uint32)(out_cnt * sizeof(int16_t)));
		if (out != tmp) {
			free(out);
		}
	}
	return (int)cnt;
}

static void sdl_sound_close(sound_t *snd)
{
	sdl_sound_t *p = sdl_sound_priv(snd);
	if (p == NULL) {
		return;
	}
	if (p->dev != 0) {
		SDL_ClearQueuedAudio(p->dev);
		SDL_PauseAudioDevice(p->dev, 1);
		SDL_CloseAudioDevice(p->dev);
		p->dev = 0;
	}
}

static void sdl_sound_output_task(sound_t *snd)
{
	(void)snd;
}

static device_t *snd_sdl_probe(driver_t *drv, dtnode_t *n)
{
	if (drv == NULL || n == NULL) {
		return NULL;
	}

	if (SDL_WasInit(SDL_INIT_AUDIO) == 0) {
		if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
			return NULL;
		}
	}

	int sample_rate = dt_read_int(n, "sample_rate", 22200);
	int channels = dt_read_int(n, "channels", 1);
	int samples = dt_read_int(n, "samples", 1024);
	if (sample_rate <= 0) {
		sample_rate = 22200;
	}
	if (channels <= 0) {
		channels = 1;
	}
	if (samples <= 0) {
		samples = 1024;
	}

	sdl_sound_t *obj = new (sdl_sound_t);
	if (obj == NULL) {
		return NULL;
	}

	sound_t *snd = dynamic_cast(sound_t)(obj);
	if (snd == NULL) {
		delete (obj);
		return NULL;
	}

	SDL_AudioSpec want;
	SDL_zero(want);
	want.freq = sample_rate;
	want.format = AUDIO_S16SYS;
	want.channels = (Uint8)channels;
	want.samples = (Uint16)samples;
	want.callback = NULL;

	obj->dev = SDL_OpenAudioDevice(NULL, 0, &want, &obj->obtained, 0);
	if (obj->dev == 0) {
		delete (obj);
		return NULL;
	}
	SDL_PauseAudioDevice(obj->dev, 0);
	obj->channels = (int)obj->obtained.channels;

	snd->write = sdl_sound_write;
	snd->close = sdl_sound_close;
	snd->output_task = sdl_sound_output_task;

	device_t *dev = register_sound(snd, drv, n);
	if (dev == NULL) {
		delete (obj);
		return NULL;
	}
	return dev;
}

impl(snd_sdl, driver_t){
        .name = "snd-sdl",
        .probe = snd_sdl_probe,
};
