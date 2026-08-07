/*
 * HID Usage Tables — common usage page / usage ID constants.
 * Aligned with Linux include/linux/hid.h.
 */
#pragma once

/* ---- Usage Pages ---- */
#define HID_UP_GENDESK           0x01
#define HID_UP_SIMULATION        0x02
#define HID_UP_GENDEVCTRLS       0x06
#define HID_UP_KEYBOARD          0x07
#define HID_UP_LED               0x08
#define HID_UP_BUTTON            0x09
#define HID_UP_CONSUMER          0x0c
#define HID_UP_DIGITIZER         0x0d
#define HID_UP_BATTERY           0x85

/* ---- Generic Desktop (0x01) ---- */
#define HID_GD_POINTER           0x01
#define HID_GD_MOUSE             0x02
#define HID_GD_JOYSTICK          0x04
#define HID_GD_GAMEPAD           0x05
#define HID_GD_KEYBOARD          0x06
#define HID_GD_X                 0x30
#define HID_GD_Y                 0x31
#define HID_GD_Z                 0x32
#define HID_GD_WHEEL             0x38
#define HID_GD_SYSTEM_CONTROL    0x80

/* ---- Button (0x09) ---- */
#define HID_BTN_1                1
#define HID_BTN_2                2
#define HID_BTN_3                3
