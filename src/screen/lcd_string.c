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
#define RANGE_TABLE_SIZE 20
#define NUM_FONTS 10

#define HEIGHT(x) x.height
char FontNames[NUM_FONTS][9];

struct font_def 
{
        u8 idx;
        FILE *fh;
        u8 font[80];
    u8 height;          /* Character height for storage        */
        u16 range[2 * (RANGE_TABLE_SIZE + 1)];  /* Array containing the ranges of supported characters */
};
static struct {
    struct font_def font;
    unsigned int x_start;
    unsigned int x;
    unsigned int y;
    u16          color;
} cur_str;

u8 FONT_GetFromString(const char *value)
{
    int i;
    for (i = 0; i < NUM_FONTS; i++) {
        if (FontNames[i][0] == 0) {
            strncpy(FontNames[i], value, 13);
            return i + 1;
        }
        if(strcasecmp(FontNames[i], value) == 0) {
            return i + 1;
        }
    }
    printf("Unknown font: %s\n", value);
    return 0;
}

u8 get_char_range(u32 c, u32 *begin, u32 *end)
{
    u32 offset = 0;
    u32 pos = 5;
    u16 *range = cur_str.font.range;
    while(1) {
        if (range[0] == 0 && range[1] == 0)
            break;
        if (c >= range[0] && c <= range[1]) {
            pos += 3 * (offset + c - range[0]);
        } else {
            offset += range[1] + 1 - range[0];
        }
        range += 2;
        pos += 4;
    }
    fseek(cur_str.font.fh, pos, SEEK_SET);
    u8 *font = cur_str.font.font;
    fread(font, 6, 1, cur_str.font.fh);
    *begin = font[0] | (font[1] << 8) | (font[2] << 16);
    *end   = font[3] | (font[4] << 8) | (font[5] << 16);
    return 1;
}

const u8 *char_offset(u32 c, u8 *width)
{
    u32 begin;
    u32 end;
    u8 *font = cur_str.font.font;

    u8 row_bytes = ((cur_str.font.height - 1) / 8) + 1;
    get_char_range(c, &begin, &end);
    *width = (end - begin) / row_bytes;
    fseek(cur_str.font.fh, begin, SEEK_SET);
    if (end - begin > sizeof(cur_str.font.font)) {
        printf("Character '%04d' is larger than allowed size\n", (int)c);
        end = begin + (sizeof(cur_str.font.font) / row_bytes) * row_bytes;
        *width = (end - begin) / row_bytes;
    }
    fread(font, end - begin, 1, cur_str.font.fh);
    return font;
}

u8 get_width(u32 c)
{
    u32 begin;
    u32 end;

    u8 row_bytes = ((cur_str.font.height - 1) / 8) + 1;
    get_char_range(c, &begin, &end);
    return (end - begin) / row_bytes;
} 

void LCD_PrintCharXY(unsigned int x, unsigned int y, u32 c)
{
    u8 row, col, width;
    const u8 *offset = char_offset(c, &width);
    if (! offset || ! width) {
        printf("Could not locate character U-%04x\n", (int)c);
        return;
    }
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

u8 open_font(unsigned int idx)
{
    char font[20];
    if (! idx)
        return 0;
    sprintf(font, "media/%s.fon", FontNames[idx-1]);
    if(cur_str.font.fh) {
        fclose(cur_str.font.fh);
        cur_str.font.fh = NULL;
    }
    cur_str.font.fh = fopen(font, "rb");
    if (! cur_str.font.fh) {
        printf("Couldn't open font file: %s\n", font);
        return 0;
    }
    setbuf(cur_str.font.fh, 0);
    if(fread(&cur_str.font.height, 1, 1, cur_str.font.fh) != 1) {
        printf("Failed to read height from font\n");
        fclose(cur_str.font.fh);
        cur_str.font.fh = NULL;
        return 0;
    }
    cur_str.font.idx = idx;
    idx = 0;
    u8 *f = (u8 *)font;
    while(1) {
        if (fread(f, 4, 1, cur_str.font.fh) != 1) {
            printf("Failed to parse font range table\n");
            fclose(cur_str.font.fh);
            cur_str.font.fh = NULL;
            return 0;
        }
        u16 start_c = f[0] | (f[1] << 8);
        u16 end_c = f[2] | (f[3] << 8);
        cur_str.font.range[idx++] = start_c;
        cur_str.font.range[idx++] = end_c;
        if (start_c == 0 && end_c == 0)
            break;
    }
    return 1;
}

u8 LCD_SetFont(unsigned int idx)
{
    u8 old = LCD_GetFont();
    if (! open_font(idx))
        open_font(old);
    return old;
}

u8 LCD_GetFont()
{
    return cur_str.font.idx;
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
