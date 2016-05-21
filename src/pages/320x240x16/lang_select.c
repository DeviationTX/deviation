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
#define LINE_HEIGHT 24
static struct lang_obj * const gui = &gui_objs.u.lang;

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
    GUI_CreateLabelBox(&gui->label[relrow], LCD_WIDTH/2-100, y,
            200 - ARROW_WIDTH, LINE_HEIGHT, &listbox, string_cb, press_cb, (void *)(long)absrow);
    return 1;
}

void PAGE_LanguageInit(int page)
{
    (void)page;
    int num_lang = count_num_languages;
    PAGE_ShowHeader(_tr(PAGE_GetName(PAGEID_LANGUAGE)));

    GUI_CreateScrollable(&gui->scrollable, LCD_WIDTH/2-100, 40, 200, LCD_HEIGHT-48, LINE_HEIGHT, num_lang, row_cb, NULL, NULL, NULL);
    GUI_SetSelected(GUI_ShowScrollableRowCol(&gui->scrollable, Transmitter.language, 0));
}
