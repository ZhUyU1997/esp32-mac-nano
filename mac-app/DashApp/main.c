#include <Dialogs.h>
#include <Events.h>
#include <Fonts.h>
#include <Menus.h>
#include <Quickdraw.h>
#include <TextEdit.h>
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <Memory.h>
#include <Timer.h>

/* ------------------------------------------------------------------ */
/* Shared memory layout (must match sensor_shm.h)                      */
/* ------------------------------------------------------------------ */
#define SHM_BASE           ((volatile uint8_t *)0xF00000UL)
#define SHM_CMD            0x00
#define SHM_STATUS         0x01
#define SHM_TEMP           0x02   /* int16 BE: temp x 10 */
#define SHM_HUMI           0x04   /* int16 BE: humi x 10 */
#define SHM_TIME           0x0A   /* 7 bytes BCD: s/m/h/d/M/Y/wday */
#define SHM_GEN            0x06   /* uint32 BE: cache generation counter */

#define shm_read32(ofs)  ((uint32_t)SHM_BASE[(ofs)] << 24 | (uint32_t)SHM_BASE[(ofs)+1] << 16 | (uint32_t)SHM_BASE[(ofs)+2] << 8 | SHM_BASE[(ofs)+3])
#define shm_read16(ofs)  ((SHM_BASE[(ofs)] << 8) | SHM_BASE[(ofs)+1])
#define shm_read8(ofs)   (SHM_BASE[(ofs)])
#define shm_write8(ofs,v)  (SHM_BASE[(ofs)] = (uint8_t)(v))
#define shm_write16(ofs,v) do { uint16_t _v = (v); SHM_BASE[(ofs)] = (_v >> 8) & 0xFF; SHM_BASE[(ofs)+1] = _v & 0xFF; } while(0)
#define shm_write32(ofs,v) do { uint32_t _v = (v); SHM_BASE[(ofs)] = (_v >> 24) & 0xFF; SHM_BASE[(ofs)+1] = (_v >> 16) & 0xFF; SHM_BASE[(ofs)+2] = (_v >> 8) & 0xFF; SHM_BASE[(ofs)+3] = _v & 0xFF; } while(0)
#define CMD_GET_MEASUREMENT 1
#define CMD_GET_TIME        2
#define STATUS_DONE         2

#define HOOK_ADDR           ((volatile uint8_t *)0xEFFD00UL)
#define HOOK_SENSOR_READ    23
#define HOOK_RTC_READ       24

/* Shared memory command values */
#define CMD_SET_TIME 3
#define CMD_GET_GEN 4

/* Uncomment to skip IPC and always show placeholder data (e.g. testing
 * on a real Mac or without the ESP32 emulator). */
// #define USE_MOCK_DATA

/* ------------------------------------------------------------------ */
/* UI constants                                                        */
/* ------------------------------------------------------------------ */
#define appleMenuID 1
#define fileMenuID  2
#define fontMenuID  3
#define styleMenuID 4
#define timeMenuID  5

#define mAppleAbout 1
#define mFileSetTime 1
#define mFileQuit   3

/* Dialog item IDs (DITL 129) */
#define iOK     11
#define iCancel 12

#define mFontChicago 1
#define mFontGeneva  2
#define mFontMonaco  3
#define mFontNYork   4

#define mStylePlain     1
#define mStyleBold      2
#define mStyleItalic    3
#define mStyleUnderline 4
#define mStyleOutline   5
#define mStyleShadow    6
#define mTimeSeconds 1

WindowPtr    mainWindow;
MenuHandle   appleMenu, fileMenu, fontMenu, styleMenu, timeMenu;
Rect windowRect = {40, 10, 260, 320};

/* Cached sensor data */
static Boolean  s_show_seconds = false;
static float    s_temp = 0.0f;
static float    s_humi = 0.0f;
static uint8_t  s_time_bcd[7] = {0};
static Boolean  s_has_data = false;
static uint32_t s_last_gen = 0;   /* last cache gen from SHM_GEN */

/* Last drawn state — used to skip redraws when nothing changed */
static uint8_t  s_drawn_time[7];
static float    s_drawn_temp;
static float    s_drawn_humi;
static Boolean  s_drawn_valid = false;

/* Forward declarations */
static void show_time_dialog(void);

/* Dialog arrow state */
static short sb_year, sb_month, sb_day, sb_hour, sb_min;
static DialogPtr dlgPtr;

/* Current font / style */
static short s_fontID  = 3;     /* Geneva */
static short s_style   = bold;  /* initial style */

/* Offscreen buffer for flicker-free drawing */
static GrafPort s_offPort;
static BitMap  s_offBits;
static void   *s_offBuf = NULL;
static short   s_offW = 0, s_offH = 0;

static void ensure_offscreen(short w, short h)
{
    short rb = (w + 15) / 16 * 2;  /* word-aligned rowBytes for 1-bit */
    long needed = rb * h;

    if (s_offBuf != NULL && s_offW == w && s_offH == h)
        return;
    if (s_offBuf != NULL) {
        ClosePort(&s_offPort);
        DisposePtr((Ptr)s_offBuf);
    }

    s_offW = w;
    s_offH = h;
    s_offBuf = NewPtr(needed);
    s_offBits.baseAddr = (Ptr)s_offBuf;
    s_offBits.rowBytes = rb;
    SetRect(&s_offBits.bounds, 0, 0, w, h);

    OpenPort(&s_offPort);
    SetPortBits(&s_offBits);
    SetRect(&s_offPort.portRect, 0, 0, w, h);
}

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */
static void ctopstr(char *s)
{
    int len = strlen(s);
    if (len > 255) len = 255;
    memmove(s + 1, s, len);
    s[0] = len;
}

static short bcd_to_dec(uint8_t bcd)
{
    return ((bcd >> 4) & 0x0F) * 10 + (bcd & 0x0F);
}

static void pstr_to_c(Str255 ps, char *out, short maxLen)
{
    short len = ps[0];
    if (len > maxLen - 1) len = maxLen - 1;
    BlockMove(ps + 1, out, len);
    out[len] = '\0';
}

static short parse_int(Str255 ps)
{
    char buf[8];
    short i, val = 0;
    pstr_to_c(ps, buf, sizeof(buf));
    for (i = 0; buf[i] >= '0' && buf[i] <= '9'; i++)
        val = val * 10 + (buf[i] - '0');
    return val;
}

static uint8_t dec2bcd(short dec)
{
    return ((dec / 10) << 4) | (dec % 10);
}


/* ------------------------------------------------------------------ */
/* IPC: read sensor data from shared memory                            */
/* ------------------------------------------------------------------ */
/* ── IPC helpers ── */

/* Trigger a fast IPC command (no I2C) and return cache generation counter. */
static uint32_t get_cache_gen(void)
{
    shm_write8(SHM_CMD, CMD_GET_GEN);
    shm_write8(SHM_STATUS, 1);
    *HOOK_ADDR = HOOK_SENSOR_READ;
    while (shm_read8(SHM_STATUS) != STATUS_DONE) ;
    return shm_read32(SHM_GEN);
}

#ifdef USE_MOCK_DATA

#define DEC2BCD(x) ((((x) / 10) << 4) | ((x) % 10))

static void read_sensor_data(void)
{
    unsigned long t = TickCount();
    s_temp = 20.0f + (float)(t % 30);
    s_humi = (float)(t % 100);
    s_time_bcd[1] = DEC2BCD(t % 60);            /* minute */
    s_time_bcd[2] = DEC2BCD((t / 60) % 24);     /* hour */
    s_time_bcd[3] = DEC2BCD((t % 28) + 1);      /* day */
    s_time_bcd[4] = DEC2BCD((t % 12) + 1);      /* month */
    s_time_bcd[5] = 0x25;                        /* year 2025 */
    s_time_bcd[0] = 0x00;                        /* second */
    s_time_bcd[6] = t % 7;                       /* weekday */
    s_has_data = true;
}

#else /* !USE_MOCK_DATA */

#define POLL_TIMEOUT_TICKS 180

static void read_sensor_data(void)
{
    unsigned long deadline = TickCount() + POLL_TIMEOUT_TICKS;

    shm_write8(SHM_CMD, CMD_GET_MEASUREMENT);
    shm_write8(SHM_STATUS, 1);
    *HOOK_ADDR = HOOK_SENSOR_READ;

    while (shm_read8(SHM_STATUS) != STATUS_DONE) {
        if (TickCount() > deadline) goto use_mock;
    }

    int16_t temp_raw, humi_raw;
    temp_raw = shm_read16(SHM_TEMP);
    humi_raw = shm_read16(SHM_HUMI);
    s_temp = (float)temp_raw / 10.0f;
    s_humi = (float)humi_raw / 10.0f;

    deadline = TickCount() + POLL_TIMEOUT_TICKS;
    shm_write8(SHM_CMD, CMD_GET_TIME);
    shm_write8(SHM_STATUS, 1);
    *HOOK_ADDR = HOOK_RTC_READ;

    while (shm_read8(SHM_STATUS) != STATUS_DONE) {
        if (TickCount() > deadline) goto use_mock;
    }

    s_time_bcd[0] = shm_read8(SHM_TIME);
    s_time_bcd[1] = shm_read8(SHM_TIME + 1);
    s_time_bcd[2] = shm_read8(SHM_TIME + 2);
    s_time_bcd[3] = shm_read8(SHM_TIME + 3);
    s_time_bcd[4] = shm_read8(SHM_TIME + 4);
    s_time_bcd[5] = shm_read8(SHM_TIME + 5);
    s_time_bcd[6] = shm_read8(SHM_TIME + 6);
    s_has_data = true;
    return;

use_mock:
    s_temp = 88.0f;
    s_humi = 0.0f;
    s_time_bcd[0] = 0x00; s_time_bcd[1] = 0x00; s_time_bcd[2] = 0x00;
    s_time_bcd[3] = 0x01; s_time_bcd[4] = 0x01; s_time_bcd[5] = 0x25;
    s_time_bcd[6] = 0x00;
    s_has_data = true;
}

#endif /* USE_MOCK_DATA */

/* ------------------------------------------------------------------ */
/* Drawing                                                             */
/* ------------------------------------------------------------------ */
static void draw_content(void)
{
    char buf[64];
    int cx;
    GrafPtr savePort;
    Rect srcRect, dstRect;

    /* Ensure offscreen buffer matches window size */
    SetPort(mainWindow);
    ensure_offscreen(mainWindow->portRect.right, mainWindow->portRect.bottom);

    /* ── Draw into offscreen buffer ── */
    GetPort(&savePort);
    SetPort(&s_offPort);
    EraseRect(&s_offPort.portRect);

    cx = s_offPort.portRect.right / 2;

    if (!s_has_data) {
        MoveTo(cx - 80, 50);
        TextSize(12);
        DrawString("\pNo data");
        goto blast;
    }

    /* ── Time: biggest, top center ── */
    if (s_show_seconds) {
        sprintf(buf, "%02X:%02X:%02X",
                s_time_bcd[2], s_time_bcd[1], s_time_bcd[0]);
    } else {
        sprintf(buf, "%02X:%02X",
                s_time_bcd[2], s_time_bcd[1]);
    }
    ctopstr(buf);
    TextFont(s_fontID);
    TextSize(s_show_seconds ? 60 : 84);
    TextFace(s_style);
    {
        short tw = StringWidth((StringPtr)buf);
        short tx = cx - tw / 2;
        MoveTo(tx, 85);
        DrawString((StringPtr)buf);

        /* ── Date: centered ── */
        sprintf(buf, "20%02X/%02X/%02X",
                s_time_bcd[5], s_time_bcd[4], s_time_bcd[3]);
        ctopstr(buf);
        TextSize(16);
        MoveTo(cx - StringWidth((StringPtr)buf) / 2, 125);
        DrawString((StringPtr)buf);

        /* ── Separator line ── */
        MoveTo(20, 140);
        LineTo(cx * 2 - 20, 140);

        /* ── Temperature + Humidity: match time left/right ── */

        /* Temperature */
        {
            int t = (int)(s_temp * 10.0f + 0.05f);
            sprintf(buf, "%d.%d", t / 10, abs(t) % 10);
        }
        ctopstr(buf);
        MoveTo(tx, 195);
        TextSize(40);
        DrawString((StringPtr)buf);
        TextSize(16);
        DrawString("\p C");

        /* Humidity — right edge aligns with time right edge */
        {
            int h = (int)(s_humi * 10.0f + 0.05f);
            sprintf(buf, "%d.%d", h / 10, abs(h) % 10);
        }
        ctopstr(buf);
        TextSize(40);
        short humi_num_w = StringWidth((StringPtr)buf);
        TextSize(16);
        short humi_unit_w = StringWidth("\p%");
        short humi_total = humi_num_w + humi_unit_w;
        MoveTo(tx + tw - humi_total, 195);
        TextSize(40);
        DrawString((StringPtr)buf);
        TextSize(16);
        DrawString("\p%");
    }
    TextFace(0);

    /* ── Blast offscreen → screen ── */
blast:
    SetPort(savePort);
    srcRect = s_offPort.portRect;
    dstRect = mainWindow->portRect;
    CopyBits(&s_offBits, &mainWindow->portBits, &srcRect, &dstRect, srcCopy, NULL);

    /* Remember what we just drew so we can skip redundant redraws */
    memcpy(s_drawn_time, s_time_bcd, 7);
    s_drawn_temp = s_temp;
    s_drawn_humi = s_humi;
    s_drawn_valid = true;
}

/* ------------------------------------------------------------------ */
/* Event handling                                                      */
/* ------------------------------------------------------------------ */
static void handle_menu(long menuChoice)
{
    int menuID = HiWord(menuChoice);
    int itemID = LoWord(menuChoice);
    Str255 daName;

    switch (menuID) {
    case appleMenuID:
        if (itemID == mAppleAbout)
            Alert(128, NULL);
        else if (itemID > 1) {
            GetMenuItemText(appleMenu, itemID, daName);
            OpenDeskAcc(daName);
        }
        HiliteMenu(0);
        break;
    case fileMenuID:
        if (itemID == mFileSetTime) {
            show_time_dialog();
        } else if (itemID == mFileQuit) {
            ExitToShell();
        }
        HiliteMenu(0);
        break;
    case fontMenuID: {
        static const short kFontIDs[] = {0, 3, 4, 2};
        if (itemID >= 1 && itemID <= 4) {
            /* Clear previous checkmark */
            short prev;
            for (prev = 1; prev <= 4; prev++)
                SetItemMark(fontMenu, prev, noMark);
            SetItemMark(fontMenu, itemID, checkMark);
            s_fontID = kFontIDs[itemID - 1];
            InvalRect(&mainWindow->portRect);
        }
        HiliteMenu(0);
        break;
    }
    case styleMenuID:
        if (itemID == mStylePlain) {
            s_style = 0;
            short i;
            for (i = 1; i <= 6; i++)
                SetItemMark(styleMenu, i, noMark);
            SetItemMark(styleMenu, mStylePlain, checkMark);
        } else {
            /* Toggle the style bit */
            static const short kStyleBits[] = {0, 1, 2, 4, 8, 16};
            short bit = kStyleBits[itemID - 1];
            s_style ^= bit;
            SetItemMark(styleMenu, itemID,
                (s_style & bit) ? checkMark : noMark);
            /* If any style set, clear Plain check; if none, check Plain */
            short i;
            Boolean any = false;
            for (i = 2; i <= 6; i++) {
                if (s_style & kStyleBits[i - 1]) any = true;
            }
            SetItemMark(styleMenu, mStylePlain, any ? noMark : checkMark);
        }
        InvalRect(&mainWindow->portRect);
        HiliteMenu(0);
        break;
    case timeMenuID:
        if (itemID == mTimeSeconds) {
            s_show_seconds = !s_show_seconds;
            SetItemMark(timeMenu, mTimeSeconds,
                s_show_seconds ? checkMark : noMark);
            InvalRect(&mainWindow->portRect);
        }
        HiliteMenu(0);
        break;
    }
}

/* ------------------------------------------------------------------ */
/* Setup                                                               */
/* ------------------------------------------------------------------ */
static void init_toolbox(void)
{
    InitGraf(&qd.thePort);
    InitFonts();
    InitWindows();
    InitMenus();
    TEInit();
    InitDialogs(NULL);
    InitCursor();
}

/* ── Date/Time setting dialog ── */

/* Push button IDs for up/down arrows */
#define SB_YEAR_UP    100
#define SB_YEAR_DN    101
#define SB_MONTH_UP   102
#define SB_MONTH_DN   103
#define SB_DAY_UP     104
#define SB_DAY_DN     105
#define SB_HOUR_UP    106
#define SB_HOUR_DN    107
#define SB_MINUTE_UP  108
#define SB_MINUTE_DN  109

/* DITL item numbers for edit fields */
#define ITEM_YEAR    2
#define ITEM_MONTH   3
#define ITEM_DAY     4
#define ITEM_HOUR    6
#define ITEM_MINUTE  7
#define ITEM_OK      8
#define ITEM_CANCEL  9

static void do_arrow(short refCon)
{
    short ditlItem, *pVal, min, max;
    short cur;

    /* Map button refCon to field info (val, min, max, ditlItem) */
    switch (refCon) {
    case SB_YEAR_UP:
    case SB_YEAR_DN:   pVal = &sb_year;  min = 2000; max = 2099; ditlItem = ITEM_YEAR; break;
    case SB_MONTH_UP:
    case SB_MONTH_DN:  pVal = &sb_month; min = 1;    max = 12;   ditlItem = ITEM_MONTH; break;
    case SB_DAY_UP:
    case SB_DAY_DN:    pVal = &sb_day;   min = 1;    max = 31;   ditlItem = ITEM_DAY; break;
    case SB_HOUR_UP:
    case SB_HOUR_DN:   pVal = &sb_hour;  min = 0;    max = 23;   ditlItem = ITEM_HOUR; break;
    case SB_MINUTE_UP:
    case SB_MINUTE_DN: pVal = &sb_min;   min = 0;    max = 59;   ditlItem = ITEM_MINUTE; break;
    default: return;
    }

    cur = *pVal;
    if (refCon == SB_YEAR_UP || refCon == SB_MONTH_UP ||
        refCon == SB_DAY_UP || refCon == SB_HOUR_UP || refCon == SB_MINUTE_UP) {
        if (cur < max) cur++; else cur = min;
    } else {
        if (cur > min) cur--; else cur = max;
    }
    *pVal = cur;

    /* Update edit text */
    {
        Handle itemH;
        short type;
        Rect r;
        char buf[8];
        if (ditlItem == ITEM_YEAR) sprintf(buf, "%04d", cur);
        else                       sprintf(buf, "%02d", cur);
        ctopstr(buf);
        GetDialogItem(dlgPtr, ditlItem, &type, &itemH, &r);
        SetDialogItemText(itemH, (StringPtr)buf);
    }
}

static pascal Boolean myFilter(DialogPtr dlg, EventRecord *evt, short *itemHit)
{
    if (evt->what != mouseDown)
        return false;

    Point pt = evt->where;
    GlobalToLocal(&pt);
    ControlHandle ctl;
    short part = FindControl(pt, (WindowPtr)dlg, &ctl);
    if (part == 0 || part != inButton)
        return false;

    if (TrackControl(ctl, pt, NULL)) {
        short ref = GetControlReference(ctl);
        if (ref >= SB_YEAR_UP && ref <= SB_MINUTE_DN) {
            do_arrow(ref);
            return true;
        }
    }
    return false;
}

static void show_time_dialog(void)
{
    DialogPtr dlg;
    short itemHit;
    Handle itemH;
    short itemType;
    Rect itemRect;
    Str255 s;

    dlg = GetNewDialog(129, NULL, (WindowPtr)-1);
    if (dlg == NULL) return;

    /* Center on screen & pre-fill */
    {
        Rect r = dlg->portRect;
        int cx = qd.screenBits.bounds.right / 2;
        int cy = qd.screenBits.bounds.bottom / 2;
        MoveWindow((WindowPtr)dlg, cx - (r.right - r.left) / 2,
                   cy - (r.bottom - r.top) / 2, false);
    }

    /* Pre-fill edit text fields */
    {
        char buf[8];
        sprintf(buf, "%04d", 2000 + bcd_to_dec(s_time_bcd[5]));
        ctopstr(buf);
        GetDialogItem(dlg, ITEM_YEAR, &itemType, &itemH, &itemRect);
        SetDialogItemText(itemH, (StringPtr)buf);

        sprintf(buf, "%02d", bcd_to_dec(s_time_bcd[4]));
        ctopstr(buf);
        GetDialogItem(dlg, ITEM_MONTH, &itemType, &itemH, &itemRect);
        SetDialogItemText(itemH, (StringPtr)buf);

        sprintf(buf, "%02d", bcd_to_dec(s_time_bcd[3]));
        ctopstr(buf);
        GetDialogItem(dlg, ITEM_DAY, &itemType, &itemH, &itemRect);
        SetDialogItemText(itemH, (StringPtr)buf);

        sprintf(buf, "%02d", bcd_to_dec(s_time_bcd[2]));
        ctopstr(buf);
        GetDialogItem(dlg, ITEM_HOUR, &itemType, &itemH, &itemRect);
        SetDialogItemText(itemH, (StringPtr)buf);

        sprintf(buf, "%02d", bcd_to_dec(s_time_bcd[1]));
        ctopstr(buf);
        GetDialogItem(dlg, ITEM_MINUTE, &itemType, &itemH, &itemRect);
        SetDialogItemText(itemH, (StringPtr)buf);
    }

    /* Init arrow values */
    sb_year  = 2000 + bcd_to_dec(s_time_bcd[5]);
    sb_month = bcd_to_dec(s_time_bcd[4]);
    sb_day   = bcd_to_dec(s_time_bcd[3]);
    sb_hour  = bcd_to_dec(s_time_bcd[2]);
    sb_min   = bcd_to_dec(s_time_bcd[1]);
    dlgPtr = dlg;

    /* Create ▲/▼ push buttons next to each edit field */
    {
        short i;
        struct { short ditl; short idUp; short idDn; } btns[] = {
            {ITEM_YEAR,   SB_YEAR_UP,   SB_YEAR_DN},
            {ITEM_MONTH,  SB_MONTH_UP,  SB_MONTH_DN},
            {ITEM_DAY,    SB_DAY_UP,    SB_DAY_DN},
            {ITEM_HOUR,   SB_HOUR_UP,   SB_HOUR_DN},
            {ITEM_MINUTE, SB_MINUTE_UP, SB_MINUTE_DN},
        };
        for (i = 0; i < 5; i++) {
            Rect r;
            GetDialogItem(dlg, btns[i].ditl, &itemType, &itemH, &r);
            short btnL = r.right + 2;
            short btnR = btnL + 12;
            short cY = (r.top + r.bottom) / 2;  /* field center */

            /* Stacked 10px buttons centered on the field */
            Rect rUp = {cY - 10, btnL, cY, btnR};
            NewControl(dlg, &rUp, "\p+", true, 0, 0, 1, pushButProc, btns[i].idUp);
            Rect rDn = {cY, btnL, cY + 10, btnR};
            NewControl(dlg, &rDn, "\p-", true, 0, 0, 1, pushButProc, btns[i].idDn);
        }
    }

    ShowWindow((WindowPtr)dlg);
    SetPort((GrafPtr)dlg);

    /* Modal loop with filter proc for scroll bars */
    while (1) {
        ModalDialog((ModalFilterProcPtr)myFilter, &itemHit);
        if (itemHit == ITEM_OK || itemHit == ITEM_CANCEL)
            break;
    }

    if (itemHit == ITEM_OK) {
        short yr, mo, dy, hr, mn;

        GetDialogItem(dlg, ITEM_YEAR, &itemType, &itemH, &itemRect);
        GetDialogItemText(itemH, s);
        yr = parse_int(s);
        GetDialogItem(dlg, ITEM_MONTH, &itemType, &itemH, &itemRect);
        GetDialogItemText(itemH, s);
        mo = parse_int(s);
        GetDialogItem(dlg, ITEM_DAY, &itemType, &itemH, &itemRect);
        GetDialogItemText(itemH, s);
        dy = parse_int(s);
        GetDialogItem(dlg, ITEM_HOUR, &itemType, &itemH, &itemRect);
        GetDialogItemText(itemH, s);
        hr = parse_int(s);
        GetDialogItem(dlg, ITEM_MINUTE, &itemType, &itemH, &itemRect);
        GetDialogItemText(itemH, s);
        mn = parse_int(s);

        if (yr >= 2000 && mo >= 1 && mo <= 12 && dy >= 1 && dy <= 31 &&
            hr >= 0 && hr <= 23 && mn >= 0 && mn <= 59) {
#ifndef USE_MOCK_DATA
            /* Write command + BCD time to shared memory */
            shm_write8(SHM_CMD, CMD_SET_TIME);
            shm_write8(SHM_TIME + 0, 0);
            shm_write8(SHM_TIME + 1, dec2bcd(mn));
            shm_write8(SHM_TIME + 2, dec2bcd(hr));
            shm_write8(SHM_TIME + 3, dec2bcd(dy));
            shm_write8(SHM_TIME + 4, dec2bcd(mo));
            shm_write8(SHM_TIME + 5, dec2bcd(yr % 100));
            shm_write8(SHM_TIME + 6, 0);
            shm_write8(SHM_STATUS, 1);
            *HOOK_ADDR = HOOK_SENSOR_READ;         /* trigger emulator */

            /* Wait for completion */
            while (shm_read8(SHM_STATUS) != STATUS_DONE) ;
#endif /* !USE_MOCK_DATA */
        }
    }

    DisposeDialog(dlg);
}
static void setup_menus(void)
{
    Handle mbar = GetNewMBar(128);
    if (mbar) {
        SetMenuBar(mbar);
        AppendResMenu(GetMenuHandle(appleMenuID), 'DRVR');
        appleMenu = GetMenuHandle(appleMenuID);
        fileMenu  = GetMenuHandle(fileMenuID);
        fontMenu  = GetMenuHandle(fontMenuID);
        styleMenu = GetMenuHandle(styleMenuID);
        timeMenu  = GetMenuHandle(timeMenuID);
    } else {
        appleMenu = NewMenu(appleMenuID, "\p\x14");
        AppendMenu(appleMenu, "\pAbout Dash");
        InsertMenu(appleMenu, 0);
        fileMenu = NewMenu(fileMenuID, "\pFile");
        AppendMenu(fileMenu, "\pSet Date/Time.../-T;(-;Quit/Q");
        InsertMenu(fileMenu, 0);
        InsertMenu(fontMenu, 0);
        styleMenu = NewMenu(styleMenuID, "\pStyle");
        AppendMenu(styleMenu, "\pPlain;Bold;Italic;Underline;Outline;Shadow");
        InsertMenu(styleMenu, 0);
        timeMenu  = NewMenu(timeMenuID, "\pTime");
        AppendMenu(timeMenu, "\pShow Seconds/S");
        InsertMenu(timeMenu, 0);
    }
    SetItemMark(fontMenu, mFontGeneva, checkMark);
    SetItemMark(styleMenu, mStyleBold, checkMark);

    /* Preview: style items show their own style */
    SetItemStyle(styleMenu, mStyleBold,     bold);
    SetItemStyle(styleMenu, mStyleItalic,   italic);
    SetItemStyle(styleMenu, mStyleUnderline, underline);
    SetItemStyle(styleMenu, mStyleOutline,  outline);
    SetItemStyle(styleMenu, mStyleShadow,   shadow);

    /* Time menu initial state — seconds off */
    SetItemMark(timeMenu, mTimeSeconds, noMark);

    DrawMenuBar();
}

/* ------------------------------------------------------------------ */
/* Main loop                                                           */
/* ------------------------------------------------------------------ */
static void main_loop(void)
{
    EventRecord event;
    WindowPtr window;

    while (1) {
        SystemTask();

        /* Refresh when cache generation changes, but only redraw
         * when the *displayed* fields actually differ from last time.
         * When seconds are hidden we skip redraws on second-only changes. */
        {
            uint32_t gen = get_cache_gen();
            if (gen != s_last_gen) {
                read_sensor_data();
                s_last_gen = gen;

                /* Decide: did any field that is currently visible change? */
                Boolean need_redraw = !s_drawn_valid;
                if (!need_redraw && (s_drawn_temp != s_temp || s_drawn_humi != s_humi))
                    need_redraw = true;
                if (!need_redraw) {
                    if (s_show_seconds) {
                        /* any time byte change → redraw (includes seconds) */
                        need_redraw = (memcmp(s_drawn_time, s_time_bcd, 7) != 0);
                    } else {
                        /* only hh:mm + date matter; ignore seconds (byte 0) */
                        need_redraw = (s_drawn_time[1] != s_time_bcd[1] ||
                                       s_drawn_time[2] != s_time_bcd[2] ||
                                       s_drawn_time[3] != s_time_bcd[3] ||
                                       s_drawn_time[4] != s_time_bcd[4] ||
                                       s_drawn_time[5] != s_time_bcd[5]);
                    }
                }

                if (need_redraw) {
                    SetPort(mainWindow);
                    draw_content();  /* updates s_drawn_* inside */
                }
            }
        }

        if (!GetNextEvent(everyEvent, &event)) continue;

        switch (event.what) {
        case mouseDown:
            switch (FindWindow(event.where, &window)) {
            case inMenuBar:
                handle_menu(MenuSelect(event.where));
                break;
            case inSysWindow:
                SystemClick(&event, window);
                break;
            case inDrag:
                DragWindow(window, event.where, &qd.screenBits.bounds);
                break;
            case inGoAway:
                if (TrackGoAway(window, event.where) && window == mainWindow)
                    ExitToShell();
                break;
            case inContent:
                if (window == mainWindow)
                    SelectWindow(window);
                else
                    SelectWindow(window);
                break;
            }
            break;
        case keyDown:
        case autoKey:
            if (event.modifiers & cmdKey)
                handle_menu(MenuKey(event.message & charCodeMask));
            break;
        case updateEvt:
            window = (WindowPtr)event.message;
            BeginUpdate(window);
            if (window == mainWindow) draw_content();
            EndUpdate(window);
            break;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Entry point                                                         */
/* ------------------------------------------------------------------ */
void main(void)
{
    init_toolbox();
    setup_menus();

    /* Center the window before creating it */
    {
        short sx = qd.screenBits.bounds.right;
        short sy = qd.screenBits.bounds.bottom;
        short w = windowRect.right - windowRect.left;
        short h = windowRect.bottom - windowRect.top;
        windowRect.left = (sx - w) / 2;
        windowRect.top  = (sy - h) / 2;
        windowRect.right = windowRect.left + w;
        windowRect.bottom = windowRect.top + h;
    }
    mainWindow = NewWindow(NULL, &windowRect, "\pDash", true,
                           documentProc, (WindowPtr)-1, true, 0);
    SetPort(mainWindow);

    /* Initial read */
    read_sensor_data();
    draw_content();

    main_loop();
}
