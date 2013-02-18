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
#include "config/model.h"
#include "standard.h"
#include "../../common/standard/_switchassign_page.c"

static u8 _action_cb(u32 button, u8 flags, void *data);

static guiObject_t *getobj_cb(int relrow, int col, void *data)
{
    (void)col;
    (void)data;
    return (guiObject_t *)&gui->value[relrow];
}

static const char *label_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    FunctionSwitch i = (long)data;
    switch (i) {
    case SWITCHFUNC_FLYMODE:
        strcpy(mp->tmpstr, _tr("Fly mode"));
        break;
    case SWITCHFUNC_HOLD:
        strcpy(mp->tmpstr, _tr("Thr hold"));
        break;
    case SWITCHFUNC_GYROSENSE:
        strcpy(mp->tmpstr, _tr("Gyro sense"));
        break;
    case SWITCHFUNC_DREXP_AIL:
        strcpy(mp->tmpstr, _tr("D/R&Exp -AIL"));
        break;
    case SWITCHFUNC_DREXP_ELE:
        strcpy(mp->tmpstr, _tr("D/R&Exp -ELE"));
        break;
    case SWITCHFUNC_DREXP_RUD:
    default:
        strcpy(mp->tmpstr, _tr("D/R&Exp -RUD"));
        break;
    }
    return mp->tmpstr;
}

static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;
    u8 w = 46;
    u8 x = 77;
    GUI_CreateLabelBox(&gui->name[relrow], 0, y, 0, ITEM_HEIGHT, &DEFAULT_FONT, label_cb, NULL, (void *)(long)absrow);
    GUI_CreateTextSelectPlate(&gui->value[relrow], x, y,
        w, ITEM_HEIGHT, &DEFAULT_FONT, NULL, switch_cb, (void *)(long)absrow);
    return 1;
}

void PAGE_SwitchAssignInit(int page)
{
    (void)page;
    PAGE_SetActionCB(_action_cb);
    PAGE_SetModal(0);
    PAGE_RemoveAllObjects();
    refresh_switches();

    PAGE_ShowHeader(_tr("Press ENT to change:"));
    GUI_CreateScrollable(&gui->scrollable, 0, ITEM_HEIGHT + 1, LCD_WIDTH, LCD_HEIGHT - ITEM_HEIGHT -1,
                     ITEM_SPACE, SWITCHFUNC_LAST, row_cb, getobj_cb, NULL, NULL);
    GUI_SetSelected(GUI_ShowScrollableRowOffset(&gui->scrollable, 0));
}

static u8 _action_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            PAGE_ChangeByID(PAGEID_MENU, PREVIOUS_ITEM);
        } else if (CHAN_ButtonIsPressed(button, BUT_ENTER)) {
            save_changes();
        }
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}
