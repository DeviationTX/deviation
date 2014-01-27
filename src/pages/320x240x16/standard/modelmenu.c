/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Deviation is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Deviation.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "common.h"
#include "../pages.h"
#include "gui/gui.h"

#if HAS_STANDARD_GUI

static struct main_page    * const mp  = &pagemem.u.main_page;

static struct stdmenu_obj * const gui = &gui_objs.u.stdmenu;
static unsigned _action_cb(u32 button, unsigned flags, void *data);

struct menu_s {
    u8 id;
    struct ImageMap icon;
};
static const struct menu_s menus[] = {
    {PAGEID_MODEL,        {"media/mnumodel.bmp", 48, 47, 0, 0}},
    {PAGEID_REVERSE,      {"media/mnurvrse.bmp", 48, 47, 0, 0}},
    {PAGEID_SUBTRIM,      {"media/mnusubtr.bmp", 48, 47, 0, 0}},
    {PAGEID_TRAVELADJ,    {"media/mnutradj.bmp", 48, 47, 0, 0}},
    {PAGEID_SWASH,        {"media/mnuswash.bmp", 48, 47, 0, 0}},
    {PAGEID_DREXP,        {"media/mnuexpdr.bmp", 48, 47, 0, 0}},
    {PAGEID_THROCURVES,   {"media/mnuthrcv.bmp", 48, 47, 0, 0}},
    {PAGEID_PITCURVES,    {"media/mnupitcv.bmp", 48, 47, 0, 0}},
    {PAGEID_GYROSENSE,    {"media/mnugyro.bmp",  48, 47, 0, 0}},
    {PAGEID_TRIM,         {"media/mnutrim.bmp",  48, 47, 0, 0}},
    {PAGEID_SWITCHASSIGN, {"media/mnuswtch.bmp", 48, 47, 0, 0}},
    {PAGEID_THROHOLD,     {"media/mnuthrhd.bmp", 48, 47, 0, 0}},
    {PAGEID_FAILSAFE,     {"media/mnufail.bmp",  48, 47, 0, 0}},
    {PAGEID_TIMER,        {"media/mnutimer.bmp", 48, 47, 0, 0}},
    {PAGEID_TELEMCFG,     {"media/mnutelem.bmp", 48, 47, 0, 0}},
    {PAGEID_DATALOG,      {"media/mnulog.bmp",   48, 47, 0, 0}},
    {PAGEID_MAINCFG,      {"media/mnucfg.bmp",   48, 47, 0, 0}},
};

static void ico_select_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u16 pos = (long)data;
    PAGE_ChangeByID(menus[pos].id);
}

static void goto_mainpage(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    PAGE_ChangeByID(PAGEID_MAIN);
}

#define NUM_ROWS 4
#define NUM_COLS 5
void PAGE_ModelMenuInit(int page)
{
    (void) page;
    long pos = 0;
    PAGE_SetModal(0);
    PAGE_ShowHeader_ExitOnly(PAGE_GetName(PAGEID_MODELMENU), goto_mainpage);
    BUTTON_RegisterCallback(&mp->action,
          CHAN_ButtonMask(BUT_LEFT)
          | CHAN_ButtonMask(BUT_RIGHT)
          | CHAN_ButtonMask(BUT_UP)
          | CHAN_ButtonMask(BUT_DOWN),
          BUTTON_PRESS | BUTTON_PRIORITY, _action_cb, NULL);
    u8 count = sizeof(menus) / sizeof(struct menu_s);
    for(int j = 0; j < NUM_ROWS; j++) {
        int y = 40 + j * (LCD_HEIGHT == 240 ? 50 : 59);
        for(int i = 0; i < NUM_COLS; i++,pos++) {
            if (pos >= count)
                break;
            int x = (LCD_WIDTH == 320 ? 12 : 92) + i*60;
            GUI_CreateIcon(&gui->icon[pos], x, y, &menus[pos].icon, ico_select_cb, (void *)pos);
        }
    }
}

void PAGE_ModelMenuExit()
{
    BUTTON_UnregisterCallback(&mp->action);
}

void MODELMENU_Show(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    PAGE_ChangeByID(PAGEID_MODELMENU);
}

static unsigned _action_cb(u32 button, unsigned flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_LONGPRESS)
         || CHAN_ButtonIsPressed(button, BUT_ENTER)
         || CHAN_ButtonIsPressed(button, BUT_EXIT))
    {
        return 0;
    }
    if (! GUI_GetSelected()) {
        GUI_SetSelected((guiObject_t *)&gui->icon[0]);
        return 1;
    }
    int incr = 0;
    if (CHAN_ButtonIsPressed(button, BUT_LEFT))
        incr = -1;
    else if (CHAN_ButtonIsPressed(button, BUT_RIGHT))
        incr = 1;
    else if (CHAN_ButtonIsPressed(button, BUT_UP))
        incr = -NUM_COLS;
    else if (CHAN_ButtonIsPressed(button, BUT_DOWN))
        incr = NUM_COLS;
    int pos = (((long)GUI_GetSelected()) - (long)(&gui->icon[0])) / sizeof(gui->icon[0]);
    int count = sizeof(menus) / sizeof(struct menu_s);
    if (pos < 0)
        return 0;
    pos = (pos + (NUM_COLS * NUM_ROWS) + incr) % (NUM_COLS * NUM_ROWS);
    if (pos  >= count) {
        if (incr == -1)
            pos = count -1;
        else if (incr == -NUM_COLS)
            pos -= NUM_COLS;
        else if (incr == 1)
            pos = 0;
        else if (incr == NUM_COLS)
             pos = (pos + incr) % (NUM_COLS * NUM_ROWS);
    }
    GUI_SetSelected((guiObject_t *)&gui->icon[pos]);
    return 1;
}
#endif //HAS_STANDARD_GUI
