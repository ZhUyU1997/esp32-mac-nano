#ifndef FRAME_BLIT_H
#define FRAME_BLIT_H

#include <stddef.h>
#include <stdint.h>
#include "fast_attr.h"

static inline void FAST_FUNC_ATTR blit_lvgl_i1_to_lcd_l8_rot90_bit1_white(uint8_t *restrict lcd_fb, const uint8_t *restrict px_bits, int lcd_w, int lcd_h)
{
	for (int dst_y = 0; dst_y < lcd_h; dst_y++) {
		uint32_t *dst_u32 = (uint32_t *)(lcd_fb + (size_t)dst_y * (size_t)lcd_w);
		const int src_x = (lcd_h - 1) - dst_y;
		const uint32_t src_byte_idx = (uint32_t)src_x >> 3;
		const uint8_t bit_idx = (uint8_t)((7 - src_x) & 7);
		const uint8_t *src_ptr = px_bits + (size_t)src_byte_idx;

		for (int dst_x = 0; dst_x < lcd_w; dst_x += 4, src_ptr += 320u) {
			const uint32_t b0 = ((uint32_t)(src_ptr[0] >> bit_idx) & 1u);
			const uint32_t b1 = ((uint32_t)(src_ptr[80u] >> bit_idx) & 1u);
			const uint32_t b2 = ((uint32_t)(src_ptr[160u] >> bit_idx) & 1u);
			const uint32_t b3 = ((uint32_t)(src_ptr[240u] >> bit_idx) & 1u);

			const uint32_t o0 = ((uint32_t)0 - b0) & 0xFFu;
			const uint32_t o1 = ((uint32_t)0 - b1) & 0xFFu;
			const uint32_t o2 = ((uint32_t)0 - b2) & 0xFFu;
			const uint32_t o3 = ((uint32_t)0 - b3) & 0xFFu;

			*dst_u32++ = o0 | (o1 << 8) | (o2 << 16) | (o3 << 24);
		}
	}
}

static inline void FAST_FUNC_ATTR blit_lvgl_i1_to_lcd_l8_rot90_bit1_black(uint8_t *restrict lcd_fb, const uint8_t *restrict px_bits, int lcd_w, int lcd_h)
{
	for (int dst_y = 0; dst_y < lcd_h; dst_y++) {
		uint32_t *dst_u32 = (uint32_t *)(lcd_fb + (size_t)dst_y * (size_t)lcd_w);
		const int src_x = (lcd_h - 1) - dst_y;
		const uint32_t src_byte_idx = (uint32_t)src_x >> 3;
		const uint8_t bit_idx = (uint8_t)((7 - src_x) & 7);
		const uint8_t *src_ptr = px_bits + (size_t)src_byte_idx;

		for (int dst_x = 0; dst_x < lcd_w; dst_x += 4, src_ptr += 320u) {
			const uint32_t b0 = ((uint32_t)(src_ptr[0] >> bit_idx) & 1u);
			const uint32_t b1 = ((uint32_t)(src_ptr[80u] >> bit_idx) & 1u);
			const uint32_t b2 = ((uint32_t)(src_ptr[160u] >> bit_idx) & 1u);
			const uint32_t b3 = ((uint32_t)(src_ptr[240u] >> bit_idx) & 1u);

			const uint32_t o0 = (~((uint32_t)0 - b0)) & 0xFFu;
			const uint32_t o1 = (~((uint32_t)0 - b1)) & 0xFFu;
			const uint32_t o2 = (~((uint32_t)0 - b2)) & 0xFFu;
			const uint32_t o3 = (~((uint32_t)0 - b3)) & 0xFFu;

			*dst_u32++ = o0 | (o1 << 8) | (o2 << 16) | (o3 << 24);
		}
	}
}

static inline void FAST_FUNC_ATTR blit_mac_mono_to_lcd_rgba(void *restrict lcd_fb, const uint8_t *restrict mac_fb, int lcd_h, int lcd_w)
{
	uint32_t *dst_u32 = (uint32_t *)lcd_fb;

	for (int dst_y = 0; dst_y < lcd_h; dst_y++) {
		const int src_x = (lcd_h - 1 - dst_y);
		const uint8_t bit_idx = (uint8_t)((7 - src_x) & 7);
		const int src_byte_idx = src_x >> 3;

		const uint8_t *src_ptr = mac_fb + src_byte_idx;
		for (int dst_x = 0; dst_x < lcd_w; dst_x += 4, src_ptr += 320) {
			const uint32_t b0 = ((uint32_t)(src_ptr[0] >> bit_idx) & 1u);
			const uint32_t b1 = ((uint32_t)(src_ptr[80] >> bit_idx) & 1u);
			const uint32_t b2 = ((uint32_t)(src_ptr[160] >> bit_idx) & 1u);
			const uint32_t b3 = ((uint32_t)(src_ptr[240] >> bit_idx) & 1u);

			const uint32_t m0 = (uint32_t)0 - b0;
			const uint32_t m1 = (uint32_t)0 - b1;
			const uint32_t m2 = (uint32_t)0 - b2;
			const uint32_t m3 = (uint32_t)0 - b3;

			const uint32_t o0 = (~m0) & 0x000000FFu;
			const uint32_t o1 = ((~m1) & 0x000000FFu) << 8;
			const uint32_t o2 = ((~m2) & 0x000000FFu) << 16;
			const uint32_t o3 = ((~m3) & 0x000000FFu) << 24;
			*dst_u32++ = o0 | o1 | o2 | o3;
		}
	}
}

#endif
