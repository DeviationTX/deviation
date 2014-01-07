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

#include <assert.h>

#include "common.h"
#include "gui/gui.h"
#include "../common/emu/fltk.h"
#include "lcd.h"

static int logical_lcd_width = LCD_WIDTH*LCD_WIDTH_MULT;
#define HEIGHT(x) x.height
/*
 * // since devo10's screen is too small in emulator , we have it zoomed by 2 for both rows and columns
 * color = 0x0 means white, other value means black
 */
void LCD_DrawPixel(unsigned int color)
{
	if (gui.x < LCD_WIDTH*LCD_WIDTH_MULT/2 && gui.y < LCD_HEIGHT*LCD_WIDTH_MULT/2) {	// both are unsigned, can not be < 0
		u8 c;
		int row, col;
		int i, j;
		// for emulator of devo 10, 0x0 means white while others mean black
		c = color ? 0x00 : 0xaa; // 0xaa is grey color(not dot)

		//Fill in 4 dots
		row = 2 * gui.y;
		col = 2 * gui.x;
		for (i = 0; i < 2; i++) {
			for (j = 0; j < 2; j++) {
                gui.image[3*(logical_lcd_width* (row + i) + col + j)] = c;
                gui.image[3*(logical_lcd_width* (row + i) + col + j) + 1] = c;
                gui.image[3*(logical_lcd_width* (row + i) + col + j) + 2] = c;
            }
        }
	}
	// this must be executed to continue drawing in the next row
    gui.x++;
    if(gui.x > gui.xend) {
        gui.x = gui.xstart;
        gui.y += gui.dir;
    }
}

void LCD_Clear(unsigned int color) {
	(void)color;
	memset(gui.image, 0xaa, sizeof(gui.image));

}

/*
 * Since we have a text based screen we need some of the text writing functions here
 */
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

void LCD_PrintCharXY(unsigned int x, unsigned int y, u32 c)
{
    u8 row, col, width;
    const u8 *offset = char_offset(c, &width);
    if (! offset || ! width) {
        printf("Could not locate character U-%04x\n", (int)c);
        return;
    }
    // Check if the requested character is available
    LCD_DrawStart(x * CHAR_WIDTH, y * CHAR_HEIGHT, (x+1) * CHAR_WIDTH,  (y+1) * CHAR_HEIGHT, DRAW_NWSE);
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
                LCD_DrawPixelXY((x * CHAR_WIDTH) + col, (y * CHAR_HEIGHT) + row, cur_str.color);
            }
            bit++;
        }
    }
    LCD_DrawStop();
}

void close_font()
{
    if(cur_str.font.fh) {
        fclose(cur_str.font.fh);
        cur_str.font.fh = NULL;
    }
}

u8 open_font(unsigned int idx)
{
    char font[20];
    close_font();
    if (! idx) {
        cur_str.font.idx = 0;
        return 1;
    }
    sprintf(font, "media/%s.fon", FontNames[idx-1]);
    finit(&FontFAT, "media");
    cur_str.font.fh = fopen2(&FontFAT, font, "rb");
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
    if (old == idx)
        return old;
    if (! open_font(idx))
        open_font(old);
    return old;
}

u8 LCD_GetFont()
{
    return cur_str.font.idx;
}