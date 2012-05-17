/*
    This project is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Foobar is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "target.h"
#include "fonts.h"

#include "fonts.h"

#include "font_arial_14.h"
#include "font_arial_bold_14.h"
#include "font_corsiva_12.h"
#include "font_system_5x7.h"
#include "font_system_8x8thin.h"
#include "font_system_8x8thick.h"
#include "font_system_8x8ord.h"
#include "font_system_5x8.h"
#include "font_system_7x8.h"

/* The font bitfield is designed to be backwards compatible with standard
 * GLCD 8bit bit-fields, but to also support upto 32x32 proportional fonts
 *
 * For fixed fonts, the 'width' field of struct FONT_DEF will be the character
 * width.  For proportional fonts, the 'width' filed will be ORed with 0x80
 *
 * If the font is proportional, the 'font_table' will begin with a list of u8
 * values which represent the width of each character.  The font data follows
 * immediately afterwards.
 * If the font is fixed-width, the font data starts at the beginning of the
 * 'font_table'
 *
 * The font data is represented as a bit-field.  A chunk of 1, 2, 3, or 4
 * bytes represents a single column of pixels.  This will be repeated for
 * the width of the font.
 * The column chunk can be thought of as a little-endian number of 8, 16, 24,
 * or 32 bits, with the low-order bit representing the top row, and the
 * 'height'-th bit representing the bottom row.
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

const struct FONT_DEF Fonts[] = {
    {5, 7, 0x20, 0x80, FontSystem5x7},
    {5, 8, 32, 128, FontSystem5x8},
    {7, 8, 32, 128, FontSystem7x8},
    {8, 8, 32, 128, Font8x8ord},
    {8, 8, 32, 128, Font8x8thk},
    {8, 8, 0x20, 0x80, Font8x8thn},
    {0x80 | 10, 15, 0x20, 0x80, FontArial_14},
    {0x80 | 10, 15, 0x20, 0x80, FontArial_bold_14},
    {0x80 | 10, 12, 0x20, 0x80, FontCorsiva_12},
    {0},
    };

#define LINE_SPACING 2
#define CHAR_SPACING 1
static struct {
    const struct FONT_DEF *font;
    unsigned int x_start;
    unsigned int x;
    unsigned int y;
    u16          color;
} cur_str;

u8 get_width(u8 c)
{
    if ((c < cur_str.font->first_char) || (c > cur_str.font->last_char))
        return 0;
    if (IS_PROPORTIONAL(cur_str.font)) {
        return *(cur_str.font->font_table + c - cur_str.font->first_char);
    } else {
        return WIDTH(cur_str.font);
    }
}

void LCD_PrintCharXY(unsigned int x, unsigned int y, char c)
{
    u8 row, col, width;
    u8 row_bytes = HEIGHT(cur_str.font) > 16 ? 4 : HEIGHT(cur_str.font) > 8 ? 2 : 1;
    const u8 *offset = cur_str.font->font_table;
    // Check if the requested character is available
    if ((c >= cur_str.font->first_char) && (c <= cur_str.font->last_char))
    {
        // Check if font is proportional
        if (IS_PROPORTIONAL(cur_str.font)) {
            const u8 *proportional_width = offset;
            // Add up width of all characters up to 'c'
            for(row = cur_str.font->first_char; row < c; row++) {
                offset += (*proportional_width++) * row_bytes;
            }
            width = *proportional_width;
            // Move offset past width table
            offset += cur_str.font->last_char - cur_str.font->first_char;
        } else {
            offset += (c - cur_str.font->first_char) * WIDTH(cur_str.font) * row_bytes;
            width = WIDTH(cur_str.font);
        }
        LCD_DrawStart(x, y, x + width - 1,  y + HEIGHT(cur_str.font) - 1, DRAW_NWSE);
        for (col = 0; col < width; col++)
        {
            const u8 *data = offset++;
            u8 bit = 0;
            // Data is right aligned,adrawn top to bottom
            for (row = 0; row < HEIGHT(cur_str.font); ++row)
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
}

void LCD_SetFont(unsigned int idx)
{
    int i;
    for(i = 0; i <= idx; i++) {
        if(Fonts[i].width == 0)
            return;
    }
    cur_str.font = &Fonts[idx];
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
        LCD_PrintChar(*str);
        str++;
    }
}

void LCD_PrintChar(const char c)
{
    if(c == '\n') {
        cur_str.x = cur_str.x_start;
        cur_str.y += HEIGHT(cur_str.font) + LINE_SPACING;
    } else {
        LCD_PrintCharXY(cur_str.x, cur_str.y, c);
        cur_str.x += get_width(c) + CHAR_SPACING;
    }
}

void LCD_GetStringDimensions(const u8 *str, u16 *width, u16 *height) {
    *height = HEIGHT(cur_str.font);
    *width = 0;
    //printf("String: %s\n", str);
    while(*str) {
        if(*str == '\n') {
            *height += HEIGHT(cur_str.font) + LINE_SPACING;
        } else {
            *width += get_width(*str) + CHAR_SPACING;
        }
        str++;
    }
    //printf("W: %d   H: %d\n",(int)*width,(int)*height);
}

void LCD_SetFontColor(u16 color) {
    cur_str.color = color;
}
