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

#ifndef OVERRIDE_PLACEMENT
#include "common.h"
#include "pages.h"
#include "gui/gui.h"
#include "config/model.h"
#include "config/ini.h"
#include <stdlib.h>

enum {
    LABEL_WIDTH =  0,
    IMAGE_X     = 75,
    IMAGE_Y     = 20,
    IMAGE_W     = 52,
    IMAGE_H     = 36,
    SCROLL_W    = 75,
};
#endif //OVERRIDE_PLACEMENT

#include "../common/_model_loadsave.c"

static void icon_notify_cb(guiObject_t *obj)
{
    int idx = GUI_ScrollableGetObjRowOffset(&gui->scrollable, obj);
    if (idx < 0)
        return;
    int absrow = (idx >> 8) + (idx & 0xff);
    change_icon(absrow);
}

static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;
    GUI_CreateLabelBox(&gui->name[relrow], 0, y,
        LABEL_WIDTH, LINE_HEIGHT, &LISTBOX_FONT, name_cb, press_cb, (const void *)(long)absrow);
    return 0;
}

void PAGE_LoadSaveInit(int page)
{
    const char *name;
    int num_models;
    int selected;
    int width = LCD_WIDTH;
    
    memset(mp, 0, sizeof(struct model_page));  // Bug fix: must initialize this
    mp->menu_type = page;
    mp->modeltype = Model.type;
    OBJ_SET_USED(&gui->image, 0);

    selected = get_scroll_count(page);
    num_models = mp->total_items; /* set by get_scroll_count */

    if (page == SAVE_MODEL) {
        name = _tr("Press ENT to copy to");
    } else {
        name = _tr("Press ENT to load");
    }
    if (page == LOAD_ICON) {
        width = SCROLL_W;
        GUI_CreateImage(&gui->image, IMAGE_X, IMAGE_Y, IMAGE_W, IMAGE_H, mp->iconstr);
        GUI_SelectionNotify(icon_notify_cb);
    }
    PAGE_ShowHeader(name);
    GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, width, LCD_HEIGHT - HEADER_HEIGHT,
                         LINE_SPACE, num_models, row_cb, NULL, NULL, NULL);
    GUI_SetSelected(GUI_ShowScrollableRowCol(&gui->scrollable, selected, 0));
}
