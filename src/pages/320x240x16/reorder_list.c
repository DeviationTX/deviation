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

void redraw() {
}

static void display_list(int idx)
{
    GUI_ShowScrollableRowCol(&gui->scrollable, idx, 0);
}

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
   PAGE_PushByID(PAGEID_REORDER, 0);
}

void PAGE_ReorderInit(int page)
{
    (void)page;
    int i;
    for(i = 0; i < rl.max; i++) {
        if (i < rl.count)
            rl.list[i] = i+1;
        else
            rl.list[i] = 0;
    }
    PAGE_CreateCancelButton(160 + (LCD_WIDTH - 320), 4, okcancel_cb);
    PAGE_CreateOkButton(264 + (LCD_WIDTH - 320), 4, okcancel_cb);
    GUI_CreateButton(&gui->up, 8 + REORD_XOFFSET, 40, BUTTON_96x16, _show_button_cb, 0x0000, press_button_cb, (void *)MOVE_UP);
    GUI_CreateButton(&gui->down, 8 + REORD_XOFFSET, 60, BUTTON_96x16, _show_button_cb, 0x0000, press_button_cb, (void *)MOVE_DOWN);

    GUI_CreateTextSelect(&gui->value, 8 + REORD_XOFFSET, 90, TEXTSELECT_96, NULL, copy_val_cb, NULL);
    GUI_CreateButton(&gui->apply, 8 + REORD_XOFFSET, 110, BUTTON_96x16, _show_button_cb, 0x0000, press_button_cb, (void *)APPLY);
    if (rl.max) {
        GUI_CreateButton(&gui->insert, 8 + REORD_XOFFSET, 140, BUTTON_96x16, _show_button_cb, 0x0000, press_button_cb, (void *)INSERT);
        GUI_CreateButton(&gui->remove, 8 + REORD_XOFFSET, 160, BUTTON_96x16, _show_button_cb, 0x0000, press_button_cb, (void *)REMOVE);
    }
    GUI_CreateScrollable(&gui->scrollable, 112 + REORD_XOFFSET, 40, 200, LCD_HEIGHT - 48,
                         24, rl.max, row_cb, NULL, NULL, NULL);
}
