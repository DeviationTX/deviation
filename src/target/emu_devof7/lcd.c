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
#include "ia9211_font.h"
#include "ia9211_bigfont.h"

struct rgb {
    u8 r;
    u8 g;
    u8 b;
};
static const struct rgb background = {0x00, 0x00, 0xff};
static const struct rgb foreground = {0xff, 0xff, 0xff};
static int logical_lcd_width = LCD_WIDTH*LCD_WIDTH_MULT;

static struct font_def default_font;
static struct font_def big_font;
struct font_str cur_str;


#define HEIGHT(x) x.height

/*
 * // since devo10's screen is too small in emulator , we have it zoomed by 2 for both rows and columns
 * color = 0x0 means white, other value means black
 */
void LCD_DrawPixel(unsigned int color)
{
	if (gui.x < LCD_WIDTH*LCD_CHAR_W && gui.y < LCD_HEIGHT*LCD_CHAR_H) {	// both are unsigned, can not be < 0
		struct rgb c;
		int row, col;
		int i, j;
		// for emulator of devo 10, 0x0 means white while others mean black
		c = color ? foreground : background; // 0xaa is grey color(not dot)

		//Fill in 4 dots
		row = ZOOM_Y * gui.y;
		col = ZOOM_X * gui.x;
		for (i = 0; i < ZOOM_Y; i++) {
			for (j = 0; j < ZOOM_X; j++) {
				gui.image[3*(logical_lcd_width* (row + i) + col + j)]     = c.r;
				gui.image[3*(logical_lcd_width* (row + i) + col + j) + 1] = c.g;
				gui.image[3*(logical_lcd_width* (row + i) + col + j) + 2] = c.b;
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
        for (unsigned i = 0; i < sizeof(gui.image); i+= 3) {
            gui.image[i] = background.r;
            gui.image[i+1] = background.g;
            gui.image[i+2] = background.b;
        }

}

/*
 * Since we have a text based screen we need some of the text writing functions here
 */
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
    const u8 *font = cur_str.font.data + pos;
    *begin = font[0] | (font[1] << 8) | (font[2] << 16);
    *end   = font[3] | (font[4] << 8) | (font[5] << 16);
    return 1;
}

const u8 *char_offset(u32 c, u8 *width)
{
    u32 begin;
    u32 end;

    u8 row_bytes = ((cur_str.font.height - 1) / 8) + 1;
    get_char_range(c, &begin, &end);
    *width = (end - begin) / row_bytes;
    return cur_str.font.data + begin;
}

void LCD_PrintCharXY(unsigned int x, unsigned int y, u32 c)
{
    u8 row, col, width;
    c = IA9211_map_char(c);
    int font_size = cur_str.font.height / CHAR_HEIGHT;

    const u8 *offset = char_offset(c, &width);
    if (! offset || ! width) {
        printf("Could not locate character U-%04x\n", (int)c);
        return;
    }
    for (unsigned int i = 0; i < y; i++)
    // Check if the requested character is available
    LCD_DrawStart(x * CHAR_WIDTH, y * CHAR_HEIGHT, (x+font_size) * CHAR_WIDTH,  (y+font_size) * CHAR_HEIGHT, DRAW_NWSE);

    // First clean th area
    for(col = 0; col < font_size * CHAR_WIDTH; col++) {
        for(row = 0; row < font_size * CHAR_HEIGHT; row++) {
            LCD_DrawPixelXY((x * CHAR_WIDTH) + col, (y * CHAR_HEIGHT) + row, 0);
        }
    }

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
}

void open_font(struct font_def *font, const u8 *data, int fontidx)
{
    font->height = *data;
    font->idx = fontidx;
    font->data = data;
    int idx = 0;
    const u8 *f = data+1;
    while(1) {
        u16 start_c = f[0] | (f[1] << 8);
        u16 end_c = f[2] | (f[3] << 8);
        font->range[idx++] = start_c;
        font->range[idx++] = end_c;
        f+= 4;
        if (start_c == 0 && end_c == 0)
            break;
    }
}

void _lcd_init()
{
    open_font(&default_font, ia9211_fon, 1);
    open_font(&big_font, ia9211_bigfon, 2);
}

u8 LCD_SetFont(unsigned int idx)
{
    u8 old = LCD_GetFont();
    cur_str.font = (idx <= 1) ? default_font : big_font;
    return old;
}

u8 FONT_GetFromString(const char *value)
{
    if (strcmp(value, "big") == 0) {
        return 2;
    }
    return 1;
}

void LCD_ShowVideo(u8 enable)
{
    (void) enable;
}
