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

#define OVERRIDE_PLACEMENT
#include "common.h"
#include "pages.h"
#include "gui/gui.h"
#include "config/model.h"

#define SEPARATOR 0

enum {
    NUM_COLS      = 7,
    REVERT_X      = LCD_WIDTH - 7*ITEM_SPACE,
    REVERT_W      = 7*ITEM_SPACE ,
    ROW_Y         = 4,
    ROW_INCREMENT = 2,
    LABEL_X       = 0,
    LABEL_W       = 2*ITEM_SPACE,
    LABEL_H       = ITEM_SPACE,
    ICON_X        = 3*ITEM_SPACE,
    ICON_W        = ITEM_SPACE,
    SCROLLABLE_X  = 7*ITEM_SPACE,
    SCROLLABLE_Y  = 4*ITEM_SPACE,
    SCROLLABLE_H  = 8*ITEM_SPACE,
    SCROLL_ROW_H  = 2*ITEM_SPACE,
};

static struct toggleselect_obj   * const gui = &gui_objs.u.toggleselect;
static struct toggle_select_page * const tp  = &pagemem.u.toggle_select_page;

#include "../common/_toggle_select.c"

const char *TGLICO_font_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    int idx = (long)data  & 0xff;
    tempstring[0] = 0xCC | (idx >> 6);
    tempstring[1] = 0x80 | (idx & 0x3f);
    tempstring[2] = 0;
    return tempstring;
}

static int row_cb(int absrow, int relrow, int y, void *data)
{
    int SelectedIcon = (long)data;
    int i;
    int count = get_toggle_icon_count();
    int cols = count - absrow * NUM_COLS;
    if (cols > NUM_COLS)
        cols = NUM_COLS;
    for(i = 0; i < cols; i++) {
        int x = 12 + i * 6;
        GUI_CreateLabelBox(&gui->symbolicon[relrow * NUM_COLS + i], x, y, 4, 2,
                           &DEFAULT_FONT, TGLICO_font_cb, tglico_select_cb, (void*)(long)((SelectedIcon << 8) | (absrow*NUM_COLS+i)));
    }
    return i;
}
#if HAS_MAPPED_GFX
void TGLICO_LoadFonts()
{
    static int loaded = 0;
    if (loaded)
        return;
    loaded = 1;
    int count = get_toggle_icon_count();
    int idx = 0;
    for (int i = 0; i < count; i++) {
        struct ImageMap img;
        img = TGLICO_GetImage(idx);
        LCD_LoadFont(i, img.file, img.x_off, img.y_off, TOGGLEICON_WIDTH, TOGGLEICON_HEIGHT);
        idx = get_next_icon(idx);
    }
}
#else
static int get_next_icon(int idx) _UNUSED;
#endif

#include "../128x64x1/toggle_select.c"

