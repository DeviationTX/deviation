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
#endif //OVERRIDE_PLACEMENT

#if DEBUG_WINDOW_SIZE
#include "../common/_debuglog_page.c"

static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;
    GUI_CreateLabelBox(&gui->line[relrow], 0, y, LCD_WIDTH - ARROW_WIDTH, LINE_HEIGHT, &LIST_FONT, str_cb, NULL, (void *)(long)absrow);
    return 0;
}

void PAGE_DebuglogInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    PAGE_RemoveAllObjects();
    PAGE_ShowHeader(PAGE_GetName(PAGEID_DEBUGLOG));

    find_line_ends();
    GUI_CreateScrollable(&gui->scrollable,
         0, HEADER_HEIGHT, LCD_WIDTH, LCD_HEIGHT - HEADER_HEIGHT, LINE_SPACE, NUM_ROWS, row_cb, NULL, NULL, NULL);
}
#endif //DEBUG_WINDOW_SIZE
