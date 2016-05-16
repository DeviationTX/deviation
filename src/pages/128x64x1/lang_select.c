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
#include "config/tx.h"
#include <string.h>

#include "../common/_lang_select.c"

static const char *_string_cb(guiObject_t *obj, const void *data);

static struct lang_obj * const gui = &gui_objs.u.lang;

void press_cb(struct guiObject *obj, s8 press_type, const void *data)
{
    (void)obj;
    (void)press_type;
    long idx = (long)data;
    CONFIG_ReadLang(idx);
    cp->return_page(0);
}

static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;
    GUI_CreateLabelBox(&gui->label[relrow], 0, y,
            0, LINE_HEIGHT, &DEFAULT_FONT, _string_cb, press_cb, (void *)(long)absrow);
    return 1;
}

void LANGPage_Select(void(*return_page)(int page))
{
    PAGE_RemoveAllObjects();
    PAGE_SetModal(1);
    cp->return_page = return_page;

    PAGE_ShowHeader(_tr("Press ENT to change"));

    cp->total_items = 1;
    if (FS_OpenDir("language")) {
        char filename[13];
        int type;
        while((type = FS_ReadDir(filename)) != 0) {
            if (type == 1 && strncasecmp(filename, "lang", 4) == 0) {
                cp->total_items++;
            }
        }
        FS_CloseDir();
    }
    GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, LCD_WIDTH, LCD_HEIGHT - HEADER_HEIGHT,
                     LINE_SPACE, cp->total_items++, row_cb, NULL, NULL, NULL);
    GUI_SetSelected(GUI_ShowScrollableRowOffset(&gui->scrollable, Transmitter.language));
}

const char *_string_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 idx = (long)data;
    return string_cb(idx, NULL);
}
