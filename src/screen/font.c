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
#include "font.h"

static FSHANDLE FontFH;

#define RANGE_TABLE_SIZE 20

static struct font_def
{
    FILE *fh;
    u8 height;          /* Character height for storage        */
    u16 range[2 * (RANGE_TABLE_SIZE + 1)];  /* Array containing the ranges of supported characters */
}font;

static u8 get_char_range(u32 c, u32 *begin, u32 *end)
{
    u32 offset = 0;
    u32 pos = 5;
    u8 buf[6];
    u16 *range = font.range;
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
    fseek(font.fh, pos, SEEK_SET);
    fread(buf, 6, 1, font.fh);
    *begin = buf[0] | (buf[1] << 8) | (buf[2] << 16);
    *end   = buf[3] | (buf[4] << 8) | (buf[5] << 16);
    return 1;
}

void char_read(u8 *fontbuf, u32 c, u8 *width)
{
    u32 begin;
    u32 end;

    u8 row_bytes = ((font.height - 1) / 8) + 1;
    get_char_range(c, &begin, &end);
    *width = (end - begin) / row_bytes;
    fseek(font.fh, begin, SEEK_SET);
    if (end - begin > CHAR_BUF_SIZE) {
        printf("Character '%04d' is larger than allowed size\n", (int)c);
        end = begin + (CHAR_BUF_SIZE / row_bytes) * row_bytes;
        *width = (end - begin) / row_bytes;
    }
    fread(fontbuf, end - begin, 1, font.fh);
}

u8 get_width(u32 c)
{
    u32 begin;
    u32 end;

    u8 row_bytes = ((font.height - 1) / 8) + 1;
    get_char_range(c, &begin, &end);
    return (end - begin) / row_bytes;
}

u8 get_height()
{
    return font.height;
}

void close_font()
{
    if(font.fh) {
        fclose(font.fh);
        font.fh = NULL;
    }
}

u8 open_font(const char* fontname)
{
    char filename[20];
    close_font();

    sprintf(filename, "media/%s.fon", fontname);
    finit(&FontFH, "media");
    font.fh = fopen2(&FontFH, filename, "rb");
    if (! font.fh) {
        printf("Couldn't open font file: %s\n", filename);
        return 0;
    }
    setbuf(font.fh, 0);
    if(fread(&font.height, 1, 1, font.fh) != 1) {
        printf("Failed to read height from font\n");
        fclose(font.fh);
        font.fh = NULL;
        return 0;
    }

    int range_idx = 0;
    u8 buf[4];
    while(1) {
        if (fread(buf, 4, 1, font.fh) != 1) {
            printf("Failed to parse font range table\n");
            fclose(font.fh);
            font.fh = NULL;
            return 0;
        }
        u16 start_c = buf[0] | (buf[1] << 8);
        u16 end_c = buf[2] | (buf[3] << 8);
        font.range[range_idx++] = start_c;
        font.range[range_idx++] = end_c;
        if (start_c == 0 && end_c == 0)
            break;
    }
    return 1;
}
