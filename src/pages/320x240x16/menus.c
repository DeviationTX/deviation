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
#include "config/tx.h"

enum {
    NUM_X             = 0,
    NUM_WIDTH         = 16,
    MENU_X            = 20,
    MENU_WIDTH        = (LCD_WIDTH - MENU_X * 2),
};
#define HEADER_HEIGHT (LCD_HEIGHT==240 ? 37 : 40)
#define LINE_SPACE    29
#define LINE_HEIGHT   20
#include "../common/_menus.c"

static void menu_press_cb(guiObject_t *obj, s8 press_type, const void *data)
{
    (void)obj;
    if(press_type == -1) {
        long i = (long)data;
        PAGE_PushByID(i, 0);
    }
}
static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;
    int idx = 0;
    unsigned i = 0;
    while(1) {
        menu_get_next_rowidx(&i);
        if (i >= PAGEID_LAST)
            break;
        if (idx == absrow) {
            GUI_CreateLabelBox(&gui->idx[relrow], NUM_X, y,
                NUM_WIDTH, LINE_HEIGHT, &LABEL_FONT, idx_string_cb, NULL, (void *)(absrow+ 1L));
            GUI_CreateLabelBox(&gui->name[relrow], MENU_X, y,
                MENU_WIDTH, LINE_HEIGHT, &MENU_FONT, menu_name_cb, menu_press_cb, (const void *)(long)i);
            break;
        }
        idx++;
        i++;
    }
    return 1;
}
