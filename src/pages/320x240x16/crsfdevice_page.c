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

static void folder_cb(struct guiObject *obj, s8 press_type, const void *data)
{
    (void)obj;
    if (press_type != -1) {
        return;
    }

    crsf_param_t *param = (crsf_param_t *)data;

    current_folder = param->id;
    show_page(current_folder);
}

static int row_cb(int absrow, int relrow, int y, void *data) {
    (void)data;

    crsf_param_t *param = current_param(absrow);
    void (*lbl_press_cb)(struct guiObject *obj, s8 press_type, const void *data) = NULL;

    switch (param->type) {
    case TEXT_SELECTION:
        if (param->max_value - param->min_value > 1) {
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
            199, LINE_HEIGHT, &LABEL_FONT, crsf_value_cb, NULL, (void *)param);
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

static crsf_param_t *param_by_id(int id) {
    crsf_param_t *param = crsf_params;

    while (param->id) {
        if (param->id == id)
            return param;
        param++;
    }
    return NULL;
}

static unsigned action_cb(u32 button, unsigned flags, void *data);

void show_page(int folder) {
    GUI_RemoveAllObjects();
    if (count_params_loaded() == crsf_devices[device_idx].number_of_params) {
        PAGE_ShowHeader(crsf_devices[device_idx].name);
    } else {
        snprintf(tempstring, sizeof tempstring, "%s %s", crsf_devices[device_idx].name, _tr_noop("LOADING"));
        PAGE_ShowHeader(tempstring);
    }
    GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, LCD_WIDTH,
                     LISTBOX_ITEMS * LINE_HEIGHT, LINE_HEIGHT,
                     folder_rows(folder), row_cb, NULL, NULL, NULL);
    GUI_SetSelected(GUI_ShowScrollableRowOffset(&gui->scrollable, current_selected));
}

static unsigned action_cb(u32 button, unsigned flags, void *data)
{
    if (current_folder != 0 && CHAN_ButtonIsPressed(button, BUT_EXIT)) {
        if (flags & BUTTON_RELEASE) {
            current_folder = param_by_id(current_folder)->parent;
            show_page(current_folder);
        }
        return 1;
    }
    return default_button_action_cb(button, flags, data);
}

void PAGE_CrsfdeviceInit(int page)
{
    device_idx = page;
#ifdef EMULATOR
    crsf_params = page == 0 ? tx_crsf_params : rx_crsf_params;
    params_loaded = crsf_devices[device_idx].number_of_params;
#endif
    crsfdevice_init();
    current_selected = 0;
    current_folder = 0;
    CRSF_read_param(device_idx, next_param, next_chunk);

    show_page(current_folder);
    PAGE_SetActionCB(action_cb);
}
#endif
