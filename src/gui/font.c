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
#include "font.h"

struct FAT FontFAT = {{0}};
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
#define RANGE_TABLE_SIZE 20

struct font_def 
{
    u8 idx;
    u8 height;          /* Character height for storage        */
    FILE *fh;
    u16 range[2 * (RANGE_TABLE_SIZE + 1)];  /* Array containing the ranges of supported characters */
}cur_font;


static u8 GetCharRange(u32 c, u32 *begin, u32 *end)
{
    u32 offset = 0;
    u32 pos = 5;
    u16 *range = cur_font.range;
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
    fseek(cur_font.fh, pos, SEEK_SET);
    u8 data[6];
    fread(data, 6, 1, cur_font.fh);
    *begin = data[0] | (data[1] << 8) | (data[2] << 16);
    *end   = data[3] | (data[4] << 8) | (data[5] << 16);
    return 1;
}


u8 FONT_ReadCharBits(u8 *font, u32 c, u8 *width)
{
    u32 begin;
    u32 end;

    u8 row_bytes = ((cur_font.height - 1) / 8) + 1;
    GetCharRange(c, &begin, &end);
    *width = (end - begin) / row_bytes;
    fseek(cur_font.fh, begin, SEEK_SET);
    if (end - begin > MAX_FONTSIZE) {
        printf("Character '%04d' is larger than allowed size\n", (int)c);
        end = begin + (MAX_FONTSIZE / row_bytes) * row_bytes;
        *width = (end - begin) / row_bytes;
    }
    fread(font, end - begin, 1, cur_font.fh);
    return end-begin;
}

u8 FONT_GetWidth(u32 c)
{
    u32 begin;
    u32 end;

    u8 row_bytes = ((cur_font.height - 1) / 8) + 1;
    GetCharRange(c, &begin, &end);
    return (end - begin) / row_bytes;
}

u8 FONT_GetHeight()
{
    return cur_font.height;
}

void FONT_Close()
{
    if(cur_font.fh) {
        fclose(cur_font.fh);
        cur_font.fh = NULL;
    }
}

u8 FONT_Open(unsigned int idx, const char* fontname)
{
    char font[20];
    FONT_Close();
    if (! idx) {
        cur_font.idx = 0;
        return 1;
    }
    sprintf(font, "media/%s.fon", fontname);
    finit(&FontFAT, "media");
    cur_font.fh = fopen2(&FontFAT, font, "rb");
    if (! cur_font.fh) {
        printf("Couldn't open font file: %s\n", font);
        return 0;
    }
    setbuf(cur_font.fh, 0);
    if(fread(&cur_font.height, 1, 1, cur_font.fh) != 1) {
        printf("Failed to read height from font\n");
        fclose(cur_font.fh);
        cur_font.fh = NULL;
        return 0;
    }
    cur_font.idx = idx;
    idx = 0;
    u8 *f = (u8 *)font;
    while(idx < RANGE_TABLE_SIZE) {
        if (fread(f, 4, 1, cur_font.fh) != 1) {
            printf("Failed to parse font range table\n");
            fclose(cur_font.fh);
            cur_font.fh = NULL;
            return 0;
        }
        u16 start_c = f[0] | (f[1] << 8);
        u16 end_c = f[2] | (f[3] << 8);
        cur_font.range[idx++] = start_c;
        cur_font.range[idx++] = end_c;
        if (start_c == 0 && end_c == 0)
            break;
    }
    return 1;
}

u8 FONT_GetIndex()
{
    return cur_font.idx;
}
