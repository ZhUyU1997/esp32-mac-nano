#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "SDL.h"

#include "probe.h"
#include "framebuffer.h"
#include "sound.h"
#include "block/block.h"
#include "macplus.h"
#include "snd.h"
#include "input.h"

#ifndef DISP_WIDTH
#define DISP_WIDTH 640
#endif

#ifndef DISP_HEIGHT
#define DISP_HEIGHT 480
#endif

#define MAC_AUDIO_HZ (MAC_SOUND_SAMPLES_PER_FRAME * 60)

static framebuffer_t *g_lcd;
static uint32_t *g_lcd_pixels;
static int g_lcd_w;
static int g_lcd_h;

static uint8_t *read_file(const char *path, size_t *out_sz)
{
	if (out_sz != NULL) {
		*out_sz = 0;
	}
	if (path == NULL || path[0] == '\0') {
		return NULL;
	}

	FILE *f = fopen(path, "rb");
	if (f == NULL) {
		return NULL;
	}
	if (fseek(f, 0, SEEK_END) != 0) {
		fclose(f);
		return NULL;
	}
	long n = ftell(f);
	if (n <= 0) {
		fclose(f);
		return NULL;
	}
	rewind(f);

	uint8_t *buf = (uint8_t *)malloc((size_t)n);
	if (buf == NULL) {
		fclose(f);
		return NULL;
	}
	size_t got = fread(buf, 1, (size_t)n, f);
	fclose(f);
	if (got != (size_t)n) {
		free(buf);
		return NULL;
	}
	if (out_sz != NULL) {
		*out_sz = (size_t)n;
	}
	return buf;
}

static uint8_t *read_file_search_up(const char *rel_path, const char *base_dir, int max_up, size_t *out_sz)
{
	if (rel_path == NULL || rel_path[0] == '\0') {
		return NULL;
	}
	if (rel_path[0] == '/') {
		return read_file(rel_path, out_sz);
	}
	if (base_dir == NULL || base_dir[0] == '\0') {
		return read_file(rel_path, out_sz);
	}

	char tmp[1024];
	for (int up = 0; up <= max_up; up++) {
		size_t pos = 0;
		size_t bl = strlen(base_dir);
		if (bl >= sizeof(tmp)) {
			break;
		}
		memcpy(tmp, base_dir, bl);
		pos = bl;
		for (int i = 0; i < up; i++) {
			const char *p = "../";
			size_t pl = 3;
			if (pos + pl >= sizeof(tmp)) {
				break;
			}
			memcpy(tmp + pos, p, pl);
			pos += pl;
		}
		size_t rl = strlen(rel_path);
		if (pos + rl >= sizeof(tmp)) {
			continue;
		}
		memcpy(tmp + pos, rel_path, rl);
		pos += rl;
		tmp[pos] = '\0';

		uint8_t *b = read_file(tmp, out_sz);
		if (b != NULL) {
			return b;
		}
	}
	return NULL;
}

static int file_exists(const char *path)
{
	if (path == NULL || path[0] == '\0') {
		return 0;
	}
	FILE *f = fopen(path, "rb");
	if (f == NULL) {
		return 0;
	}
	fclose(f);
	return 1;
}

static int resolve_path_search_up(char *out, size_t cap, const char *rel_path, const char *base_dir, int max_up)
{
	if (out == NULL || cap == 0) {
		return 0;
	}
	out[0] = '\0';

	if (rel_path == NULL || rel_path[0] == '\0') {
		return 0;
	}
	if (rel_path[0] == '/') {
		size_t n = strlen(rel_path);
		if (n + 1 > cap) {
			return 0;
		}
		memcpy(out, rel_path, n + 1);
		return 1;
	}
	if (base_dir == NULL || base_dir[0] == '\0') {
		size_t n = strlen(rel_path);
		if (n + 1 > cap) {
			return 0;
		}
		memcpy(out, rel_path, n + 1);
		return 1;
	}

	char tmp[1024];
	for (int up = 0; up <= max_up; up++) {
		size_t pos = 0;
		size_t bl = strlen(base_dir);
		if (bl >= sizeof(tmp)) {
			break;
		}
		memcpy(tmp, base_dir, bl);
		pos = bl;
		for (int i = 0; i < up; i++) {
			const char *p = "../";
			size_t pl = 3;
			if (pos + pl >= sizeof(tmp)) {
				break;
			}
			memcpy(tmp + pos, p, pl);
			pos += pl;
		}
		size_t rl = strlen(rel_path);
		if (pos + rl >= sizeof(tmp)) {
			continue;
		}
		memcpy(tmp + pos, rel_path, rl);
		pos += rl;
		tmp[pos] = '\0';
		if (!file_exists(tmp)) {
			continue;
		}
		size_t tn = strlen(tmp);
		if (tn + 1 > cap) {
			return 0;
		}
		memcpy(out, tmp, tn + 1);
		return 1;
	}

	size_t n = strlen(rel_path);
	if (n + 1 > cap) {
		return 0;
	}
	memcpy(out, rel_path, n + 1);
	return 1;
}

static void blit_mac_1bpp_to_argb(uint32_t *dst, int dst_w, int dst_h, const uint8_t *src_1bpp, int src_w, int src_h)
{
	if (dst == NULL || src_1bpp == NULL) {
		return;
	}
	if (dst_w <= 0 || dst_h <= 0 || src_w <= 0 || src_h <= 0) {
		return;
	}
	if (dst_w != src_w || dst_h != src_h) {
		return;
	}

	int row_bytes = (src_w + 7) / 8;
	for (int y = 0; y < src_h; y++) {
		const uint8_t *row = src_1bpp + (size_t)y * (size_t)row_bytes;
		uint32_t *out = dst + (size_t)y * (size_t)dst_w;
		for (int x = 0; x < src_w; x++) {
			uint8_t b = row[x >> 3];
			uint8_t m = (uint8_t)(1u << (7 - (x & 7)));
			out[x] = (b & m) ? 0xff000000u : 0xffffffffu;
		}
	}
}

static void on_mac_frame(uint8_t *mac_fb, void *ctx)
{
	(void)ctx;
	if (g_lcd == NULL || g_lcd_pixels == NULL) {
		return;
	}
	blit_mac_1bpp_to_argb(g_lcd_pixels, g_lcd_w, g_lcd_h, mac_fb, DISP_WIDTH, DISP_HEIGHT);
}

static input_keycode_t sdl_scancode_to_key(SDL_Scancode sc)
{
	switch (sc) {
	case SDL_SCANCODE_A:
		return INPUT_KEY_A;
	case SDL_SCANCODE_B:
		return INPUT_KEY_B;
	case SDL_SCANCODE_C:
		return INPUT_KEY_C;
	case SDL_SCANCODE_D:
		return INPUT_KEY_D;
	case SDL_SCANCODE_E:
		return INPUT_KEY_E;
	case SDL_SCANCODE_F:
		return INPUT_KEY_F;
	case SDL_SCANCODE_G:
		return INPUT_KEY_G;
	case SDL_SCANCODE_H:
		return INPUT_KEY_H;
	case SDL_SCANCODE_I:
		return INPUT_KEY_I;
	case SDL_SCANCODE_J:
		return INPUT_KEY_J;
	case SDL_SCANCODE_K:
		return INPUT_KEY_K;
	case SDL_SCANCODE_L:
		return INPUT_KEY_L;
	case SDL_SCANCODE_M:
		return INPUT_KEY_M;
	case SDL_SCANCODE_N:
		return INPUT_KEY_N;
	case SDL_SCANCODE_O:
		return INPUT_KEY_O;
	case SDL_SCANCODE_P:
		return INPUT_KEY_P;
	case SDL_SCANCODE_Q:
		return INPUT_KEY_Q;
	case SDL_SCANCODE_R:
		return INPUT_KEY_R;
	case SDL_SCANCODE_S:
		return INPUT_KEY_S;
	case SDL_SCANCODE_T:
		return INPUT_KEY_T;
	case SDL_SCANCODE_U:
		return INPUT_KEY_U;
	case SDL_SCANCODE_V:
		return INPUT_KEY_V;
	case SDL_SCANCODE_W:
		return INPUT_KEY_W;
	case SDL_SCANCODE_X:
		return INPUT_KEY_X;
	case SDL_SCANCODE_Y:
		return INPUT_KEY_Y;
	case SDL_SCANCODE_Z:
		return INPUT_KEY_Z;

	case SDL_SCANCODE_1:
		return INPUT_KEY_1;
	case SDL_SCANCODE_2:
		return INPUT_KEY_2;
	case SDL_SCANCODE_3:
		return INPUT_KEY_3;
	case SDL_SCANCODE_4:
		return INPUT_KEY_4;
	case SDL_SCANCODE_5:
		return INPUT_KEY_5;
	case SDL_SCANCODE_6:
		return INPUT_KEY_6;
	case SDL_SCANCODE_7:
		return INPUT_KEY_7;
	case SDL_SCANCODE_8:
		return INPUT_KEY_8;
	case SDL_SCANCODE_9:
		return INPUT_KEY_9;
	case SDL_SCANCODE_0:
		return INPUT_KEY_0;

	case SDL_SCANCODE_RETURN:
		return INPUT_KEY_ENTER;
	case SDL_SCANCODE_ESCAPE:
		return INPUT_KEY_ESC;
	case SDL_SCANCODE_BACKSPACE:
		return INPUT_KEY_BACKSPACE;
	case SDL_SCANCODE_TAB:
		return INPUT_KEY_TAB;
	case SDL_SCANCODE_SPACE:
		return INPUT_KEY_SPACE;

	case SDL_SCANCODE_MINUS:
		return INPUT_KEY_MINUS;
	case SDL_SCANCODE_EQUALS:
		return INPUT_KEY_EQUAL;
	case SDL_SCANCODE_LEFTBRACKET:
		return INPUT_KEY_LEFTBRACE;
	case SDL_SCANCODE_RIGHTBRACKET:
		return INPUT_KEY_RIGHTBRACE;
	case SDL_SCANCODE_BACKSLASH:
		return INPUT_KEY_BACKSLASH;
	case SDL_SCANCODE_SEMICOLON:
		return INPUT_KEY_SEMICOLON;
	case SDL_SCANCODE_APOSTROPHE:
		return INPUT_KEY_APOSTROPHE;
	case SDL_SCANCODE_GRAVE:
		return INPUT_KEY_GRAVE;
	case SDL_SCANCODE_COMMA:
		return INPUT_KEY_COMMA;
	case SDL_SCANCODE_PERIOD:
		return INPUT_KEY_DOT;
	case SDL_SCANCODE_SLASH:
		return INPUT_KEY_SLASH;

	case SDL_SCANCODE_INSERT:
		return INPUT_KEY_INSERT;
	case SDL_SCANCODE_HOME:
		return INPUT_KEY_HOME;
	case SDL_SCANCODE_PAGEUP:
		return INPUT_KEY_PAGEUP;
	case SDL_SCANCODE_DELETE:
		return INPUT_KEY_DELETE;
	case SDL_SCANCODE_END:
		return INPUT_KEY_END;
	case SDL_SCANCODE_PAGEDOWN:
		return INPUT_KEY_PAGEDOWN;
	case SDL_SCANCODE_RIGHT:
		return INPUT_KEY_RIGHT;
	case SDL_SCANCODE_LEFT:
		return INPUT_KEY_LEFT;
	case SDL_SCANCODE_DOWN:
		return INPUT_KEY_DOWN;
	case SDL_SCANCODE_UP:
		return INPUT_KEY_UP;
	case SDL_SCANCODE_F12:
		return INPUT_KEY_F12;
	default:
		return INPUT_KEY_NONE;
	}
}

static uint8_t sdl_mods_to_hid(uint16_t mod)
{
	uint8_t m = 0;
	if (mod & KMOD_LCTRL)
		m |= 1u << 0;
	if (mod & KMOD_LSHIFT)
		m |= 1u << 1;
	if (mod & KMOD_LALT)
		m |= 1u << 2;
	if (mod & KMOD_LGUI)
		m |= 1u << 3;
	if (mod & KMOD_RCTRL)
		m |= 1u << 4;
	if (mod & KMOD_RSHIFT)
		m |= 1u << 5;
	if (mod & KMOD_RALT)
		m |= 1u << 6;
	if (mod & KMOD_RGUI)
		m |= 1u << 7;
	return m;
}

static inline bool key_in_buf(const input_keycode_t *buf, input_keycode_t k, int n)
{
	for (int i = 0; i < n; i++) {
		if (buf[i] == k) {
			return true;
		}
	}
	return false;
}

static void post_keyboard_if_changed(uint8_t *prev_mod, input_keycode_t *prev_keys)
{
	const uint8_t *state = SDL_GetKeyboardState(NULL);
	uint8_t mod = sdl_mods_to_hid((uint16_t)SDL_GetModState());

	input_keycode_t keys[6] = {0};
	int n = 0;
	for (int sc = 0; sc < SDL_NUM_SCANCODES && n < (int)(sizeof(keys) / sizeof(keys[0])); sc++) {
		if (!state[sc]) {
			continue;
		}
		input_keycode_t k = sdl_scancode_to_key((SDL_Scancode)sc);
		if (k == INPUT_KEY_NONE) {
			continue;
		}
		keys[n++] = k;
	}

	if (*prev_mod == mod && memcmp(prev_keys, keys, sizeof(keys)) == 0) {
		return;
	}
	const input_keycode_t mod_to_key[8] = {
	        INPUT_KEY_LEFTCTRL,
	        INPUT_KEY_LEFTSHIFT,
	        INPUT_KEY_LEFTALT,
	        INPUT_KEY_LEFTMETA,
	        INPUT_KEY_RIGHTCTRL,
	        INPUT_KEY_RIGHTSHIFT,
	        INPUT_KEY_RIGHTALT,
	        INPUT_KEY_RIGHTMETA,
	};
	for (int b = 0; b < 8; b++) {
		uint8_t m = (uint8_t)(1u << b);
		if ((mod & m) == (*prev_mod & m)) {
			continue;
		}
		input_keycode_t k = mod_to_key[b];
		if (k != INPUT_KEY_NONE) {
			input_post_key(k, (mod & m) ? 1u : 0u);
		}
	}

	for (int i = 0; i < (int)(sizeof(keys) / sizeof(keys[0])); i++) {
		input_keycode_t k = prev_keys[i];
		if (k == INPUT_KEY_NONE) {
			continue;
		}
		if (!key_in_buf(keys, k, (int)(sizeof(keys) / sizeof(keys[0])))) {
			input_post_key(k, 0u);
		}
	}

	for (int i = 0; i < (int)(sizeof(keys) / sizeof(keys[0])); i++) {
		input_keycode_t k = keys[i];
		if (k == INPUT_KEY_NONE) {
			continue;
		}
		if (!key_in_buf(prev_keys, k, (int)(sizeof(keys) / sizeof(keys[0])))) {
			input_post_key(k, 1u);
		}
	}

	*prev_mod = mod;
	memcpy(prev_keys, keys, sizeof(keys));
}

static uint16_t clamp_u16(int v, int lo, int hi)
{
	if (v < lo) {
		v = lo;
	}
	if (v > hi) {
		v = hi;
	}
	return (uint16_t)v;
}

static char *str_replace_all(char *in, const char *token, const char *value)
{
	if (in == NULL || token == NULL || token[0] == '\0' || value == NULL) {
		return in;
	}

	size_t in_len = strlen(in);
	size_t tok_len = strlen(token);
	size_t val_len = strlen(value);

	if (tok_len == 0 || in_len < tok_len) {
		return in;
	}

	size_t count = 0;
	for (char *p = in;;) {
		char *hit = strstr(p, token);
		if (hit == NULL) {
			break;
		}
		count++;
		p = hit + tok_len;
	}
	if (count == 0) {
		return in;
	}

	size_t out_len = in_len + count * (val_len - tok_len);
	char *out = (char *)malloc(out_len + 1);
	if (out == NULL) {
		return in;
	}

	char *w = out;
	char *p = in;
	for (;;) {
		char *hit = strstr(p, token);
		if (hit == NULL) {
			size_t tail = strlen(p);
			memcpy(w, p, tail);
			w += tail;
			break;
		}
		size_t head = (size_t)(hit - p);
		memcpy(w, p, head);
		w += head;
		memcpy(w, value, val_len);
		w += val_len;
		p = hit + tok_len;
	}
	*w = '\0';
	free(in);
	return out;
}

int main(int argc, char **argv)
{
	const char *rom_path = "macintosh/rom.bin";
	const char *romex_path = "macintosh/pcex/pcex_mmio.rom";
	const char *hd_path = "macintosh/hd.img";
	const char *fd_path = "macintosh/fd.img";
	int frames_limit = 0;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--frames") == 0 && (i + 1) < argc) {
			frames_limit = atoi(argv[++i]);
		} else if (strcmp(argv[i], "--rom") == 0 && (i + 1) < argc) {
			rom_path = argv[++i];
		} else if (strcmp(argv[i], "--romex") == 0 && (i + 1) < argc) {
			romex_path = argv[++i];
		} else if (strcmp(argv[i], "--hd") == 0 && (i + 1) < argc) {
			hd_path = argv[++i];
		} else if (strcmp(argv[i], "--fd") == 0 && (i + 1) < argc) {
			fd_path = argv[++i];
		}
	}

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0) {
		fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
		return 1;
	}

	char *base_dir = SDL_GetBasePath();

	char hd_resolved[1024];
	char fd_resolved[1024];
	(void)resolve_path_search_up(hd_resolved, sizeof(hd_resolved), hd_path, base_dir, 6);
	(void)resolve_path_search_up(fd_resolved, sizeof(fd_resolved), fd_path, base_dir, 6);

	size_t tpl_sz = 0;
	uint8_t *tpl_raw = read_file_search_up("main/arch/x64/mach-sdl/dtree/mach-sdl.json", base_dir, 6, &tpl_sz);
	if (tpl_raw == NULL || tpl_sz == 0) {
		SDL_free(base_dir);
		SDL_Quit();
		return 1;
	}
	char *json = (char *)malloc(tpl_sz + 1);
	if (json == NULL) {
		free(tpl_raw);
		SDL_free(base_dir);
		SDL_Quit();
		return 1;
	}
	memcpy(json, tpl_raw, tpl_sz);
	json[tpl_sz] = '\0';
	free(tpl_raw);

	json = str_replace_all(json, "macintosh/hd.img", hd_resolved);
	json = str_replace_all(json, "macintosh/fd.img", fd_resolved);

	(void)probe_device(json, strlen(json));
	free(json);

	g_lcd = framebuffer_lookup("lcd");
	if (g_lcd == NULL) {
		SDL_free(base_dir);
		SDL_Quit();
		return 1;
	}
	(void)framebuffer_restart(g_lcd);
	g_lcd_pixels = (uint32_t *)framebuffer_get_framebuffer(g_lcd);
	g_lcd_w = g_lcd->width;
	g_lcd_h = g_lcd->height;

	sound_t *snd = sound_lookup("snd");
	if (snd == NULL) {
		SDL_free(base_dir);
		SDL_Quit();
		return 1;
	}

	size_t rom_sz = 0;
	size_t romex_sz = 0;
	uint8_t *rom = read_file_search_up(rom_path, base_dir, 6, &rom_sz);
	uint8_t *romex = read_file_search_up(romex_path, base_dir, 6, &romex_sz);
	if (rom == NULL || rom_sz == 0) {
		free(romex);
		free(rom);
		SDL_free(base_dir);
		SDL_Quit();
		return 1;
	}
	if (romex == NULL || romex_sz == 0) {
		romex = (uint8_t *)malloc(1);
		if (romex == NULL) {
			free(rom);
			SDL_free(base_dir);
			SDL_Quit();
			return 1;
		}
		romex[0] = 0xff;
		romex_sz = 1;
	}

	unsigned char *ram = malloc(MACPLUS_RAMSIZE);
	if (ram == NULL) {
		free(romex);
		free(rom);
		SDL_free(base_dir);
		SDL_Quit();
		return 1;
	}

	macplus_config_t config = {
	        .rom = rom,
	        .romex = romex,
	        .romex_size = romex_sz,
	        .sound = snd,
	        .hd = {
	                [6] = block_lookup("hd"),        /* SCSI ID 6: primary disk */
	                [0] = block_lookup("hd0-img"),   /* SCSI ID 0: extra disk */
	                [1] = block_lookup("hd1-img"),   /* SCSI ID 1 */
	                [2] = block_lookup("hd2-img"),   /* SCSI ID 2 */
	                [3] = block_lookup("hd3-img"),   /* SCSI ID 3 */
	                [4] = block_lookup("hd4-img"),   /* SCSI ID 4 */
	                [5] = block_lookup("hd5-img"),   /* SCSI ID 5 */
	                [7] = block_lookup("hd7-img"),   /* SCSI ID 7 */
	        },
	        .fd = block_lookup("fd"),
	        .frame_callback = on_mac_frame,
	};
	for (unsigned int i = 0; i < MACPLUS_RAMSIZE / MEMMAP_ES; i++) {
		config.ram[i] = &ram[i * MEMMAP_ES];
	}

	macplus_t *s = mac_get_instance(config);

	uint8_t prev_mod = 0;
	input_keycode_t prev_keys[6] = {0};
	uint16_t ax, ay;
	bool quit = false;
	int frames = 0;
	const int scale = 1;

	while (!quit) {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
			case SDL_QUIT:
				quit = true;
				break;
			default:
				break;
			}
		}

		post_keyboard_if_changed(&prev_mod, prev_keys);
		int mx = 0;
		int my = 0;
		uint32_t btns = SDL_GetMouseState(&mx, &my);
		int win_w = DISP_WIDTH * scale;
		int win_h = DISP_HEIGHT * scale;
		ax = clamp_u16(mx, 0, win_w > 0 ? (win_w - 1) : 0) / (uint16_t)(scale > 0 ? scale : 1);
		ay = clamp_u16(my, 0, win_h > 0 ? (win_h - 1) : 0) / (uint16_t)(scale > 0 ? scale : 1);

		input_report_mouse_button(INPUT_MOUSE_BTN_LEFT,   (btns & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0);
		input_report_mouse_button(INPUT_MOUSE_BTN_RIGHT,  (btns & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0);
		input_report_mouse_button(INPUT_MOUSE_BTN_MIDDLE, (btns & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0);
		input_post_mouse_move_abs(ax, ay);

		macplus_run_frame(s);
		(void)framebuffer_wait_vsync(g_lcd, 16);

		frames++;
		if (frames_limit > 0 && frames >= frames_limit) {
			break;
		}
	}

	mac_free(s);
	free(ram);
	free(romex);
	free(rom);
	SDL_free(base_dir);

	SDL_Quit();
	return 0;
}
