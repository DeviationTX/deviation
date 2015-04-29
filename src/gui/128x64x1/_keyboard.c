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

#define TEXTBOX_X_OFFSET 1
#define TEXTBOX_Y_OFFSET 1
#define TEXTBOX_HEIGHT  11 
#define TEXTBOX_ROUND    1
#define TEXTBOX_BG_COLOR 0x0000
#define TEXTBOX_COLOR    0xFFFF
#define TEXTBOX_OUTLINE  0xFFFF

#define Y_OFFSET 14
#define KEY_H 12
#define KEY_W1 11  // TINY_FONT'S max size is actually 7x6 instead of 7x5
#define KEY_W2 (KEY_W1 + 12)  // 3 chars
#define KEY_W3 (KEY_W1 + 18)  // 4 chars

static const char caps_str[] = "CAP";
static const char del_str[]  = "DEL";
static const char mix_str[]  = ".?12";
static const char char_str[] = "ABC";
static const char space_str[] = " ";

static void _make_box(struct guiBox *box, u16 x, u16 y, u16 width, u16 height)
{
    box->x = x;
    box->y = y;
    box->width = width -1 ;
    box->height = height -1;
}

void _draw_key_bg(struct guiBox *box, int pressed, u32 color)
{
    (void)color;
    if (pressed) {
        LCD_FillRoundRect(box->x, box->y, box->width, box->height, 1, 1);
    } else {
        LCD_FillRoundRect(box->x, box->y, box->width, box->height, 1, 0);
        LCD_DrawRoundRect(box->x, box->y, box->width, box->height, 1, 1);
    }
}
