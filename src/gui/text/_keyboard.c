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

#include "char_map.h"
#include "lcd_page_props.h"

#define TEXTBOX_X_OFFSET 3
#define TEXTBOX_Y_OFFSET 3
#define TEXTBOX_HEIGHT  1*2
#define TEXTBOX_ROUND    1
#define TEXTBOX_BG_COLOR 0x0000
#define TEXTBOX_COLOR    0xFFFF
#define TEXTBOX_OUTLINE  0xFFFF

#define Y_OFFSET (5*ITEM_SPACE)
#define KEY_H (1*ITEM_SPACE)
#define KEY_W1 (2*ITEM_SPACE)
#define KEY_W2 (4*ITEM_SPACE)  // 3 chars
#define KEY_W3 (5*ITEM_SPACE)  // 4 chars

static const char caps_str[] = { LCD_UP_ARROW, 0 };
static const char del_str[]  = "DEL";
static const char mix_str[]  = "012";
static const char char_str[] = "ABC";
static const char space_str[] = "SPACE";

static void _make_box(struct guiBox *box, u16 x, u16 y, u16 width, u16 height)
{
    box->x = x;
    box->y = y;
    box->width = width + 2 ;
    box->height = height -1;
}

void _draw_key_bg(struct guiBox *box, int pressed, u32 color)
{
    (void)color;
    if (pressed) {
        LCD_PrintCharXY(box->x, box->y, LCD_SELECT_CHAR);
    } else {
        LCD_PrintCharXY(box->x, box->y, ' ');
    }
}

static void _kb_draw_text(const char *str)
{
    GUI_DrawLabelHelper(TEXTBOX_X_OFFSET, TEXTBOX_Y_OFFSET, LCD_WIDTH - TEXTBOX_X_OFFSET, 1, str, NULL, 0);
}
