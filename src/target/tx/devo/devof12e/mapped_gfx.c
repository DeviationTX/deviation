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

// These routines manage drawing to a mapped graphics buffer.
// This buffer is then mapped into individual characters which
// Are uploaded to the LCD font buffer
#include "common.h"

#define WINDOW_CHARS_X 6
#define WINDOW_CHARS_Y 4
#define WINDOW_X (WINDOW_CHARS_X * CHAR_WIDTH)
#define WINDOW_Y (WINDOW_CHARS_Y * CHAR_HEIGHT)
#define BYTES_PER_FONTCHAR 27
u8 font_map[WINDOW_CHARS_X * WINDOW_CHARS_Y * BYTES_PER_FONTCHAR];
u8 window = 0;

//Use u8 since the window size is less than 256 pixels in either dir
static u8 map_xstart;
static u8 map_xend;
static u8 map_x;
static u8 map_y;
static u8 map_dir;

//This will convert an x/y pixel into a bit in the font map
void LCD_DrawMappedPixel(unsigned int color)
{
    unsigned x = map_x;
    unsigned y = map_y;

    if (window == 0 || ! color)
        return;
    if (x >= WINDOW_X || y >= WINDOW_Y)
        return;
    int byte_offset = y / CHAR_HEIGHT * WINDOW_CHARS_X + x / CHAR_WIDTH;
    unsigned y1 = y % CHAR_HEIGHT;
    unsigned x1 = x % CHAR_WIDTH;
    int byte = y1 / 2 * 3 + x1 / 4;          //there are 3 bytes across, contianing a total of 2 rows
    int bit = 4 * (y1 & 1) + 3 - (x1 & 0x3); //each byte is  4pixels wide by 2 rows tall
    font_map[BYTES_PER_FONTCHAR * byte_offset + byte] |= 1 << bit;
    map_x++;
    if(map_x > map_xend) {
        map_x = map_xstart;
        map_y += map_dir;
    }
}

void LCD_DrawMappedPixelXY(unsigned int x, unsigned int y, unsigned int color)
{
    map_x = x;
    map_y = y;
    LCD_DrawMappedPixel(color);
}

void LCD_DrawMappedStart(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, enum DrawDir dir)
{
    map_xstart = x0;
    map_xend   = x1;
    map_x      = x0;
    if (dir == DRAW_NWSE) {
        map_y = y0;
        map_dir = 1;
    } else if (dir == DRAW_SWNE) {
        map_y = y1;
        map_dir = -1;
    }
}

unsigned LCD_GetMappedWindow() {
    return window;
}
