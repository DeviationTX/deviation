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
#include "telemetry.h"
#include "pages.h"
#include "gui/gui.h"
#include "config/model.h"

#include "../common/_telemconfig_page.c"

void PAGE_TelemconfigInit(int page)
{
    (void)page;
    enum {
        COL1 = (10 + ((LCD_WIDTH - 320) / 2)),
        COL2 = (COL1 + 55),
        COL3 = (COL1 + 156),
        COL4 = (COL1 + 225),
        ROW1 = (70 + ((LCD_HEIGHT - 240) / 2)),
    };
    const u8 row_height = 25;
    PAGE_SetModal(0);
    PAGE_ShowHeader(PAGE_GetName(PAGEID_TELEMCFG));

    if (telem_state_check() == 0) {
        GUI_CreateLabelBox(&gui->msg, 20, 80, 280, 100, &NARROW_FONT, NULL, NULL, tempstring);
        return;
    }

    for (long i = 0; i < TELEM_NUM_ALARMS; i++) {
        GUI_CreateLabelBox(&gui->name[i], COL1, ROW1 + row_height * i, COL2 - COL1, 18, &LABEL_FONT,
           label_cb, NULL, (void *)i);
        GUI_CreateTextSelect(&gui->type[i], COL2, ROW1 + row_height * i, TEXTSELECT_96, NULL, telem_name_cb, (void *)i);
        GUI_CreateTextSelect(&gui->gtlt[i], COL3, ROW1 + row_height * i, TEXTSELECT_64, sound_test_cb, gtlt_cb, (void *)i);
        GUI_CreateTextSelect(&gui->value[i], COL4, ROW1 + row_height * i, TEXTSELECT_64, NULL, limit_cb, (void *)i);
    }
}

static inline guiObject_t *_get_obj(int idx, int objid)
{
    (void)objid;
    return (guiObject_t *)&gui->value[idx];
}
