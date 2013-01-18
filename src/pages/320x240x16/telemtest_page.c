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
#include "telemetry.h"
#include "gui/gui.h"

#include "../common/_telemtest_page.c"

static void show_page()
{
    const u8 row_height = 20;
    for(long i = 0; i < 4; i++) {
        GUI_CreateLabelBox(&gui1->templbl[i], 10, 40 + i*row_height, 40, 16, &TELEM_TXT_FONT,
                           label_cb, NULL, (void *)(TELEM_TEMP1+i));
        GUI_CreateLabelBox(&gui1->temp[i], 60,  40 + i*row_height, 40, 16, &TELEM_ERR_FONT,
                           telem_cb, NULL, (void *)(TELEM_TEMP1+i));
    }
    for(long i = 0; i < 3; i++) {
        GUI_CreateLabelBox(&gui1->voltlbl[i], 110, 40 + i*row_height, 40, 16, &TELEM_TXT_FONT,
                           label_cb, NULL, (void *)(TELEM_VOLT1+i));
        GUI_CreateLabelBox(&gui1->volt[i], 155,  40 + i*row_height, 40, 16, &TELEM_ERR_FONT,
                           telem_cb, NULL, (void *)(TELEM_VOLT1+i));
    }
    for(long i = 0; i < 2; i++) {
        GUI_CreateLabelBox(&gui1->rpmlbl[i], 210, 40 + i*row_height, 40, 16, &TELEM_TXT_FONT,
                           label_cb, NULL, (void *)(TELEM_RPM1+i));
        GUI_CreateLabelBox(&gui1->rpm[i], 255,  40 + i*row_height, 40, 16, &TELEM_ERR_FONT,
                           telem_cb, NULL, (void *)(TELEM_RPM1+i));
    }
    for(long i = 0; i < 5; i++) {
        GUI_CreateLabelBox(&gui1->gpslbl[i], 20, 140 + i*row_height, 60, 16, &TELEM_TXT_FONT,
                           label_cb, NULL, (void *)(TELEM_GPS_LAT+i));
        GUI_CreateLabelBox(&gui1->gps[i], 100,  140 + i*row_height, 200, 16, &TELEM_ERR_FONT,
                           telem_cb, NULL, (void *)(TELEM_GPS_LAT+i));
    }
    tp.telem = Telemetry;
    tp.telem.time[0] = 0;
    tp.telem.time[1] = 0;
    tp.telem.time[2] = 0;
}

void PAGE_TelemtestInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    PAGE_ShowHeader(PAGE_GetName(PAGEID_TELEMMON));
    if (telem_state_check() == 0) {
        GUI_CreateLabelBox(&gui1->msg, 20, 80, 280, 100, &NARROW_FONT, NULL, NULL, tp.str);
        return;
    }
    show_page();
}

void PAGE_TelemtestModal(void(*return_page)(int page), int page)
{
    PAGE_SetModal(1);
    tp.return_page = return_page;
    tp.return_val = page;
    PAGE_RemoveAllObjects();

    PAGE_ShowHeader_ExitOnly(PAGE_GetName(PAGEID_TELEMMON), okcancel_cb);
    if (telem_state_check() == 0) {
        GUI_CreateLabelBox(&gui1->msg, 20, 80, 280, 100, &NARROW_FONT, NULL, NULL, tp.str);
        return;
    }

    show_page();
}
static inline guiObject_t *_get_obj(int idx, int objid) {
    return (guiObject_t *)&gui1->gps[idx*2 + objid];
}
