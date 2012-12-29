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
#include "simple.h"
#include "../../common/simple/_switchassign_page.c"

static u8 _action_cb(u32 button, u8 flags, void *data);

static s16 view_origin_relativeY;
static s8 current_selected = 0;

void PAGE_SwitchAssignInit(int page)
{
    if (page < 0 && current_selected > 0) // enter this page from childen page , so we need to get its previous mp->current_selected item
        page = current_selected;
    PAGE_SetActionCB(_action_cb);
    PAGE_SetModal(0);
    PAGE_RemoveAllObjects();
    view_origin_relativeY = 0;
    current_selected = 0;
    refresh_switches();

    PAGE_ShowHeader(_tr("Press ENT to change:"));

    // Create a logical view
    u8 view_origin_absoluteX = 0;
    u8 view_origin_absoluteY = ITEM_SPACE;
    GUI_SetupLogicalView(VIEW_ID, 0, 0, LCD_WIDTH - ARROW_WIDTH, LCD_HEIGHT - view_origin_absoluteY ,
        view_origin_absoluteX, view_origin_absoluteY);

    u8 row = 0;
    u8 w = 66;
    u8 x = 58;
    GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
            0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("Fly mode"));
    GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, row),
            w, ITEM_HEIGHT, &DEFAULT_FONT, NULL, switch_cb, (void *)(long)SWITCHFUNC_FLYMODE);
    row += ITEM_SPACE;

    GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
            0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("Thr hold"));
    GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, row),
            w, ITEM_HEIGHT, &DEFAULT_FONT, NULL, switch_cb, (void *)(long)SWITCHFUNC_HOLD);
    row += ITEM_SPACE;

    GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
            0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("Gyro sense"));
    GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, row),
            w, ITEM_HEIGHT, &DEFAULT_FONT, NULL, switch_cb, (void *)(long)SWITCHFUNC_GYROSENSE);
    row += ITEM_SPACE;

    GUI_Select1stSelectableObj();

}

static u8 _action_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            // Todo: check if there is duplicate switches
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
