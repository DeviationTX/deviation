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
#include "gui/gui.h"
#include "lcd.h"

struct FAT FontFAT = {{0}};

void LCD_SetXY(unsigned int x, unsigned int y)
{
    cur_str.x_start = x;
    cur_str.x = x;
    cur_str.y = y;
}

void LCD_PrintStringXY(unsigned int x, unsigned int y, const char *str)
{
    LCD_SetXY(x, y);
    LCD_PrintString(str);
}

void LCD_PrintString(const char *str)
{
    while(*str != 0) {
        u32 ch;
        str = utf8_to_u32(str, &ch);
        LCD_PrintChar(ch);
    }
}

void LCD_PrintChar(u32 c)
{
    if(c == '\n') {
        // New line
        cur_str.x = cur_str.x_start;
        cur_str.y++;
    } else {
        LCD_PrintCharXY(cur_str.x, cur_str.y, c);
        cur_str.x++;
    }
}

void LCD_GetCharDimensions(u32 c, u16 *width, u16 *height) {
    (void) c;
    *height = 1;
    *width = 1;
}

void LCD_GetStringDimensions(const u8 *str, u16 *width, u16 *height) {
    u16 width_t, height_t;

    height_t = 0;
    width_t = 0;
    while(*str != 0) {
        u32 ch;
        str = (u8*)utf8_to_u32((char*)str, &ch);
        if(ch == '\n')
            height_t++;
        width_t++;
    }

    *width = width_t;
    *height = height_t;
}

void LCD_SetFontColor(u16 color) {
    cur_str.color = color;
}
