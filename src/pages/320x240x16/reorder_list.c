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

static const int REORD_XOFFSET = ((LCD_WIDTH - 320) / 2);

static const char *_show_button_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    switch((long)(data)) {
        case MOVE_UP:   return _tr("Move Up");
        case MOVE_DOWN: return _tr("Move Down");
        case APPLY:     return _tr("Copy To");
        case INSERT:    return _tr("Insert");
        case REMOVE:    return _tr("Remove");
        case SAVE:      return _tr("Save");
    }
    return "";
}

static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;
    struct LabelDesc listbox = {
        .font = DEFAULT_FONT.font,
        .style = LABEL_LISTBOX,
        .font_color = DEFAULT_FONT.font_color,
        .fill_color = DEFAULT_FONT.fill_color,
        .outline_color = DEFAULT_FONT.outline_color
    };
    GUI_CreateLabelBox(&gui->name[relrow], 112 + REORD_XOFFSET, y,
            200 - ARROW_WIDTH, 24, &listbox, list_cb, NULL, (void *)(long)absrow);
    return 1;
}


void PAGE_ReorderInit(int page)
{
    (void)page;
    int i;
    int requested = rl.max;
    if (rl.max < rl.count)
        rl.max = rl.count;

    for(i = 0; i < rl.max; i++) {
        if (i < rl.count)
            rl.list[i] = i+1;
        else
            rl.list[i] = 0;
    }
    PAGE_ShowHeader("");
    GUI_CreateButton(&gui->save, 220 + (LCD_WIDTH - 320), 4, BUTTON_96, _show_button_cb, 0x0000, okcancel_cb, (void *)SAVE);
    GUI_CreateButton(&gui->up, 8 + REORD_XOFFSET, 40, BUTTON_96x16, _show_button_cb, 0x0000, press_button_cb, (void *)MOVE_UP);
    GUI_CreateButton(&gui->down, 8 + REORD_XOFFSET, 60, BUTTON_96x16, _show_button_cb, 0x0000, press_button_cb, (void *)MOVE_DOWN);

    GUI_CreateTextSelect(&gui->value, 8 + REORD_XOFFSET, 90, TEXTSELECT_96, NULL, value_val_cb, NULL);
    GUI_CreateButton(&gui->apply, 8 + REORD_XOFFSET, 110, BUTTON_96x16, _show_button_cb, 0x0000, press_button_cb, (void *)APPLY);
    GUI_CreateTextSelect(&gui->copy, 8 + REORD_XOFFSET, 130, TEXTSELECT_96, NULL, copy_val_cb, NULL);
    if (requested) {
        GUI_CreateButton(&gui->insert, 8 + REORD_XOFFSET, 160, BUTTON_96x16, _show_button_cb, 0x0000, press_button_cb, (void *)INSERT);
        GUI_CreateButton(&gui->remove, 8 + REORD_XOFFSET, 180, BUTTON_96x16, _show_button_cb, 0x0000, press_button_cb, (void *)REMOVE);
    }
    GUI_CreateScrollable(&gui->scrollable, 112 + REORD_XOFFSET, 40, 200, LISTBOX_ITEMS * 24,
                         24, rl.max, row_cb, NULL, NULL, NULL);
}
