#include <Dialogs.h>
#include <Events.h>
#include <Fonts.h>
#include <Menus.h>
#include <Quickdraw.h>
#include <TextEdit.h>
#include <Windows.h>
#include <stdio.h>
#include <string.h>

#define appleMenuID 1
#define fileMenuID  2
#define mAppleAbout 1
#define mFileQuit   3

#define ID_BTN_INC   1
#define ID_BTN_RESET 2

WindowPtr    mainWindow;
MenuHandle   appleMenu, fileMenu;
ControlHandle btnInc, btnReset;
Rect windowRect = {40, 10, 200, 240};

static int counter = 0;

static void ctopstr(char *s)
{
    int len = strlen(s);
    if (len > 255) len = 255;
    memmove(s + 1, s, len);
    s[0] = len;
}

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

static void draw_content(void)
{
    char buf[32];

    SetPort(mainWindow);
    EraseRect(&mainWindow->portRect);

    /* 标题 */
    MoveTo(20, 30);
    TextSize(14);
    TextFace(bold);
    DrawString("\pCounter App");
    TextFace(0);

    /* 计数标签 */
    sprintf(buf, "Count: %d", counter);
    ctopstr(buf);
    MoveTo(20, 70);
    TextSize(48);
    DrawString((StringPtr)buf);

    DrawControls(mainWindow);
}

static void handle_control_click(Point localPt)
{
    ControlHandle ctrl;
    short part = FindControl(localPt, mainWindow, &ctrl);
    if (part != inButton) return;
    if (!TrackControl(ctrl, localPt, NULL)) return;

    if      (ctrl == btnInc)   counter++;
    else if (ctrl == btnReset) counter = 0;

    InvalRect(&mainWindow->portRect);
}

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
        if (itemID == mFileQuit) ExitToShell();
        HiliteMenu(0);
        break;
    }
}

static void setup_menus(void)
{
    Handle mbar = GetNewMBar(128);
    if (mbar) {
        SetMenuBar(mbar);
        AppendResMenu(GetMenuHandle(appleMenuID), 'DRVR');
        appleMenu = GetMenuHandle(appleMenuID);
        fileMenu  = GetMenuHandle(fileMenuID);
    } else {
        appleMenu = NewMenu(appleMenuID, "\p\x14");
        AppendMenu(appleMenu, "\pAbout Counter");
        InsertMenu(appleMenu, 0);
        fileMenu = NewMenu(fileMenuID, "\pFile");
        AppendMenu(fileMenu, "\pQuit");
        InsertMenu(fileMenu, 0);
    }
    DrawMenuBar();
}

static void create_controls(void)
{
    Rect r;

    r.top = 100; r.bottom = 125;
    r.left = 20; r.right = 100;
    btnInc = NewControl(mainWindow, &r, "\p+1", true, 0, 0, 1, pushButProc, ID_BTN_INC);

    r.top = 100; r.bottom = 125;
    r.left = 110; r.right = 200;
    btnReset = NewControl(mainWindow, &r, "\pReset", true, 0, 0, 1, pushButProc, ID_BTN_RESET);
}

static void main_loop(void)
{
    EventRecord event;
    WindowPtr window;
    Point localPt;

    while (1) {
        SystemTask();
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
                if (window == mainWindow) {
                    localPt = event.where;
                    GlobalToLocal(&localPt);
                    handle_control_click(localPt);
                } else {
                    SelectWindow(window);
                }
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

void main(void)
{
    init_toolbox();
    setup_menus();

    mainWindow = NewWindow(NULL, &windowRect, "\pCounter", true,
                           documentProc, (WindowPtr)-1, true, 0);
    SetPort(mainWindow);
    create_controls();
    draw_content();
    main_loop();
}
