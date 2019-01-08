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
#include "emu.h"

void LCD_Init()
{
    memset(gui.image, 0xf0, sizeof(gui.image));
    gui.x = 0;
    gui.y = 10;
    gui.xstart = 100;
    gui.xend = 47;
}

void LCD_DrawStart(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, enum DrawDir dir)
{
    if (dir == DRAW_SWNE) {
        gui.y = y1;  // bug fix: must do it this way to draw bmp
        gui.dir = -1;
    } else {
        gui.y = y0;
        gui.dir = 1;
    }
    gui.xstart = x0;
    gui.xend = x1;
    gui.x = x0;
}

void LCD_DrawStop(void) {
}

void LCD_DrawPixelXY(unsigned int x, unsigned int y, unsigned int color)
{
    gui.xstart = x;
    gui.x = x;
    gui.y = y;
    LCD_DrawPixel(color);
}

void LCD_DrawPixel(unsigned int color)
{
	if (gui.x < LCD_WIDTH && gui.y < LCD_HEIGHT) {	// both are unsigned, can not be < 0
		u8 r, g, b;
		r = (color >> 8) & 0xf8;
		g = (color >> 3) & 0xfc;
		b = (color << 3) & 0xf8;
        gui.image[3*(LCD_WIDTH*gui.y+gui.x)] = r;
        gui.image[3*(LCD_WIDTH*gui.y+gui.x)+1] = g;
        gui.image[3*(LCD_WIDTH*gui.y+gui.x)+2] = b;
	}
	// this must be executed to continue drawing in the next row
    gui.x++;
    if(gui.x > gui.xend) {
        gui.x = gui.xstart;
        gui.y += gui.dir;
    }
}

void LCD_ForceUpdate()
{
}
