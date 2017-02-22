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
    LABELNUM_X        = 0,
    LABELNUM_WIDTH    = 16,
    LABEL_X           = 20,
    LABEL_WIDTH       = 0,
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
            GUI_CreateLabelBox(&gui->idx[relrow], LABELNUM_X, y,
                LABELNUM_WIDTH, LINE_HEIGHT,  &DEFAULT_FONT, idx_string_cb, NULL, (void *)(absrow+ 1L));
            GUI_CreateLabelBox(&gui->name[relrow], LABEL_X, y,
                LCD_WIDTH - LABEL_X * 2, LINE_HEIGHT, &MENU_FONT, menu_name_cb, menu_press_cb, (const void *)(long)i);
            break;
        }
        idx++;
        i++;
    }
    return 1;
}
