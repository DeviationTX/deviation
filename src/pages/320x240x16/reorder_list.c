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

static const char *_show_button_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    switch((long)(data)) {
        case MOVE_UP:   return _tr("Move Up");
        case MOVE_DOWN: return _tr("Move Down");
        case APPLY:     return _tr("Copy To");
        case INSERT:    return _tr("Insert");
        case REMOVE:    return _tr("Remove");
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
    int i;
    for(i = 0; i < rl.max; i++) {
        if (i < count)
            list[i] = i+1;
        else
            list[i] = 0;
    }
    PAGE_CreateCancelButton(160, 4, okcancel_cb);
    PAGE_CreateOkButton(264, 4, okcancel_cb);
    GUI_CreateButton(8, 40, BUTTON_96x16, _show_button_cb, 0x0000, press_button_cb, (void *)MOVE_UP);
    GUI_CreateButton(8, 60, BUTTON_96x16, _show_button_cb, 0x0000, press_button_cb, (void *)MOVE_DOWN);

    rl.textsel = GUI_CreateTextSelect(8, 90, TEXTSELECT_96, 0x0000, NULL, copy_val_cb, NULL);
    GUI_CreateButton(8, 110, BUTTON_96x16, _show_button_cb, 0x0000, press_button_cb, (void *)APPLY);
    if (max_allowed) {
        GUI_CreateButton(8, 140, BUTTON_96x16, _show_button_cb, 0x0000, press_button_cb, (void *)INSERT);
        GUI_CreateButton(8, 160, BUTTON_96x16, _show_button_cb, 0x0000, press_button_cb, (void *)REMOVE);
    }
    rl.listbox = GUI_CreateListBox(112, 40, 200, 192, rl.max, selected, string_cb, select_cb, NULL, NULL);
}
