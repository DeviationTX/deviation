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
static struct lang_obj * const gui = &gui_objs.u.lang;

static int row_cb(int absrow, int relrow, int y, void *data)
{
    int row_count = (long)data;
    if (absrow >= row_count) {
        GUI_CreateLabelBox(&gui->label[relrow], LCD_WIDTH/2-100, y,
                200 - ARROW_WIDTH, LINE_HEIGHT, &LISTBOX_FONT, NULL, NULL, "");
    } else {
        GUI_CreateLabelBox(&gui->label[relrow], LCD_WIDTH/2-100, y,
                200 - ARROW_WIDTH, LINE_HEIGHT, &LISTBOX_FONT, string_cb, press_cb, (void *)(long)absrow);
    }
    return 1;
}

void PAGE_LanguageInit(int page)
{
    (void)page;
    int num_lang = count_num_languages();
    PAGE_ShowHeader(PAGE_GetName(PAGEID_LANGUAGE));

    int min_rows = num_lang >= LISTBOX_ITEMS ? num_lang : LISTBOX_ITEMS;
    GUI_CreateScrollable(&gui->scrollable, LCD_WIDTH/2-100, 40, 200, LISTBOX_ITEMS * LINE_HEIGHT, LINE_HEIGHT, min_rows, row_cb, NULL, NULL, (void *)(long)num_lang);
    GUI_SetSelected(GUI_ShowScrollableRowCol(&gui->scrollable, Transmitter.language, 0));
}
