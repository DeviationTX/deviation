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

#define gui (&gui_objs.u.stdmenu)

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

void PAGE_ModelMenuInit(int page)
{
    (void) page;
    long pos = 0;
    PAGE_SetModal(0);
    PAGE_ShowHeader_ExitOnly(PAGE_GetName(PAGEID_MODELMENU), goto_mainpage);
    u8 count = sizeof(menus) / sizeof(struct menu_s);
    for(int j = 0; j < 4; j++) {
        int y = 40 + j * (LCD_HEIGHT == 240 ? 50 : 59);
        for(int i = 0; i < 5; i++,pos++) {
            if (pos >= count)
                break;
            int x = (LCD_WIDTH == 320 ? 12 : 92) + i*60;
            GUI_CreateIcon(&gui->icon[pos], x, y, &menus[pos].icon, ico_select_cb, (void *)pos);
        }
    }
}

void MODELMENU_Show(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    PAGE_ChangeByID(PAGEID_MODELMENU);
}

