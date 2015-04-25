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

#if HAS_TELEMETRY
#include "../common/_telemconfig_page.c"

static unsigned _action_cb(u32 button, unsigned flags, void *data);
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
    int x = 9;
    u8 w1 = 46;
    u8 w2 = 21;
    u8 w3 = 40;
    GUI_CreateLabelBox(&gui->idx[relrow], 0, y,
            9, LINE_HEIGHT, &TINY_FONT, idx_cb, NULL, (void *)(long)absrow);
    GUI_CreateTextSelectPlate(&gui->name[relrow], x, y,
            w1, LINE_HEIGHT, &DEFAULT_FONT, NULL, telem_name_cb, (void *)(long)absrow);
    x += w1 + 3;
    GUI_CreateTextSelectPlate(&gui->gtlt[relrow], x, y,
            w2, LINE_HEIGHT, &TINY_FONT, sound_test_cb, gtlt_cb, (void *)(long)absrow);
    x += w2 + 3;
    GUI_CreateTextSelectPlate(&gui->value[relrow], x, y,
            w3, LINE_HEIGHT, &DEFAULT_FONT, NULL, limit_cb, (void *)(long)absrow);
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
    PAGE_SetActionCB(_action_cb);
    if (telem_state_check() == 0) {
        GUI_CreateLabelBox(&gui->msg, 20, 10, 0, 0, &DEFAULT_FONT, NULL, NULL, tempstring);
        OBJ_SET_USED(&gui->value, 0);  // A indication not allow to scroll up/down
        return;
    }

    PAGE_ShowHeader(_tr("Telemetry config")); // using the same name as related menu item to reduce language strings

    GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, LCD_WIDTH, LCD_HEIGHT - HEADER_HEIGHT,
                         LINE_SPACE, TELEM_NUM_ALARMS, row_cb, getobj_cb, NULL, NULL);
    GUI_SetSelected(GUI_ShowScrollableRowOffset(&gui->scrollable, current_selected));
}
void PAGE_TelemconfigExit()
{
    if(telem_state_check())
        current_selected = GUI_ScrollableGetObjRowOffset(&gui->scrollable, GUI_GetSelected());
}

static const char *idx_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 idx = (long)data;
    sprintf(tempstring, "%d", idx+1);
    return tempstring;
}

static unsigned _action_cb(u32 button, unsigned flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            PAGE_ChangeByID(PAGEID_MENU, PREVIOUS_ITEM);
        }
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}

static inline guiObject_t *_get_obj(int idx, int objid)
{
    return GUI_GetScrollableObj(&gui->scrollable, idx, objid);
}

#endif //HAS_TELEMETRY
