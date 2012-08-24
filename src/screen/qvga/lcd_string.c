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
#include "fonts.h"

struct FONT_DEF 
{
	u8 width;     	/* Character width for storage         */
	u8 height;  		/* Character height for storage        */
        const u32 *range;  /* Array containing the ranges of supported characters */
	const u8 *font_table;       /* Font table start address in memory  */
};
#define WIDTH(x)           (0x7F & x->width)
#define HEIGHT(x)          (x->height)
#define IS_PROPORTIONAL(x) (0x80 & x->width)
const struct FONT_DEF Fonts[];

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
#define FONTDECL(w, h, range, font, name) name,
static const char * const FontNames[] = {
    #include "fonts.h"
    "",
};
#undef FONTDECL
#if 0
    "system5x7",
    "arial10",
    "arial10narrow",
    "arial14",
    "arial14bold",
    "arial14narrow",
    "arial14narrowbold",
    "arial18bold",
    "arial24bold",
    "trebuchet48",
    "",
#endif

#define FONTDECL(w, h, range, font, name) {w, h, range, font},
const struct FONT_DEF Fonts[] = {
    #include "fonts.h"
    {0, 0, 0, 0},
};
#undef FONTDECL
#if 0
    {5, 7, 0x20, 0x80, FontSystem5x7},
    {0x80 | 10, 10, 0x20, 0x7F, FontArial_10},
    {0x80 | 10, 12, 0x20, 0x7F, FontArial_10_Narrow},
    {0x80 | 10, 15, 0x20, 0x80, FontArial_14},
    {0x80 | 10, 15, 0x20, 0x80, FontArial_14_Bold},
    {0x80 | 10, 15, 0x20, 0x7F, FontArial_14_Narrow},
    {0x80 | 10, 15, 0x20, 0x7F, FontArial_14_NarrowBold},
    {0x80 | 15, 15, 0x20, 0x5b, FontArial_18_Bold},
    {0x80 | 27, 23, 0x20, 0x5b, FontArial_24_Bold},
    {0x80 | 34, 48, 0x25, 0x3a, FontTrebuchet_MS_48},
    {0, 0, 0, 0, 0},
    };
#endif
#define LINE_SPACING 2
#define CHAR_SPACING 1
static struct {
    const struct FONT_DEF *font;
    unsigned int x_start;
    unsigned int x;
    unsigned int y;
    u16          color;
} cur_str;

u8 FONT_GetFromString(const char *value)
{
    int i;
    for (i = 0; FontNames[i][0]; i++)
        if(strcasecmp(FontNames[i], value) == 0) {
            return 1 + i;
        }
    printf("Unknown font: %s\n", value);
    return 0;
}

const u8 *char_offset(u8 c, const struct FONT_DEF *font, u8 *width)
{
    u32 offset = 0;
    u32 count = 0;
    int found = 0;
    const u32 *ptr = font->range;
    u8 row_bytes = (HEIGHT(cur_str.font) - 1) / 8 + 1;
    while(*ptr) {
        uint32_t i;
        for (i = *ptr; i <= *(ptr+1); i++) {
            if (c == i) {
                found = 1;
                if (IS_PROPORTIONAL(font)) {
                    *width = font->font_table[count];
                } else {
                    *width = WIDTH(font);
                    return font->font_table + offset * row_bytes * (*width);
                }
            }
            if (IS_PROPORTIONAL(font)) {
                if (! found) {
                    //Keep track of the width of this character
                    offset += font->font_table[count] * row_bytes;
                }
                //Keep track of the number of bytes in the width table
                count++;
            } else {
                //Keep track of number of characters to this point
                offset++;
            }
        }
        ptr += 2;
    }
    if(! found)
        return NULL;
    return font->font_table + count + offset;
}

u8 get_width(u32 c)
{
    const u32 *ptr = cur_str.font->range;
    const u8 *pos = cur_str.font->font_table;
    while(*ptr) {
        if (c >= *ptr && c <= *(ptr+1)) {
            return IS_PROPORTIONAL(cur_str.font)
                          ? *(pos + (c - *ptr))
                          : WIDTH(cur_str.font);
        }
        pos += (*(ptr+1) - *ptr);
        ptr += 2;
    }
    return 0;
}

void LCD_PrintCharXY(unsigned int x, unsigned int y, u32 c)
{
    u8 row, col, width;
    const u8 *offset = char_offset(c, cur_str.font, &width);
    if (! offset)
        return;
    // Check if the requested character is available
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

u8 LCD_SetFont(unsigned int idx)
{
    unsigned int i;
    u8 old = LCD_GetFont();
    idx--;
    for(i = 0; i <= idx; i++) {
        if(Fonts[i].width == 0)
            return old;
    }
    cur_str.font = &Fonts[idx];
    return old;
}

u8 LCD_GetFont()
{
    return (cur_str.font - Fonts) + 1;
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
        cur_str.y += HEIGHT(cur_str.font) + LINE_SPACING;
    } else {
        LCD_PrintCharXY(cur_str.x, cur_str.y, c);
        cur_str.x += get_width(c) + CHAR_SPACING;
    }
}

void LCD_GetCharDimensions(u32 c, u16 *width, u16 *height) {
    *height = HEIGHT(cur_str.font);
    *width = get_width(c);
}

void LCD_GetStringDimensions(const u8 *str, u16 *width, u16 *height) {
    int line_width = 0;
    *height = HEIGHT(cur_str.font);
    *width = 0;
    //printf("String: %s\n", str);
    while(*str) {
        u32 ch;
        str = (const u8 *)utf8_to_u32((const char *)str, &ch);
        if(ch == '\n') {
            *height += HEIGHT(cur_str.font) + LINE_SPACING;
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
