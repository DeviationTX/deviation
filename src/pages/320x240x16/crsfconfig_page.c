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
#include "pages.h"
#include "gui/gui.h"

#ifndef OVERRIDE_PLACEMENT
enum {
    LABEL_X        = 0,
    LABEL_WIDTH    = 122,
    MSG_X          = 20,
    MSG_Y          = 10,
};
#endif

#if SUPPORT_CRSF_CONFIG

#define LINE_HEIGHT    18
#define HEADER_HEIGHT  40
#define LINE_SPACE     20

#include "../common/_crsfconfig_page.c"

static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;

    GUI_CreateLabelBox(&gui->name[relrow], LCD_WIDTH/2-100, y,
            200 - ARROW_WIDTH, LINE_HEIGHT, &LABEL_FONT, crsfconfig_str_cb, press_cb, (void *)absrow);
    return 1;
}

void PAGE_CrsfconfigInit(int page)
{
    (void)page;
    PAGE_SetModal(0);

    memset(crsf_devices, 0, sizeof crsf_devices);
    CRSF_ping_devices(ADDR_BROADCAST);    // ask all TBS devices to respond with device info

    PAGE_ShowHeader(PAGE_GetName(PAGEID_CRSFCFG));
    GUI_CreateScrollable(&gui->scrollable, LCD_WIDTH/2-100, HEADER_HEIGHT, LCD_WIDTH,
                     LISTBOX_ITEMS * LINE_HEIGHT, LINE_HEIGHT,
                     CRSF_MAX_DEVICES, row_cb, NULL, NULL, NULL);
    PAGE_SetScrollable(&gui->scrollable, &current_selected);
}
#endif
