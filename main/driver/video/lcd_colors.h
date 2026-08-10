/* lcd_colors.h - 64-color table for the 8bpp 6-wire RGB565 panel
 *
 * LCD_COLOR_XX is the framebuffer byte; RGB888 is shown in comments for
 * cross-reference. LCD_COLOR_ON selects the default lit-pixel color.
 *
 * Mapping: framebuffer byte bit7,6 -> R (R2R4=20, R1R3=10, /31)
 *                     bit5,4 -> G (G3G5=40, G2G4=20, /63)
 *                     bit3,2 -> B (B2B4=20, B1B3=10, /31)
 *                     bit0,1 -> not connected
 */
#ifndef LCD_COLORS_H
#define LCD_COLORS_H

#define LCD_COLOR_00 0x00u /* --c00: #000000 */
#define LCD_COLOR_04 0x04u /* --c04: #000052 */
#define LCD_COLOR_08 0x08u /* --c08: #0000A5 */
#define LCD_COLOR_0C 0x0Cu /* --c0c: #0000F7 */
#define LCD_COLOR_10 0x10u /* --c10: #005100 */
#define LCD_COLOR_14 0x14u /* --c14: #005152 */
#define LCD_COLOR_18 0x18u /* --c18: #0051A5 */
#define LCD_COLOR_1C 0x1Cu /* --c1c: #0051F7 */
#define LCD_COLOR_20 0x20u /* --c20: #00A200 */
#define LCD_COLOR_24 0x24u /* --c24: #00A252 */
#define LCD_COLOR_28 0x28u /* --c28: #00A2A5 */
#define LCD_COLOR_2C 0x2Cu /* --c2c: #00A2F7 */
#define LCD_COLOR_30 0x30u /* --c30: #00F300 */
#define LCD_COLOR_34 0x34u /* --c34: #00F352 */
#define LCD_COLOR_38 0x38u /* --c38: #00F3A5 */
#define LCD_COLOR_3C 0x3Cu /* --c3c: #00F3F7 */
#define LCD_COLOR_40 0x40u /* --c40: #520000 */
#define LCD_COLOR_44 0x44u /* --c44: #520052 */
#define LCD_COLOR_48 0x48u /* --c48: #5200A5 */
#define LCD_COLOR_4C 0x4Cu /* --c4c: #5200F7 */
#define LCD_COLOR_50 0x50u /* --c50: #525100 */
#define LCD_COLOR_54 0x54u /* --c54: #525152 */
#define LCD_COLOR_58 0x58u /* --c58: #5251A5 */
#define LCD_COLOR_5C 0x5Cu /* --c5c: #5251F7 */
#define LCD_COLOR_60 0x60u /* --c60: #52A200 */
#define LCD_COLOR_64 0x64u /* --c64: #52A252 */
#define LCD_COLOR_68 0x68u /* --c68: #52A2A5 */
#define LCD_COLOR_6C 0x6Cu /* --c6c: #52A2F7 */
#define LCD_COLOR_70 0x70u /* --c70: #52F300 */
#define LCD_COLOR_74 0x74u /* --c74: #52F352 */
#define LCD_COLOR_78 0x78u /* --c78: #52F3A5 */
#define LCD_COLOR_7C 0x7Cu /* --c7c: #52F3F7 */
#define LCD_COLOR_80 0x80u /* --c80: #A50000 */
#define LCD_COLOR_84 0x84u /* --c84: #A50052 */
#define LCD_COLOR_88 0x88u /* --c88: #A500A5 */
#define LCD_COLOR_8C 0x8Cu /* --c8c: #A500F7 */
#define LCD_COLOR_90 0x90u /* --c90: #A55100 */
#define LCD_COLOR_94 0x94u /* --c94: #A55152 */
#define LCD_COLOR_98 0x98u /* --c98: #A551A5 */
#define LCD_COLOR_9C 0x9Cu /* --c9c: #A551F7 */
#define LCD_COLOR_A0 0xA0u /* --ca0: #A5A200 */
#define LCD_COLOR_A4 0xA4u /* --ca4: #A5A252 */
#define LCD_COLOR_A8 0xA8u /* --ca8: #A5A2A5 */
#define LCD_COLOR_AC 0xACu /* --cac: #A5A2F7 */
#define LCD_COLOR_B0 0xB0u /* --cb0: #A5F300 */
#define LCD_COLOR_B4 0xB4u /* --cb4: #A5F352 */
#define LCD_COLOR_B8 0xB8u /* --cb8: #A5F3A5 */
#define LCD_COLOR_BC 0xBCu /* --cbc: #A5F3F7 */
#define LCD_COLOR_C0 0xC0u /* --cc0: #F70000 */
#define LCD_COLOR_C4 0xC4u /* --cc4: #F70052 */
#define LCD_COLOR_C8 0xC8u /* --cc8: #F700A5 */
#define LCD_COLOR_CC 0xCCu /* --ccc: #F700F7 */
#define LCD_COLOR_D0 0xD0u /* --cd0: #F75100 */
#define LCD_COLOR_D4 0xD4u /* --cd4: #F75152 */
#define LCD_COLOR_D8 0xD8u /* --cd8: #F751A5 */
#define LCD_COLOR_DC 0xDCu /* --cdc: #F751F7 */
#define LCD_COLOR_E0 0xE0u /* --ce0: #F7A200 */
#define LCD_COLOR_E4 0xE4u /* --ce4: #F7A252 */
#define LCD_COLOR_E8 0xE8u /* --ce8: #F7A2A5 */
#define LCD_COLOR_EC 0xECu /* --cec: #F7A2F7 */
#define LCD_COLOR_F0 0xF0u /* --cf0: #F7F300 */
#define LCD_COLOR_F4 0xF4u /* --cf4: #F7F352 */
#define LCD_COLOR_F8 0xF8u /* --cf8: #F7F3A5 */
#define LCD_COLOR_FC 0xFCu /* --cfc: #F7F3F7 */

#define LCD_COLOR_OFF LCD_COLOR_00 /* --c00: #000000 black */
/* LCD_COLOR_ON - real Mac 128K P4 white is cool-tinted:
 * P4 CIE (0.270,0.300) -> sRGB ~(125,199,255),
 * Lospec "Mac Paint" 2021 CRT sample (139,200,254).
 * 0xAC (165,162,247) is the closest in distance but reads purple on
 * panel (G<R); 0xBC (165,243,247) keeps G>R and reads as bright
 * white, slightly cyan.
 * Refs:
 * - P4 phosphor CIE: https://web.archive.org/web/20070831164640/http://www.torrscientific.co.uk/phosphors.htm
 * - CRT sample palette: https://lospec.com/palette-list/mac-paint
 * - Mac 128K video spec (P4 aluminized):
 *   http://absurdengineering.org/library/MASTER%20Tech%20Info%20Library/Macintosh%20Hardware/Mac%20HW%20Specs/TIL01655%20-%20Macintosh%20128K%20and%20512K%20-%20Video%20Screen%20Specs%20(Discontinued%20).pdf
 */
#define LCD_COLOR_ON LCD_COLOR_BC

#endif /* LCD_COLORS_H */
