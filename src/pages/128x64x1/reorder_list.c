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
        case MOVE_UP:   return _tr("Up");
        case MOVE_DOWN: return _tr("Dn");
        case APPLY:     return _tr("Copy To");
        case INSERT:    return "+";
        case REMOVE:    return "-";
    }
    return "";
}


static int row_cb(int absrow, int relrow, int y, void *data)
{
#define LABEL_X 59
#define LABEL_WIDTH (LCD_WIDTH-59-4)
    (void)data;
    GUI_CreateLabelBox(&gui->name[relrow], LABEL_X, y, LABEL_WIDTH, LINE_HEIGHT,
            &DEFAULT_FONT, list_cb, NULL, (void *)(long)absrow);
    return 0;
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

    u8 space = LINE_HEIGHT;
    u8 y = 0;
    u8 w = 55;

    guiObject_t *obj = GUI_CreateButtonPlateText(&gui->up, 0, y, w/2 -2, LINE_HEIGHT,
            &DEFAULT_FONT,  _show_button_cb, 0x0000, press_button_cb, (void *)MOVE_UP);
    GUI_SetSelected(obj);
    GUI_CreateButtonPlateText(&gui->down, w/2, y, w/2 -2 , LINE_HEIGHT,
            &DEFAULT_FONT, _show_button_cb, 0x0000, press_button_cb, (void *)MOVE_DOWN);
    y += space;
    GUI_CreateTextSelectPlate(&gui->value, 0, y, w, LINE_HEIGHT,
            &DEFAULT_FONT, NULL, value_val_cb, NULL);
    y += space;
    GUI_CreateButtonPlateText(&gui->apply, 0, y, w, LINE_HEIGHT,
            &DEFAULT_FONT, _show_button_cb, 0x0000, press_button_cb, (void *)APPLY);
    y += space;
    GUI_CreateTextSelectPlate(&gui->copy, 0, y, w, LINE_HEIGHT,
            &DEFAULT_FONT, NULL, copy_val_cb, NULL);
    if (requested) {
        y += space;
        GUI_CreateButtonPlateText(&gui->insert, 0, y, w/2 -2, LINE_HEIGHT,
                    &DEFAULT_FONT, _show_button_cb, 0x0000, press_button_cb, (void *)INSERT);
        GUI_CreateButtonPlateText(&gui->remove, w/2, y, w/2 - 2, LINE_HEIGHT,
                    &DEFAULT_FONT, _show_button_cb, 0x0000, press_button_cb, (void *)REMOVE);
    }
    GUI_CreateButtonPlateText(&gui->save, (w+LCD_WIDTH)/2 - 15 + 2, 0, 30, LINE_HEIGHT,
        &DEFAULT_FONT, NULL, 0x0000, okcancel_cb, (void *)_tr("Save"));

    u8 x = w + 4;
    GUI_CreateScrollable(&gui->scrollable, x, LINE_HEIGHT, LCD_WIDTH - x , LCD_HEIGHT-LINE_HEIGHT,
                         LINE_SPACE, rl.max, row_cb, NULL, NULL, NULL);
    GUI_SetSelectable((guiObject_t *)&gui->scrollable, 0);
}

