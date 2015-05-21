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
#include "config/display.h"
#include "gui/gui.h"
#include "../common/emu/fltk.h"
#include "lcd.h"
#include "tw8816_font.h"
#include "tw8816_2_font.h"
#include "tw8816_3_font.h"

struct rgb {
    u8 r;
    u8 g;
    u8 b;
};
static const struct rgb background = {0x00, 0x00, 0xff};
static const struct rgb foreground = {0xff, 0xff, 0xff};

static struct font_def tiny_font;
static struct font_def default_font;
static struct font_def large_font;
struct font_str cur_str;


#define HEIGHT(x) x.height

/*
 * // since devo10's screen is too small in emulator , we have it zoomed by 2 for both rows and columns
 * color = 0x0 means white, other value means black
 */
void LCD_DrawPixel(unsigned int color)
{
	if (gui.x < IMAGE_X && gui.y < IMAGE_Y) {	// both are unsigned, can not be < 0
		struct rgb c;
		// for emulator of devo 10, 0x0 means white while others mean black
		c = color ? foreground : background; // 0xaa is grey color(not dot)

		gui.image[3*(IMAGE_X * gui.y + gui.x) + 0]     = c.r;
		gui.image[3*(IMAGE_X * gui.y + gui.x) + 1]     = c.g;
		gui.image[3*(IMAGE_X * gui.y + gui.x) + 2]     = c.b;
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
unsigned window_x, window_y;
int window_mult;
u8 font_ram[27 * 202];
extern u8 font_map[27 * 4 * 6];
extern u8 window;
void DrawMappedChar(int pos, u8 *data);
void LCD_PrintCharXY(unsigned int x, unsigned int y, u32 c)
{
    u8 row, col, width;
    if (c > 0xff) {
        window_mult = cur_str.font.zoom;
        window_x = x * 12;
        window_y = y * 18;
        LCD_DrawStart(x * CHAR_WIDTH, y * CHAR_HEIGHT, (x+window_mult) * CHAR_WIDTH,  (y+window_mult) * CHAR_HEIGHT, DRAW_NWSE);
        DrawMappedChar(0, font_ram + (c & 0xff) * 27);
        LCD_DrawStop();
        return;
    }
    c = TW8816_map_char(c);
    int font_size = cur_str.font.zoom;
    const u8 *offset = char_offset(c, &width);
    if (! offset || ! width) {
        printf("Could not locate character U-%04x\n", (int)c);
        return;
    }
    x = x - x % 2;
    y = y - y % 2;
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
    font->zoom = *data / CHAR_HEIGHT;
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
    open_font(&tiny_font,    tw8816_fon, 3);
    open_font(&default_font, tw8816_2_fon, 1);
    open_font(&large_font,   tw8816_3_fon, 2);
    Display.xygraph.grid_color = 0xffff;
    Display.xygraph.axis_color = 0xffff;
    Display.xygraph.fg_color = 0xffff;
    Display.xygraph.point_color = 0xffff;
}

u8 LCD_SetFont(unsigned int idx)
{
    u8 old = LCD_GetFont();
    if (idx == 3)
        cur_str.font = tiny_font;
    else if (idx == 2)
        cur_str.font = large_font;
    else
        cur_str.font = default_font;
    return old;
}

u8 FONT_GetFromString(const char *value)
{
    (void)value;
    return 1;
}

void LCD_ShowVideo(u8 enable)
{
    (void) enable;
}

void DrawMappedChar(int pos, u8 *data) {
    int x_off = window_x + ((pos % 6) * 12 * 3);
    int y_off = window_y + ((pos / 6) * 18 * 3);
    for (int x = 0; x < 12; x++) {
        for (int y = 0; y < 18; y++) {
            int byte = y / 2 * 3 + x / 4;
            int bit = 4 * (y & 1) + 3 - (x & 0x3);
            struct rgb c;
            c = (data[byte] & (1 << bit)) ? foreground : background;
            for (int my = 0; my < window_mult; my++) {
                for (int mx = 0; mx < window_mult; mx++) {
                    gui.image[3*((IMAGE_X * (y_off +y*window_mult+my)) + x_off + x*window_mult + mx) + 0] = c.r;
                    gui.image[3*((IMAGE_X * (y_off +y*window_mult+my)) + x_off + x*window_mult + mx) + 1] = c.g;
                    gui.image[3*((IMAGE_X * (y_off +y*window_mult+my)) + x_off + x*window_mult + mx) + 2] = c.b;
                }
            }
        }
    }
}

void LCD_CreateMappedWindow(unsigned val, unsigned x, unsigned y, unsigned w, unsigned h)
{
    window_x = x * 12;
    window_y = y * 18;
    (void)val;
    (void)y;
    (void)w;
    (void)h;
}
void LCD_UnmapWindow(unsigned val)
{
    (void)val;
}
void LCD_SetMappedWindow(unsigned val)
{
    if (val != 0) {
        memset(font_map, 0, sizeof(font_map));
    } else {
        if (window < 4) {
            window_mult = 3;
            for (int i = 0; i < 24; i++) {
                DrawMappedChar(i, font_map + i * 27);
            }
        } else {
            memcpy(font_ram + (window - 4) * 27, font_map, 27);
        }
    }
    window = val;
}

