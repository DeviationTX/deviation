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
#include "../common_emu/fltk.h"

void LCD_DrawPixel(unsigned int color)
{
    u8 c;
    int row, col;
    int i, j;
    c = color ? 0x00 : 0xaa;

    //Fill in 4 dots
    row = 2 * gui.y;
    col = 2 * gui.x;
    printf("(%d, %d) => %d\n", gui.x, gui.y, 3*(LCD_WIDTH * row) + col);
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 2; j++) {
            gui.image[3*(LCD_WIDTH * (row + i) + col + j) + 0] = c;
            gui.image[3*(LCD_WIDTH * (row + i) + col + j) + 1] = c;
            gui.image[3*(LCD_WIDTH * (row + i) + col + j) + 2] = c;
        }
    }
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

/* stubs */
u8 LCD_ImageIsTransparent(const char *file)
{
    (void) file;
    assert(0);
}

u8 LCD_ImageDimensions(const char *file, u16 *w, u16 *h)
{
    (void)file;
    (void)w;
    (void)h;
    assert(0);
}

void LCD_DrawWindowedImageFromFile(u16 x, u16 y, const char *file, s16 w, s16 h, u16 x_off, u16 y_off)
{
    assert(0);
}

void LCD_DrawImageFromFile(u16 x, u16 y, const char *file)
{
    assert(0);
}
