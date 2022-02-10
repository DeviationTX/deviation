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
        if (param->max_value - param->min_value > 1) {
            GUI_CreateTextSelectPlate(&gui->value[relrow].ts,
                LCD_WIDTH - width - 18, y,
                width + 13, LINE_HEIGHT, &TEXTSEL_FONT,
                NULL, value_textsel, (void *)param);
        } else {
            GUI_CreateButtonPlateText(&gui->value[relrow].but,
                LCD_WIDTH - width - 18, y,
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
        GUI_CreateTextSelectPlate(&gui->value[relrow].ts, EDIT_VALUE_X, y,
            EDIT_VALUE_WIDTH, LINE_HEIGHT, &TEXTSEL_FONT,
            NULL, value_numsel, (void *)param);
        break;
    case STRING:
        GUI_CreateLabelBox(&gui->value[relrow].lbl, EDIT_VALUE_X, y,
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

    if (count_params_loaded() != crsf_devices[device_idx].number_of_params) {
        snprintf(tempstring, sizeof tempstring, "%s %s", crsf_devices[device_idx].name, _tr_noop("LOADING"));
    } else if (Model.protocol == PROTOCOL_ELRS) {
        snprintf(tempstring, sizeof tempstring, "%s  %d/%d  %c",
                 crsf_devices[device_idx].name, elrs_info.bad_pkts, elrs_info.good_pkts,
                 (elrs_info.flags & 1) ? 'C' : '-');
    } else  {
        return crsf_devices[device_idx].name;
    }
    return tempstring;
}

static void show_header() {
    GUI_Redraw(&gui->msg);
}

static void show_page(int folder) {
    GUI_RemoveAllObjects();
    GUI_CreateLabelBox(&gui->msg, 0, 0, LCD_WIDTH, HEADER_HEIGHT, &TITLE_FONT, hdr_str_cb, NULL, NULL);
    GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, LCD_WIDTH,
        LCD_HEIGHT - HEADER_HEIGHT, LINE_SPACE, folder_rows(folder),
        row_cb, NULL, NULL, NULL);
    GUI_SetSelected(GUI_ShowScrollableRowOffset(&gui->scrollable, 0));
}

void PAGE_CrsfdeviceInit(int page)
{
    device_idx = page;
    crsfdevice_init();
    current_folder = 0;
    if (crsf_devices[device_idx].number_of_params)
        CRSF_read_param(device_idx, next_param, next_chunk);

    show_page(current_folder);
    PAGE_SetActionCB(action_cb);
}
#endif
