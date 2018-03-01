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

#if DEBUG_WINDOW_SIZE
#include "../common/_debuglog_page.c"


static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;
    struct LabelDesc font = DEFAULT_FONT;
    font.style = LABEL_LEFT;    
    GUI_CreateLabelBox(&gui->line[relrow], 5, y, LCD_WIDTH - ARROW_WIDTH - 5, 16, &font, str_cb, NULL, (void *)(long)absrow);
    return 0;
}

void PAGE_DebuglogInit(int page)
{
    (void)page;
    const int ROW_HEIGHT = 20;
    PAGE_ShowHeader(PAGE_GetName(PAGEID_DEBUGLOG));
    find_line_ends();
    GUI_CreateScrollable(&gui->scrollable,
         0, 40, LCD_WIDTH, LCD_HEIGHT - 40, ROW_HEIGHT, NUM_ROWS, row_cb, getobj_cb, NULL, NULL);
}
#endif //DEBUG_WINDOW_SIZE
