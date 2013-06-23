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

#define TEXTBOX_X_OFFSET 5
#define TEXTBOX_Y_OFFSET (2 + (LCD_HEIGHT - 240) / 2)
#define TEXTBOX_HEIGHT   24
#define TEXTBOX_ROUND    3
#define TEXTBOX_BG_COLOR 0xFFFF
#define TEXTBOX_COLOR    0x0000
#define TEXTBOX_OUTLINE  0x0000

#define Y_OFFSET (TEXTBOX_X_OFFSET + TEXTBOX_HEIGHT + 1 + (LCD_HEIGHT - 240) / 2)
#define KEY_H 52
#define KEY_W1 32
#define KEY_W2 (32 + 12)
#define KEY_W3 (32 + 18)

static const char caps_str[] = "CAPS";
static const char del_str[]  = "DEL";
static const char mix_str[]  = ".?123";
static const char char_str[] = "ABC";

static void _make_box(struct guiBox *box, u16 x, u16 y, u16 width, u16 height)
{
    #define X_SPACE 3
    #define Y_SPACE 7
    box->x = x + X_SPACE;
    box->y = y + Y_SPACE;
    box->width = width - 2 * X_SPACE;
    box->height = height - 2 * Y_SPACE;
}

void _draw_key_bg(struct guiBox *box, int pressed, u32 color)
{
    (void)pressed;
    LCD_FillRoundRect(box->x, box->y, box->width, box->height, 3, color);
}
