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

#ifndef OVERRIDE_PLACEMENT
enum {
    LABEL_X          = 2,
    LABEL_WIDTH      = 120,
    MSG_X            = 20,
    MSG_Y            = 10,
    EDIT_LABEL_WIDTH = 60,
    EDIT_VALUE_X     = 66,
    EDIT_VALUE_WIDTH = 56,
};
#endif

#if SUPPORT_CRSF_CONFIG

#define LINE_HEIGHT    18
#define HEADER_HEIGHT  40
#define LINE_SPACE     20

#include "../common/_crsfdevice_page.c"

static int row_cb(int absrow, int relrow, int y, void *data) {
    (void)data;

    crsf_param_t *param = current_param(absrow);
    if (!param) return 0;
    void (*lbl_press_cb)(struct guiObject *obj, s8 press_type, const void *data) = NULL;

    switch (param->type) {
    case TEXT_SELECTION:
        if (non_null_values(param) > 1 && (param->max_value - param->min_value > 1)) {
            GUI_CreateTextSelect(&gui->value[relrow].ts, LCD_WIDTH - 199, y,
                TEXTSELECT_128, NULL, value_textsel, (void *)param);
        } else {
            GUI_CreateButton(&gui->value[relrow].but, LCD_WIDTH - 199 + ARROW_WIDTH, y,
                BUTTON_96x16, crsf_value_cb, button_press, (void *)param);
        }
        break;
    case COMMAND:
        lbl_press_cb = command_press;
        break;
    case INFO:
        GUI_CreateLabelBox(&gui->value[relrow].lbl, LCD_WIDTH - 199, y,
            199, LINE_HEIGHT, &LABEL_FONT, crsf_value_cb, noop_press, (void *)param);
        break;
    case FOLDER:
        lbl_press_cb = folder_cb;
        break;
    case UINT8:
    case UINT16:
    case INT8:
    case INT16:
    case FLOAT:
        GUI_CreateTextSelect(&gui->value[relrow].ts, LCD_WIDTH - 199, y,
            TEXTSELECT_96, NULL, value_numsel, (void *)param);
        break;
    case STRING:
        GUI_CreateLabelBox(&gui->value[relrow].lbl, LCD_WIDTH - 199, y,
            199, LINE_HEIGHT, &LABEL_FONT, crsf_value_cb, stredit_cb, (void *)param);
        break;
    case OUT_OF_RANGE:
        break;
    }

    GUI_CreateLabelBox(&gui->name[relrow], 15, y,
        LCD_WIDTH - 215, LINE_HEIGHT, &LABEL_FONT,
        crsf_name_cb, lbl_press_cb, (void *)param);

    return 1;
}

static void show_header() {}

static void show_page(u8 folder) {
    int row_count = folder_rows(folder);
    int row_idx = GUI_ScrollableGetObjRowOffset(&gui->scrollable, GUI_GetSelected());

    if (current_folder && crsf_params[current_folder-1].parent == folder) {
        crsf_params[current_folder-1].child_row_idx = row_idx;     // save row when leaving child
        row_idx = crsf_params[current_folder-1].parent_row_idx;    // use row when returning to parent
    } else if (folder && current_folder != folder) {
        crsf_params[folder-1].parent_row_idx = row_idx;            // save row when entering child
        row_idx = crsf_params[folder-1].child_row_idx;             // use row when returning to child
    }

    int absrow = (row_idx >> 8) + (row_idx & 0xff);
    if (absrow >= row_count) row_idx = 0;

    current_folder = folder;

    GUI_RemoveAllObjects();
    GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, LCD_WIDTH,
                         LISTBOX_ITEMS * LINE_HEIGHT, LINE_HEIGHT,
                         row_count, row_cb, NULL, NULL, NULL);
    GUI_SetSelected(GUI_ShowScrollableRowOffset(&gui->scrollable, row_idx));
}


void PAGE_CrsfdeviceInit(int page)
{
    device_idx = page;
    crsfdevice_init();
    next_param = 1;
    if (crsf_devices[device_idx].number_of_params) {
        if (crsf_devices[device_idx].address == ADDR_RADIO)
            protocol_read_params(device_idx, crsf_params);
        else 
            CRSF_read_param(device_idx, next_param, 0);
    }

    current_folder = 255;
    show_page(0);
    PAGE_SetActionCB(action_cb);
}
#endif
