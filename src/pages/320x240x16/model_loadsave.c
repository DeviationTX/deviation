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
#include "config/model.h"
#include "config/ini.h"
#include <stdlib.h>

#include "../common/_model_loadsave.c"

static u8 list_selected = 0;
static void icon_notify_cb(guiObject_t *obj)
{
    int idx = GUI_ScrollableGetObjRowOffset(&gui->scrollable, obj);
    if (idx < 0)
        return;
    int absrow = (idx >> 8) + (idx & 0xff);
    list_selected = absrow;
    change_icon(absrow);
}

static void ok_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    press_cb(NULL, -1, (void *)(long)list_selected);
}
static void press1_cb(guiObject_t *obj, s8 press_type, const void *data)
{
    list_selected = (long)data;
    if (HAS_TOUCH && OBJ_IS_USED(&gui->image)) {
        //differentiate between touch and button
        static int is_touch = 0;
        if (press_type >= 0) {
            u32 buttons = ScanButtons();
            is_touch = 0;
            if(! CHAN_ButtonIsPressed(buttons, BUT_ENTER)) {
                is_touch = 1;
            }
            return;
        }
        if (press_type == -1 && is_touch) {
            is_touch = -1;
            icon_notify_cb(obj);
            return;
        }
    }
    press_cb(obj, press_type, data);
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
    if (absrow >= mp->total_items) {
        GUI_CreateLabelBox(&gui->label[relrow], 8 + ((LCD_WIDTH - 320) / 2), y,
            200 - ARROW_WIDTH, 24, &listbox, NULL, NULL, "");
    } else {
        GUI_CreateLabelBox(&gui->label[relrow], 8 + ((LCD_WIDTH - 320) / 2), y,
            200 - ARROW_WIDTH, 24, &listbox, name_cb, press1_cb, (void *)(long)absrow);
    }
    return 0;
}
void PAGE_LoadSaveInit(int page)
{
    int num_models;
    int selected;
    const char * name = NULL;
    enum loadSaveType menu_type = page;
    mp->menu_type = page;
    OBJ_SET_USED(&gui->image, 0);

    selected = get_scroll_count(page);

    switch(menu_type) {
      case LOAD_MODEL:      name = _tr("Load Model"); break;
      case SAVE_MODEL:      name = _tr("Save Model as..."); break;
      case LOAD_TEMPLATE:   name = _tr("Load Model Template"); break;
      case LOAD_ICON:       name = _tr("Select Icon"); break;
      case LOAD_LAYOUT:     name = _tr("Load Layout"); break;
    }
    //PAGE_ShowHeader(name);

    num_models = mp->total_items;
    if (num_models < LISTBOX_ITEMS)
        num_models = LISTBOX_ITEMS;
    if (page != LOAD_TEMPLATE && page != LOAD_LAYOUT) {
        PAGE_ShowHeaderWithSize(name, LCD_WIDTH - 88, 0);
        u16 w = 0, h = 0;
        char *img = mp->iconstr;
        if(! fexists(img))
            img = UNKNOWN_ICON;
        LCD_ImageDimensions(img, &w, &h);
        GUI_CreateImage(&gui->image, 212 + ((LCD_WIDTH - 320) / 2), 88, w, h, mp->iconstr);
        PAGE_CreateOkButton(LCD_WIDTH - 48, 4, ok_cb);
        GUI_SelectionNotify(icon_notify_cb);
    }
    else
        PAGE_ShowHeader(name);

    GUI_CreateScrollable(&gui->scrollable, 8 + ((LCD_WIDTH - 320) / 2), 40, 200, LISTBOX_ITEMS * 24,
                         24, num_models, row_cb, NULL, NULL, NULL);
    GUI_SetSelected(GUI_ShowScrollableRowCol(&gui->scrollable, selected, 0));
    list_selected = selected;
}
