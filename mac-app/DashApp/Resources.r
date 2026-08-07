#include "Types.r"
#include "Dialogs.r"
#include "Menus.r"
#include "Processes.r"

resource 'DITL' (128) {
    {
        { 70, 20, 90, 140 },
        Button { enabled, "OK" };
        { 10, 20, 26, 200 },
        StaticText { disabled, "Dash" };
        { 30, 75, 42, 145 },
        StaticText { disabled, "v1.0" };
        { 48, 55, 60, 165 },
        StaticText { disabled, "Retro68" };
    }
};

resource 'ALRT' (128) {
    { 80, 40, 190, 210 },
    128,
    { OK, visible, sound1, OK, visible, sound1, OK, visible, sound1, OK, visible, sound1 },
    noAutoCenter
};

resource 'MENU' (1) {
    1, textMenuProc;
    allEnabled, enabled;
    apple;
    { "About Dash", noIcon, noKey, noMark, plain; }
};

resource 'MENU' (2) {
    2, textMenuProc;
    allEnabled, enabled;
    "File";
    { "Set Date/Time...", noIcon, "T", noMark, plain;
      "-", noIcon, noKey, noMark, plain;
      "Quit", noIcon, "Q", noMark, plain; }
};

resource 'MENU' (3) {
    3, textMenuProc;
    allEnabled, enabled;
    "Font";
    { "Chicago", noIcon, noKey, noMark, plain;
      "Geneva", noIcon, noKey, noMark, plain;
      "Monaco", noIcon, noKey, noMark, plain;
      "New York", noIcon, noKey, noMark, plain; }
};

resource 'MENU' (4) {
    4, textMenuProc;
    allEnabled, enabled;
    "Style";
    { "Plain", noIcon, noKey, noMark, plain;
      "Bold", noIcon, noKey, noMark, plain;
      "Italic", noIcon, noKey, noMark, plain;
      "Underline", noIcon, noKey, noMark, plain;
      "Outline", noIcon, noKey, noMark, plain;
      "Shadow", noIcon, noKey, noMark, plain; }
};

resource 'MENU' (5) {
    5, textMenuProc;
    allEnabled, enabled;
    "Time";
    { "Show Seconds", noIcon, "S", noMark, plain; }
};

resource 'MBAR' (128) {
    { 1, 2, 3, 4, 5 }
};

resource 'DLOG' (129) {
    {90, 15, 210, 295},
    dBoxProc,
    invisible,
    noGoAway,
    0,
    129,
    "Set Time",
    alertPositionMainScreen
};

resource 'DITL' (129) {
    {
        /* Row 1: Date */
        { 10, 10, 24, 44 },
        StaticText { disabled, "Date:" };
        { 10, 48, 24, 84 },
        EditText { enabled, "" };        /* year 36px */
        { 10, 112, 24, 132 },
        EditText { enabled, "" };        /* month 20px */
        { 10, 160, 24, 180 },
        EditText { enabled, "" };        /* day 20px */
        /* Row 2: Time */
        { 36, 10, 50, 44 },
        StaticText { disabled, "Time:" };
        { 36, 48, 50, 68 },
        EditText { enabled, "" };        /* hour 20px */
        { 36, 96, 50, 116 },
        EditText { enabled, "" };        /* minute 20px */
        /* Row 3: Buttons at bottom */
        { 85, 35, 101, 90 },
        Button { enabled, "OK" };
        { 85, 105, 101, 160 },
        Button { enabled, "Cancel" };
    }
};

resource 'SIZE' (-1) {
    reserved,
    acceptSuspendResumeEvents,
    reserved,
    canBackground,
    doesActivateOnFGSwitch,
    backgroundAndForeground,
    dontGetFrontClicks,
    ignoreChildDiedEvents,
    is32BitCompatible,
    notHighLevelEventAware,
    onlyLocalHLEvents,
    notStationeryAware,
    dontUseTextEditServices,
    reserved,
    reserved,
    reserved,
    256 * 1024,
    128 * 1024
};
