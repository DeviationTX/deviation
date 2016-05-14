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

#ifndef OVERRIDE_PLACEMENT
#include "common.h"
#include "telemetry.h"
#include "pages.h"
#include "gui/gui.h"
#include "config/model.h"

enum {
    LABEL_X    = 0,
    LABEL_W    = 9,
    TEXTSEL1_X = 9,
    TEXTSEL1_W = 46,
    TEXTSEL2_X = 58,
    TEXTSEL2_W = 21,
    TEXTSEL3_X = 82,
    TEXTSEL3_W = 40,
    MSG_X      = 20,
    MSG_Y      = 10,
};
#endif //OVERRIDE_PLACEMENT

#if HAS_TELEMETRY
#include "../common/_telemconfig_page.c"

static const char *idx_cb(guiObject_t *obj, const void *data);

static u16 current_selected = 0;

static guiObject_t *getobj_cb(int relrow, int col, void *data)
{
    (void)data;
    col = (3 + col) % 3;
    if(col == 0)
        return (guiObject_t *)&gui->name[relrow];
    else if(col == 1)
        return (guiObject_t *)&gui->gtlt[relrow];
    else
        return (guiObject_t *)&gui->value[relrow];
}

static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;
    GUI_CreateLabelBox(&gui->idx[relrow], LABEL_X, y,
            LABEL_W, LINE_HEIGHT, &TINY_FONT, idx_cb, NULL, (void *)(long)absrow);
    GUI_CreateTextSelectPlate(&gui->name[relrow], TEXTSEL1_X, y,
            TEXTSEL1_W, LINE_HEIGHT, &DEFAULT_FONT, NULL, telem_name_cb, (void *)(long)absrow);
    GUI_CreateTextSelectPlate(&gui->gtlt[relrow], TEXTSEL2_X, y,
            TEXTSEL2_W, LINE_HEIGHT, &TINY_FONT, sound_test_cb, gtlt_cb, (void *)(long)absrow);
    GUI_CreateTextSelectPlate(&gui->value[relrow], TEXTSEL3_X, y,
            TEXTSEL3_W, LINE_HEIGHT, &DEFAULT_FONT, NULL, limit_cb, (void *)(long)absrow);
    return 3;
}

void PAGE_TelemconfigInit(int page)
{
    (void)label_cb;
    (void)page;
    //if (page < 0)
    //    page = current_selected;
    PAGE_SetModal(0);
    PAGE_RemoveAllObjects();
    if (telem_state_check() == 0) {
        GUI_CreateLabelBox(&gui->msg, MSG_X, MSG_Y, 0, 0, &DEFAULT_FONT, NULL, NULL, tempstring);
        OBJ_SET_USED(&gui->value, 0);  // A indication not allow to scroll up/down
        return;
    }

    PAGE_ShowHeader(_tr("Telemetry config")); // using the same name as related menu item to reduce language strings

    GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, LCD_WIDTH, LCD_HEIGHT - HEADER_HEIGHT,
                         LINE_SPACE, TELEM_NUM_ALARMS, row_cb, getobj_cb, NULL, NULL);
    PAGE_SetScrollable(&gui->scrollable, &current_selected);
}

static const char *idx_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 idx = (long)data;
    sprintf(tempstring, "%d", idx+1);
    return tempstring;
}

static inline guiObject_t *_get_obj(int idx, int objid)
{
    return GUI_GetScrollableObj(&gui->scrollable, idx, objid);
}

#endif //HAS_TELEMETRY
