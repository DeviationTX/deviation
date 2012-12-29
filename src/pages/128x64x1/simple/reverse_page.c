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
#include "../../common/simple/_reverse_page.c"

static u8 _action_cb(u32 button, u8 flags, void *data);

static s16 view_origin_relativeY;
static s8 current_selected = 0;
guiObject_t *scroll_bar;

void PAGE_ReverseInit(int page)
{
    if (page < 0 && current_selected > 0) // enter this page from childen page , so we need to get its previous mp->current_selected item
        page = current_selected;
    PAGE_SetActionCB(_action_cb);
    PAGE_SetModal(0);
    PAGE_RemoveAllObjects();
    view_origin_relativeY = 0;
    current_selected = 0;

    // Create a logical view
    u8 view_origin_absoluteX = 0;
    u8 view_origin_absoluteY = 0;
    GUI_SetupLogicalView(VIEW_ID, 0, 0, LCD_WIDTH - ARROW_WIDTH, LCD_HEIGHT - view_origin_absoluteY ,
        view_origin_absoluteX, view_origin_absoluteY);

    u8 row = 0;
    u8 w = 59;
    u8 x = 58;
    for (u8 i = 0; i < Model.num_channels; i++) {
        GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
                0, ITEM_HEIGHT, &DEFAULT_FONT, SIMPLEMIX_channelname_cb, NULL, (void *)(long)i);
        GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, row),
                w, ITEM_HEIGHT, &DEFAULT_FONT, NULL, reverse_cb, (void *)(long)i);
        row += ITEM_SPACE;
    }
    GUI_Select1stSelectableObj();

    // The following items are not draw in the logical view;
    scroll_bar = GUI_CreateScrollbar(LCD_WIDTH - ARROW_WIDTH, 0, LCD_HEIGHT, Model.num_channels, NULL, NULL, NULL);
    if (page > 0)
        PAGE_NavigateItems(page, VIEW_ID,Model.num_channels, &current_selected, &view_origin_relativeY, scroll_bar);
}

static u8 _action_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    u8 total_items = GUI_GetScrollbarNumItems(scroll_bar);
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            PAGE_ChangeByID(PAGEID_MENU, PREVIOUS_ITEM);
        } else if (CHAN_ButtonIsPressed(button, BUT_UP)) {
            PAGE_NavigateItems(-1, VIEW_ID, total_items, &current_selected, &view_origin_relativeY, scroll_bar);
        }  else if (CHAN_ButtonIsPressed(button, BUT_DOWN)) {
            PAGE_NavigateItems(1, VIEW_ID, total_items, &current_selected, &view_origin_relativeY, scroll_bar);
        }
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}
