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
    LABEL_X        = 0,
    LABEL_WIDTH    = 55,
    TEXTSEL_X      = 55,
    TEXTSEL_WIDTH  = 65,
    RESET_X        = 58,
    RESET_WIDTH    = 59,
    START_WIDTH    = 50,
};
#endif //OVERRIDE_PLACEMENT

#if HAS_MUSIC_CONFIG

#include "../common/_musicconfig_page.c"

static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;
    (void)relrow;
    u8 space = LINE_SPACE;
    //Row 1
    GUI_CreateLabelBox(&gui->name, LABEL_X, y,
            LABEL_WIDTH, LINE_HEIGHT, &DEFAULT_FONT, musicconfig_str_cb, NULL, (void *)(long)absrow);
    //GUI_CreateTextSelectPlate(&gui->type, TEXTSEL_X, y,
    //        TEXTSEL_WIDTH, LINE_HEIGHT, &DEFAULT_FONT, NULL, set_timertype_cb, (void *)(long)absrow);
    
}

void PAGE_MusicconfigInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    PAGE_RemoveAllObjects();
    PAGE_ShowHeader(PAGE_GetName(PAGEID_MUSICCFG));
    GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, LCD_WIDTH, LCD_HEIGHT - HEADER_HEIGHT,
                     LCD_HEIGHT - HEADER_HEIGHT, NUM_TIMERS, row_cb, NULL, NULL, NULL);
    PAGE_SetScrollable(&gui->scrollable, &current_selected);

}
#endif
