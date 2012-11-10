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
#include "config/tx.h"

#include <stdlib.h>

#define HELI_LABEL "Heli"  // string too long for devo10, so define it separately for devo8 and devo10
#define PLANE_LABEL "Plane"
#include "../common/_model_page.c"

static u8 _action_cb(u32 button, u8 flags, void *data);

#define VIEW_ID 0
static u8 callback_result;
static s16 view_origin_relativeY;
static s8 current_selected = 0;

void PAGE_ModelInit(int page)
{
    (void)changeicon_cb;
    if (page < 0 && current_selected > 0) // enter this page from childen page , so we need to get its previous mp->current_selected item
        page = current_selected;
    PAGE_SetActionCB(_action_cb);
    PAGE_SetModal(0);
    PAGE_RemoveAllObjects();
    mp->file_state = 0;
    PAGE_ShowHeader(_tr("Model"));
    view_origin_relativeY = 0;
    current_selected = 0;
    mp->total_items = 0;

    // Create a logical view
    u8 view_origin_absoluteX = 0;
    u8 view_origin_absoluteY = ITEM_HEIGHT + 1;
    u8 space = ITEM_HEIGHT + 1;
    GUI_SetupLogicalView(VIEW_ID, 0, 0, LCD_WIDTH -5, LCD_HEIGHT - view_origin_absoluteY ,
        view_origin_absoluteX, view_origin_absoluteY);

    u8 row = 0;
    u8 w = 59;
    u8 x = 63;
    GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
            0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("File:"));
    guiObject_t *obj = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, row),
            w, ITEM_HEIGHT, &DEFAULT_FONT, file_press_cb, file_val_cb, NULL);
    GUI_SetSelected(obj);
    mp->total_items++;

    row += space;
    GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
            0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("Model name:"));
    GUI_CreateButtonPlateText(GUI_MapToLogicalView(VIEW_ID, x ), GUI_MapToLogicalView(VIEW_ID, row),
        w, ITEM_HEIGHT, &DEFAULT_FONT, show_text_cb, 0x0000, _changename_cb, Model.name);
    mp->total_items++;

    row += space;
    GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
            0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("Model type:"));
    GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, row),
            w, ITEM_HEIGHT, &DEFAULT_FONT, type_press_cb, type_val_cb, NULL);
    mp->total_items++;

    row += space;
    GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
            0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("Tx power:"));
    GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, row),
            w, ITEM_HEIGHT, &DEFAULT_FONT, NULL, powerselect_cb, NULL);
    mp->total_items++;

    row += space;
    GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
            w, ITEM_HEIGHT, &DEFAULT_FONT, NULL, protoselect_cb, NULL);
    mp->total_items++;
    mp->obj = GUI_CreateButtonPlateText(GUI_MapToLogicalView(VIEW_ID, x +5 ), GUI_MapToLogicalView(VIEW_ID, row),
            w-10, ITEM_HEIGHT, &DEFAULT_FONT, show_bindtext_cb, 0x0000, bind_cb, NULL);
    mp->total_items++;

    row += space;
    if(Model.fixed_id == 0)
        strncpy(mp->fixed_id, _tr("None"), sizeof(mp->fixed_id));
    else
        sprintf(mp->fixed_id, "%d", (int)Model.fixed_id);
    GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
        0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("Fixed ID:"));
    GUI_CreateButtonPlateText(GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, row),
            w, ITEM_HEIGHT, &DEFAULT_FONT, show_text_cb, 0x0000, fixedid_cb, mp->fixed_id);
    mp->total_items++;

    row += space;
    GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
            0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("# Channels:"));
    mp->chanObj = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, row),
            w, ITEM_HEIGHT, &DEFAULT_FONT, NULL, numchanselect_cb, NULL);
    mp->total_items++;

    // The following items are not draw in the logical view;
    mp->scroll_bar = GUI_CreateScrollbar(LCD_WIDTH - ARROW_WIDTH, ITEM_HEIGHT, LCD_HEIGHT- ITEM_HEIGHT, mp->total_items, NULL, NULL, NULL);
    if (page > 0)
        PAGE_NavigateItems(page, VIEW_ID, mp->total_items, &current_selected, &view_origin_relativeY, mp->scroll_bar);
}

static void _changename_done_cb(guiObject_t *obj, void *data)  // devo8 doesn't handle cancel/discard properly,
{
    (void)obj;
    (void)data;
    GUI_RemoveObj(obj);
    if (callback_result == 1) {  // only change name when DONE is hit, otherwise, discard the change
        strncpy(Model.name, (const char *)mp->tmpstr, sizeof(Model.name));
        //Save model info here so it shows up on the model page
        CONFIG_SaveModelIfNeeded();
    }
    PAGE_ModelInit(-1);
}

static void _changename_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    PAGE_SetModal(0);
    PAGE_RemoveAllObjects();
    strcpy(mp->tmpstr, (const char *)Model.name); // Don't change model name directly
    GUI_CreateKeyboard(KEYBOARD_ALPHA, mp->tmpstr, 20, // no more than 20 chars is allowed for model name
            _changename_done_cb, (void *)&callback_result);
}

static u8 _action_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            PAGE_ChangeByName("SubMenu", sub_menu_item);
        } else if (CHAN_ButtonIsPressed(button, BUT_UP)) {
            PAGE_NavigateItems(-1, VIEW_ID, mp->total_items, &current_selected, &view_origin_relativeY, mp->scroll_bar);
        }  else if (CHAN_ButtonIsPressed(button, BUT_DOWN)) {
            PAGE_NavigateItems(1, VIEW_ID, mp->total_items, &current_selected, &view_origin_relativeY, mp->scroll_bar);
        }
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}
