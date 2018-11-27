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
#include "gui/font.h"

//#define LINE_SPACING 2 // move to _gui.h as devo10's line spacing is different from devo8's
#define CHAR_SPACING 1

static struct {
    u8 font_idx;
    unsigned int x_start;
    unsigned int x;
    unsigned int y;
    u16          color;
} cur_str;

#define NUM_FONTS 10
#define FONT_NAME_LEN 9
char FontNames[NUM_FONTS][FONT_NAME_LEN];

void LCD_PrintCharXY(unsigned int x, unsigned int y, u32 c)
{
    u8 row, col, width;
    u8 font[MAX_FONTSIZE];
    const u8* offset = &font[0];
    u8 length = FONT_ReadCharBits(font, c, &width);
    if ( length == 0 || ! width) {
        printf("Could not locate character U-%04x\n", (int)c);
        return;
    }
    // Check if the requested character is available
    LCD_DrawStart(x, y, x + width - 1,  y + FONT_GetHeight() - 1, DRAW_NWSE);
    for (col = 0; col < width; col++)
    {
        const u8 *data = offset++;
        u8 bit = 0;
        // Data is right aligned,adrawn top to bottom
        for (row = 0; row < FONT_GetHeight(); ++row)
        {
            if (bit == 8) {
                data = offset++;
                bit = 0;
            }
            if (*data & (1 << bit)) {
                LCD_DrawPixelXY(x + col, y + row, cur_str.color);
            }
            bit++;
        }
    }
    LCD_DrawStop();
}

u8 LCD_SetFont(unsigned int idx)
{
    u8 old = LCD_GetFont();
    if (old == idx)
        return old;
    if (! FONT_Open(idx, FontNames[idx]))
        FONT_Open(old, FontNames[old]);
    return old;
}

u8 LCD_GetFontFromString(const char *value)
{
    int i;
    for (i = 0; i < NUM_FONTS; i++) {
        if (FontNames[i][0] == 0) {
            strlcpy(FontNames[i], value, FONT_NAME_LEN);
            return i + 1;
        }
        if (strcasecmp(FontNames[i], value) == 0) {
            return i + 1;
        }
    }
    printf("Unknown font: %s\n", value);
    return -1;
}

u8 LCD_GetFont()
{
    return cur_str.font_idx; 
}

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
        cur_str.x = cur_str.x_start;
        cur_str.y += FONT_GetHeight() + LINE_SPACING;
    } else {
        LCD_PrintCharXY(cur_str.x, cur_str.y, c);
        cur_str.x += FONT_GetWidth(c) + CHAR_SPACING;
    }
}

void LCD_GetCharDimensions(u32 c, u16 *width, u16 *height) {
    *height = FONT_GetHeight();
    *width = FONT_GetWidth(c);
}

void LCD_GetStringDimensions(const u8 *str, u16 *width, u16 *height) {
    int line_width = 0;
    *height = FONT_GetHeight();
    *width = 0;
    //printf("String: %s\n", str);
    while(*str) {
        u32 ch;
        str = (const u8 *)utf8_to_u32((const char *)str, &ch);
        if(ch == '\n') {
            *height += FONT_GetHeight() + LINE_SPACING;
            if(line_width > *width)
                *width = line_width;
            line_width = 0;
        } else {
            line_width += FONT_GetWidth(ch) + CHAR_SPACING;
        }
    }
    if(line_width > *width)
        *width = line_width;
    //printf("W: %d   H: %d\n",(int)*width,(int)*height);
}

void LCD_SetFontColor(u16 color) {
    cur_str.color = color;
}
