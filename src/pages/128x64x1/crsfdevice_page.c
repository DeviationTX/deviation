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

#include "../common/_crsfdevice_page.c"


static int folder_rows(int folder) {
    int count = 0;
    crsf_param_t *param = crsf_params;

    while (param->id) {
        if (param->parent == folder && !param->hidden) count += 1;
        param += 1;
    }
    return count;
}

void button_press(guiObject_t *obj, const void *data)
{
    (void)obj;
    crsf_param_t *param = (crsf_param_t *)data;

    if (param->u.text_sel == param->min_value)
        param->u.text_sel = param->max_value;
    else
        param->u.text_sel = param->min_value;

    CRSF_set_param(param, param->u.text_sel);
}

static void command_press(guiObject_t *obj, s8 press_type, const void *data)
{
    (void)obj;

    if (press_type != -1) {
        return;
    }

    crsf_param_t *param = (crsf_param_t *)data;

    if (param->u.status == READY) {
        CRSF_set_param(param, START);
    }
}

static const char *value_textsel(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    crsf_param_t *param = (crsf_param_t *)data;
    u8 changed = 0;

    param->u.text_sel = GUI_TextSelectHelper(param->u.text_sel,
                            param->min_value, param->max_value,
                            dir, 1, 1, &changed);

    if (changed) CRSF_set_param(param, param->u.text_sel);
    return current_text(param);
}

static void press_cb(struct guiObject *obj, s8 press_type, const void *data)
{
    (void)obj;
    if (press_type != -1) {
        return;
    }

    crsf_param_t *param = (crsf_param_t *)data;

    switch (param->type) {
    case FOLDER:
        current_folder = param->id;
        show_page(current_folder);
        break;
    case UINT8:
    case UINT16:
    case FLOAT:
    case INT8:
    case INT16:
    case STRING:
    case INFO:
    case OUT_OF_RANGE:
    default:
        break;
    }
}

static int row_cb(int absrow, int relrow, int y, void *data) {
    (void)data;

    crsf_param_t *param = current_param(absrow);

    switch (param->type) {
    case TEXT_SELECTION:
        GUI_CreateLabelBox(&gui->name[relrow], LABEL_X, y,
            EDIT_LABEL_WIDTH, LINE_HEIGHT, &LABEL_FONT,
            crsf_name_cb, NULL, (void *)param);
        if (param->max_value - param->min_value > 1) {
            GUI_CreateTextSelectPlate(&gui->value[relrow].ts, EDIT_VALUE_X, y,
                EDIT_VALUE_WIDTH, LINE_HEIGHT, &TEXTSEL_FONT,
                NULL, value_textsel, (void *)param);
        } else {
            GUI_CreateButtonPlateText(&gui->value[relrow].but, EDIT_VALUE_X, y,
                EDIT_VALUE_WIDTH, LINE_HEIGHT, &BUTTON_FONT,
                crsf_value_cb, button_press, (void *)param);
        }
        break;
    case COMMAND:
        GUI_CreateLabelBox(&gui->name[relrow], LABEL_X, y,
            LABEL_WIDTH, LINE_HEIGHT, &LABEL_FONT,
            crsf_name_cb, command_press, (void *)param);
        break;
    case INFO:
        GUI_CreateLabelBox(&gui->name[relrow], LABEL_X, y,
            EDIT_LABEL_WIDTH, LINE_HEIGHT, &LABEL_FONT,
            crsf_name_cb, NULL, (void *)param);
        GUI_CreateLabelBox(&gui->value[relrow].lbl, EDIT_VALUE_X, y,
            EDIT_VALUE_WIDTH, LINE_HEIGHT, &LABEL_FONT,
            crsf_value_cb, NULL, (void *)param);
        break;
    case FOLDER:
        GUI_CreateLabelBox(&gui->name[relrow], LABEL_X, y,
            LABEL_WIDTH, LINE_HEIGHT, &LABEL_FONT, crsf_name_cb, press_cb, (void *)param);
        break;

    // TODO(Hexfet)  implement
    case UINT8:
    case UINT16:
    case FLOAT:
    case INT8:
    case INT16:
    case STRING:
    case OUT_OF_RANGE:
    default:
        GUI_CreateLabelBox(&gui->value[relrow].lbl, LABEL_X, y,
            EDIT_LABEL_WIDTH, LINE_HEIGHT, &LABEL_FONT, crsf_name_cb, press_cb, (void *)param);
        break;
    }
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
    PAGE_ShowHeader(crsf_devices[device_idx].name);
    GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, LCD_WIDTH,
        LCD_HEIGHT - HEADER_HEIGHT, LINE_SPACE, folder_rows(folder),
        row_cb, NULL, NULL, NULL);
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
