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
#include "config/model.h"

#include "../common/_timer_page.c"

static u8 _action_cb(u32 button, u8 flags, void *data);
#define VIEW_ID 0
static s8 current_page = 0;
static u8 view_height;
static guiObject_t *scroll_bar;

static void _show_page()
{
    PAGE_SetActionCB(_action_cb);
    u8 space = ITEM_HEIGHT + 5;
    // create a logical view
    u8 view_origin_absoluteX = 0;
    u8 view_origin_absoluteY = ITEM_HEIGHT + 1;
    view_height = space * 3;
    GUI_SetupLogicalView(VIEW_ID, 0, 0, LCD_WIDTH -5, view_height, view_origin_absoluteX, view_origin_absoluteY);

    u8 y = 0;
    u8 w = 65;
    u8 x = 55;
    for (u8 i = 0; i < NUM_TIMERS; i++) {
        y = i * (space * 3);
        //Row 1
        GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, y),
                0, ITEM_HEIGHT, &DEFAULT_FONT, timer_str_cb, NULL, (void *)(long)i);
        guiObject_t *obj =GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, y),
                w, ITEM_HEIGHT, &DEFAULT_FONT, toggle_timertype_cb, set_timertype_cb, (void *)(long)i);
        if (i == 0)
            GUI_SetSelected(obj);

        //Row 2
        y += space;
        GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, y),
                0, ITEM_HEIGHT,&DEFAULT_FONT, NULL, NULL, _tr("Switch:"));
        GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, y),
                w, ITEM_HEIGHT, &DEFAULT_FONT, toggle_source_cb, set_source_cb, (void *)(long)i);
        //Row 3
        y += space;
        tp->startLabelObj[i] = GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, y),
                50, // bug fix: label width and height can't be 0, otherwise, the label couldn't be hidden dynamically
                ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("Start:"));
        tp->startObj[i] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, y),
                w, ITEM_HEIGHT, &DEFAULT_FONT,NULL, set_start_cb, (void *)(long)i);

        update_countdown(i);
    }
    space = ITEM_HEIGHT + 1;
    scroll_bar = GUI_CreateScrollbar(LCD_WIDTH - 3, space, LCD_HEIGHT- space, NUM_TIMERS, NULL, NULL, NULL);
}

static void _navigate_items(s8 direction)
{
    guiObject_t *obj = GUI_GetSelected();
    if (direction > 0) {
        GUI_SetSelected((guiObject_t *)GUI_GetNextSelectable(obj));
    } else {
        GUI_SetSelected((guiObject_t *)GUI_GetPrevSelectable(obj));
    }
    obj = GUI_GetSelected();
    if (!GUI_IsObjectInsideCurrentView(VIEW_ID, obj)) {
        current_page = (current_page + direction)%NUM_TIMERS;
        if (current_page < 0)
            current_page = NUM_TIMERS -1; //rewind to last page
        if (current_page == 0)
            GUI_SetRelativeOrigin(VIEW_ID, 0, 0);
        else
            GUI_ScrollLogicalView(VIEW_ID, view_height);
    }
    GUI_SetScrollbar(scroll_bar, current_page);
}

static u8 _action_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    if (flags & BUTTON_PRESS || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            PAGE_ChangeByName("SubMenu", sub_menu_item);
        }  else if (CHAN_ButtonIsPressed(button, BUT_UP)) {
            _navigate_items(-1);
        }  else if (CHAN_ButtonIsPressed(button,BUT_DOWN)) {
            _navigate_items(1);
        } else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}
