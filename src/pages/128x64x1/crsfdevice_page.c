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
    ROW_SPLIT_WIDTH  = 105,
};
#endif

#if SUPPORT_CRSF_CONFIG

#include "../common/_crsfdevice_page.c"

static int gui_lines(crsf_param_t *param) {
    u16 label_width, value_width=0, _unused;

    LCD_GetStringDimensions((const u8 *)param->name, &label_width, &_unused);

    const u8 *max_string = (const u8 *)tempstring;
    switch (param->type) {
    case TEXT_SELECTION:
        LCD_GetStringDimensions((const u8 *)param->max_str, &value_width, &_unused);
        snprintf(tempstring, sizeof tempstring, "%s", param->max_str);
        break;
    case UINT8:
    case UINT16:
    case INT8:
    case INT16:
        snprintf(tempstring, sizeof tempstring, "%d", param->max_value);
        LCD_GetStringDimensions((const u8 *)tempstring, &value_width, &_unused);
        break;
    case FLOAT:
        snprintf(tempstring, sizeof tempstring, "%f", param->max_value);
        LCD_GetStringDimensions((const u8 *)tempstring, &value_width, &_unused);
        break;
    case STRING:
        snprintf(tempstring, sizeof tempstring, "%s*", "", param->u.string_max_len);
        LCD_GetStringDimensions((const u8 *)tempstring, &value_width, &_unused);
        break;
    case COMMAND:
    case INFO:
    case FOLDER:
    case OUT_OF_RANGE:
        max_string = NULL;
        break;
    }

    if (max_string) LCD_GetStringDimensions((const u8 *)tempstring, &value_width, &_unused);
    param->lines_per_row = (value_width && (label_width + value_width > ROW_SPLIT_WIDTH)) ? 2 : 1;
    return (int)param->lines_per_row;
}

static int size_cb(int absrow, void *data) {
    (void)data;
    crsf_param_t *param = current_param(absrow);

    return gui_lines(param);
}

static int row_cb(int absrow, int relrow, int y, void *data) {
    (void)data;
    u16 width, height;

    crsf_param_t *param = current_param(absrow);
    if (!param) return 0;
    void (*lbl_press_cb)(struct guiObject *obj, s8 press_type, const void *data) = NULL;

    // draw name first so value on top
    switch (param->type) {
        case COMMAND: lbl_press_cb = command_press; break;
        case FOLDER: lbl_press_cb = folder_cb; break;
        case INFO: lbl_press_cb = noop_press; break;
        default: break;
    }
    GUI_CreateLabelBox(&gui->name[relrow], LABEL_X, y,
        LABEL_WIDTH, LINE_HEIGHT, &LABEL_FONT,
        crsf_name_cb, lbl_press_cb, (void *)param);


    switch (param->type) {
    case TEXT_SELECTION:
        LCD_GetStringDimensions((const u8 *)param->max_str, &width, &height);
        if (non_null_values(param) > 1 && (param->max_value - param->min_value > 1)) {
            GUI_CreateTextSelectPlate(&gui->value[relrow].ts,
                LCD_WIDTH - width - 18, y + (gui_lines(param) - 1) * LINE_SPACE,
                width + 13, LINE_HEIGHT, &TEXTSEL_FONT,
                NULL, value_textsel, (void *)param);
        } else {
            GUI_CreateButtonPlateText(&gui->value[relrow].but,
                LCD_WIDTH - width - 18, y + (gui_lines(param) - 1) * LINE_SPACE,
                width + 13, LINE_HEIGHT, &BUTTON_FONT,
                crsf_value_cb, button_press, (void *)param);
        }
        break;
    case COMMAND:
        lbl_press_cb = command_press;
        break;
    case INFO:
        // value combined with selectable name to
        // workaround issue with scrolling non-selectable items
        break;
    case FOLDER:
        lbl_press_cb = folder_cb;
        break;
    case UINT8:
    case UINT16:
    case INT8:
    case INT16:
    case FLOAT:
        GUI_CreateTextSelectPlate(&gui->value[relrow].ts, EDIT_VALUE_X, y + (gui_lines(param) - 1) * LINE_SPACE,
            EDIT_VALUE_WIDTH, LINE_HEIGHT, &TEXTSEL_FONT,
            NULL, value_numsel, (void *)param);
        break;
    case STRING:
        GUI_CreateLabelBox(&gui->value[relrow].lbl, EDIT_VALUE_X, y + (gui_lines(param) - 1) * LINE_SPACE,
            EDIT_VALUE_WIDTH, LINE_HEIGHT, &LABEL_FONT,
            crsf_value_cb, stredit_cb, (void *)param);
        break;
    case OUT_OF_RANGE:
        break;
    }

    return 1;
}

static const char *hdr_str_cb(guiObject_t *obj, const void *data) {
    (void)obj;
    (void)data;

    int params_left = params_needed();
    if (params_left != 0) {
        char short_name[10];
        strlcpy(short_name, crsf_devices[device_idx].name, 10);
        snprintf(tempstring, sizeof tempstring, "%s %s %d", short_name, _tr_noop("LOADING"), params_left);
    } else {
        if (protocol_module_is_elrs(3)) {
            if (protocol_elrs_is_armed()) {
                snprintf(tempstring, sizeof tempstring, "%s  ! Armed !", crsf_devices[device_idx].name);
            } else if (elrs_info.flag_info[0]) {
                snprintf(tempstring, sizeof tempstring, "%s", elrs_info.flag_info);
            } else {
                snprintf(tempstring, sizeof tempstring, "%s  %d/%d  %c",
                         crsf_devices[device_idx].name, elrs_info.bad_pkts, elrs_info.good_pkts,
                         (elrs_info.flags & FLAG_CONN) ? 'C' : '-');
            }
        } else {
            return crsf_devices[device_idx].name;
        }
    }
    return tempstring;
}

static void show_header() {
    GUI_Redraw(&gui->msg);
}

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
    GUI_CreateLabelBox(&gui->msg, 0, 0, LCD_WIDTH, HEADER_HEIGHT, &TITLE_FONT, hdr_str_cb, NULL, NULL);
    GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, LCD_WIDTH,
        LCD_HEIGHT - HEADER_HEIGHT, LINE_SPACE, row_count,
        row_cb, NULL, size_cb, NULL);
    GUI_SetSelected(GUI_ShowScrollableRowOffset(&gui->scrollable, row_idx));
}

void PAGE_CrsfdeviceInit(int page)
{
    device_idx = page;
    crsfdevice_init();
    next_param = 1;
    if (crsf_devices[device_idx].number_of_params)
        CRSF_read_param(device_idx, next_param, 0);

    current_folder = 255;
    show_page(0);
    PAGE_SetActionCB(action_cb);
}
#endif
