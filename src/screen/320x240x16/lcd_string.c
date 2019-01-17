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
#include "config/font.h"

/*
 * The font 'font_table' begins with a list of u24 values which represent
 * the offeset (from the beginning of the font file) of each character.
 * The font data follows immediately afterwards.
 *
 * The font data is represented as a bit-field.  A chunk of 1, 2, 3, or 4
 * bytes represents a single column of pixels.  This will be repeated for
 * the width of the font.
 * The column chunk can be thought of as a little-endian number of 8, 16, 24,
 * 32, 40, 48, 54 or 64 bits, with the low-order bit representing the top row,
 * and the 'height'-th bit representing the bottom row.
 *
 * Example: Here is a '!' as a 1x10 bit-field:
 *        *    1
 *        *    1
 *        *    1
 *        *    1
 *        *    1
 *        *    1
 *        *    1
 *             0   = 0x7F
 *        *    1
 *        *    1   = 0x03
 *
 *  So this would appear as '0x7F, 0x03' in the font table
 *
 */
//#define LINE_SPACING 2 // move to _gui.h as devo10's line spacing is different from devo8's
#define CHAR_SPACING 1
#define NUM_FONTS 10

#define FONT_NAME_LEN 9
char FontNames[NUM_FONTS][FONT_NAME_LEN];

static struct {
    u8 font_idx;
    unsigned int x_start;
    unsigned int x;
    unsigned int y;
    u16          color;
} cur_str;


void LCD_PrintCharXY(unsigned int x, unsigned int y, u32 c)
{
    u8 row, col, width;
    u8 font[CHAR_BUF_SIZE];
    const u8 *offset;
    offset = font;
    char_read(font, c, &width);
    if (! offset || ! width) {
        printf("Could not locate character U-%04x\n", (int)c);
        return;
    }
    // Check if the requested character is available
    LCD_DrawStart(x, y, x + width - 1,  y + get_height() - 1, DRAW_NWSE);
    for (col = 0; col < width; col++)
    {
        const u8 *data = offset++;
        u8 bit = 0;
        // Data is right aligned,adrawn top to bottom
        for (row = 0; row < get_height(); ++row)
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

u8 FONT_GetFromString(const char *value)
{
    int i;
    for (i = 0; i < NUM_FONTS; i++) {
        if (FontNames[i][0] == 0) {
            strlcpy(FontNames[i], value, FONT_NAME_LEN);
            return i + 1;
        }
        if(strcasecmp(FontNames[i], value) == 0) {
            return i + 1;
        }
    }
    printf("Unknown font: %s\n", value);
    return 0;
}

u8 LCD_SetFont(unsigned int idx)
{
    u8 old = LCD_GetFont();
    if (old == idx)
        return old;
    if (idx == 0) {
        close_font();
        cur_str.font_idx = 0;
        return 0;
    } else if (! open_font(FontNames[idx - 1])){
        if (old == 0)
            close_font();
        else
            open_font(FontNames[old - 1]);
    }
    else
        cur_str.font_idx = idx;
    return old;
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
        cur_str.y += get_height() + LINE_SPACING;
    } else {
        LCD_PrintCharXY(cur_str.x, cur_str.y, c);
        cur_str.x += get_width(c) + CHAR_SPACING;
    }
}

void LCD_GetCharDimensions(u32 c, u16 *width, u16 *height) {
    *height = get_height();
    *width = get_width(c);
}

void LCD_GetStringDimensions(const u8 *str, u16 *width, u16 *height) {
    int line_width = 0;
    *height = get_height();
    *width = 0;
    //printf("String: %s\n", str);
    while(*str) {
        u32 ch;
        str = (const u8 *)utf8_to_u32((const char *)str, &ch);
        if(ch == '\n') {
            *height += get_height() + LINE_SPACING;
            if(line_width > *width)
                *width = line_width;
            line_width = 0;
        } else {
            line_width += get_width(ch) + CHAR_SPACING;
        }
    }
    if(line_width > *width)
        *width = line_width;
    //printf("W: %d   H: %d\n",(int)*width,(int)*height);
}

void LCD_SetFontColor(u16 color) {
    cur_str.color = color;
}

#define TESTNAME drawtext
#include "tests.h"
