#include "Types.r"
#include "Dialogs.r"
#include "Menus.r"
#include "Processes.r"

resource 'DITL' (128) {
    {
        { 70, 20, 90, 140 },
        Button { enabled, "OK" };
        { 10, 20, 26, 200 },
        StaticText { disabled, "Counter App" };
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
    { "About Counter", noIcon, noKey, noMark, plain; }
};

resource 'MENU' (2) {
    2, textMenuProc;
    allEnabled, enabled;
    "File";
    { "Quit", noIcon, "Q", noMark, plain; }
};

resource 'MBAR' (128) {
    { 1, 2 }
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
