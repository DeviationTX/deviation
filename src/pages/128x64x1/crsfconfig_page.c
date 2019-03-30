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

#include "../common/_crsfconfig_page.c"

static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;

    GUI_CreateLabelBox(&gui->name[relrow], LABEL_X, y,
            LABEL_WIDTH, LINE_HEIGHT, &LABEL_FONT, crsfconfig_str_cb, press_cb, (void *)absrow);
    return 1;
}

void PAGE_CrsfconfigInit(int page)
{
    (void)page;
    PAGE_SetModal(0);

#ifndef EMULATOR
    memset(crsf_devices, 0, sizeof crsf_devices);
    CRSF_ping_devices();    // ask all TBS devices to respond with device info
#else
    number_of_devices = sizeof crsf_devices / sizeof crsf_devices[0];
#endif

    PAGE_ShowHeader(PAGE_GetName(PAGEID_CRSFCFG));
    GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, LCD_WIDTH, LCD_HEIGHT - HEADER_HEIGHT,
                     LINE_SPACE, CRSF_MAX_DEVICES,
                     row_cb, NULL, NULL, NULL);
    PAGE_SetScrollable(&gui->scrollable, &current_selected);
}
#endif
