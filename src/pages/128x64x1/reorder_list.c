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

#include "../common/_reorder_list.c"

#define VIEW_ID 0
static s8 current_selected = 0;

static u8 _action_cb(u32 button, u8 flags, void *data);
static void _okcancel_cb(guiObject_t *obj, const void *data);
static struct buttonAction action;

static const char *_show_button_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    switch((long)(data)) {
        case MOVE_UP:   return _tr("Up");
        case MOVE_DOWN: return _tr("Dn");
        case APPLY:     return _tr("Copy To");
        case INSERT:    return "+";
        case REMOVE:    return "-";
    }
    return "";
}

void PAGE_ShowReorderList(u8 *list, u8 count, u8 selected, u8 max_allowed, const char *(*text_cb)(u8 idx), void(*return_page)(u8 *))
{
    rl.return_page = return_page;
    rl.list = list;
    rl.selected = selected;
    rl.copyto = selected;
    rl.count = count;
    rl.text_cb = text_cb;
    rl.max = max_allowed;
    if (rl.max < count)
        rl.max = count;

    PAGE_RemoveAllObjects();
    PAGE_SetModal(1);
    current_selected = 0;
    int i;
    for(i = 0; i < rl.max; i++) {
        if (i < count)
            list[i] = i+1;
        else
            list[i] = 0;
    }

    u8 space = ITEM_HEIGHT + 1;
    u8 y = 0;
    u8 w = 55;

    // Create a logical view
    u8 view_origin_absoluteX = 0;
    u8 view_origin_absoluteY =  0;
    GUI_SetupLogicalView(VIEW_ID, 0, 0, w, LCD_HEIGHT , view_origin_absoluteX, view_origin_absoluteY);

    guiObject_t *obj = GUI_CreateButtonPlateText(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, y), w/2 -2, ITEM_HEIGHT,
            &DEFAULT_FONT,  _show_button_cb, 0x0000, press_button_cb, (void *)MOVE_UP);
    GUI_SetSelected(obj);
    GUI_CreateButtonPlateText(GUI_MapToLogicalView(VIEW_ID, w/2), GUI_MapToLogicalView(VIEW_ID, y), w/2 -2 , ITEM_HEIGHT,
            &DEFAULT_FONT, _show_button_cb, 0x0000, press_button_cb, (void *)MOVE_DOWN);
    y += space;
    rl.textsel = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, y), w, ITEM_HEIGHT,
            &DEFAULT_FONT, NULL, copy_val_cb, NULL);
    y += space;
    GUI_CreateButtonPlateText(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, y), w, ITEM_HEIGHT,
            &DEFAULT_FONT, _show_button_cb, 0x0000, press_button_cb, (void *)APPLY);
    if (max_allowed) {
        y += space;
        GUI_CreateButtonPlateText(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, y), w/2 -2, ITEM_HEIGHT,
                    &DEFAULT_FONT, _show_button_cb, 0x0000, press_button_cb, (void *)INSERT);
        GUI_CreateButtonPlateText(GUI_MapToLogicalView(VIEW_ID, w/2), GUI_MapToLogicalView(VIEW_ID, y), w/2 - 2, ITEM_HEIGHT,
                    &DEFAULT_FONT, _show_button_cb, 0x0000, press_button_cb, (void *)REMOVE);
    }
    y += space;
    GUI_CreateButtonPlateText(GUI_MapToLogicalView(VIEW_ID, (w -30)/2), GUI_MapToLogicalView(VIEW_ID, y), 30, ITEM_HEIGHT,
        &DEFAULT_FONT, NULL, 0x0000, _okcancel_cb, (void *)_tr("Save"));

    u8 x = w + 4;
    rl.listbox = GUI_CreateListBoxPlateText(x, 0, LCD_WIDTH - x , LCD_HEIGHT, rl.max, selected, &DEFAULT_FONT,
        LISTBOX_KEY_RIGHTLEFT, string_cb, select_cb, NULL, NULL);
    GUI_SetSelectable(rl.listbox, 0);

    PAGE_SetActionCB(NULL);
    // we need to grab the key handler from the listbox to let rl.textsel catch left/right keys when it is selected
    // hence registerCallback has to be used here
    BUTTON_RegisterCallback(&action,
            CHAN_ButtonMask(BUT_ENTER)
            | CHAN_ButtonMask(BUT_EXIT)
            | CHAN_ButtonMask(BUT_LEFT)
            | CHAN_ButtonMask(BUT_RIGHT)
            | CHAN_ButtonMask(BUT_UP)
            | CHAN_ButtonMask(BUT_DOWN),
            BUTTON_PRESS | BUTTON_LONGPRESS | BUTTON_PRIORITY,
            _action_cb, obj);
}

static void _okcancel_cb(guiObject_t *obj, const void *data)
{
    BUTTON_UnregisterCallback(&action);
    okcancel_cb(obj, data);
}

static u8 _action_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            BUTTON_UnregisterCallback(&action);
            GUI_RemoveAllObjects();
            rl.return_page(NULL);
        } else if (CHAN_ButtonIsPressed(button, BUT_LEFT) && (rl.textsel == GUI_GetSelected())) {
            // catch the left/right keys when r1.textsel is selected
            copy_val_cb(NULL, 1, NULL);
            GUI_Redraw(rl.textsel);

        } else if (CHAN_ButtonIsPressed(button, BUT_RIGHT)&& (rl.textsel == GUI_GetSelected())) {
            copy_val_cb(NULL, -1, NULL);
            GUI_Redraw(rl.textsel);
        }
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}
