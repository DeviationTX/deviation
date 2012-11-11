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

static u8 _action_cb(u32 button, u8 flags, void *data);
static const char *idx_cb(guiObject_t *obj, const void *data);

#define VIEW_ID 0

static u8 total_items;
static guiObject_t *scroll_bar;
static s8 current_selected = 0;
static s16 view_origin_relativeY = 0;

void PAGE_TelemconfigInit(int page)
{
    (void)label_cb;
    if (page < 0)
        page = current_selected;
    PAGE_SetModal(0);
    PAGE_RemoveAllObjects();
    PAGE_SetActionCB(_action_cb);
    PAGE_ShowHeader(_tr("Telemetry"));
    current_selected = 0;
    total_items = 0;

    // Create 1 logical view
    u8 view_origin_absoluteX = 0;
    u8 view_origin_absoluteY = ITEM_HEIGHT + 1;
    u8 space = ITEM_HEIGHT + 1;
    GUI_SetupLogicalView(VIEW_ID, 0, 0, LCD_WIDTH -5, LCD_HEIGHT - view_origin_absoluteY ,
            view_origin_absoluteX, view_origin_absoluteY);

    u8 row = 0;
    u8 w1 = 42;
    u8 w2 = 23;
    u8 w3 = 40;
    u8 i;
    for (i = 0; i < TELEM_NUM_ALARMS; i++) {
        u8 x = 9;
        GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
                9, ITEM_HEIGHT, &TINY_FONT, idx_cb, NULL, (void *)(long)i);
        GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, row),
                w1, ITEM_HEIGHT, &DEFAULT_FONT, NULL, telem_name_cb, (void *)(long)i);
        x += w1 + 5;
        GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, row),
                w2, ITEM_HEIGHT, &TINY_FONT, NULL, gtlt_cb, (void *)(long)i);
        x += w2 + 3;
        tp.valueObj[i] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, row),
                w3, ITEM_HEIGHT, &DEFAULT_FONT, NULL, limit_cb, (void *)(long)i);
        total_items++;
        row += space;
    }
    total_items *= 3;
    total_items++;
    scroll_bar = GUI_CreateScrollbar(LCD_WIDTH - ARROW_WIDTH, ITEM_HEIGHT, LCD_HEIGHT- ITEM_HEIGHT, total_items, NULL, NULL, NULL);
    if (page > 0)
        PAGE_NavigateItems(page, VIEW_ID, total_items, &current_selected, &view_origin_relativeY, scroll_bar);
}

static const char *idx_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 idx = (long)data;
    sprintf(tp.str, "%d", idx+1);
    return tp.str;
}

static u8 _action_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            PAGE_ChangeByName("SubMenu", sub_menu_item);
        }
        else if (CHAN_ButtonIsPressed(button, BUT_UP)) {
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

