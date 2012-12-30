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

struct menu_s {
    u8 id;
    const char file[20];
};
static const struct menu_s menus[] = {
    {PAGEID_MODEL,        "media/mnumodel.bmp"},
    {PAGEID_REVERSE,      "media/mnurvrse.bmp"},
    {PAGEID_DREXP,        "media/mnuexpdr.bmp"},
    {PAGEID_SUBTRIM,      "media/mnusubtr.bmp"},
    {PAGEID_TRAVELADJ,    "media/mnutradj.bmp"},
    {PAGEID_THROCURVES,   "media/mnuthrcv.bmp"},
    {PAGEID_PITCURVES,    "media/mnupitcv.bmp"},
    {PAGEID_THROHOLD,     "media/mnuthrhd.bmp"},
    {PAGEID_GYROSENSE,    "media/mnugyro.bmp"},
    {PAGEID_SWASH,        "media/mnuswash.bmp"},
    {PAGEID_FAILSAFE,     "media/mnufail.bmp"},
    {PAGEID_SWITCHASSIGN, "media/mnuswtch.bmp"},
    {PAGEID_TIMER,        "media/mnutimer.bmp"},
    {PAGEID_TELEMCFG,     "media/mnutelem.bmp"},
    {PAGEID_TRIM,         "media/mnutrim.bmp"},
    {PAGEID_MAINCFG,      "media/mnucfg.bmp"},
};

static void ico_select_cb(guiObject_t *obj, s8 press_type, const void *data)
{
    (void)obj;
    if (press_type == -1) {
        u16 pos = (long)data;
        PAGE_ChangeByID(menus[pos].id);
    }
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
        int y = 40 + j * 50;
        for(int i = 0; i < 5; i++,pos++) {
            if (pos >= count)
                break;
            int x = 12 + i*60;
            GUI_CreateImageOffset(x, y, 48, 47, 0, 0, menus[pos].file, ico_select_cb, (void *)pos);
        }
    }
}

void MODELMENU_Show(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    PAGE_ChangeByID(PAGEID_MODELMENU);
}

