/*
    This project is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Foobar is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "../tx.h"
#include <stdlib.h>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/fl_draw.H>

void Initialize_Backlight() {}
void Initialize_ButtonMatrix() {}
void Initialize_Clock(void) {}
void Initialize_PowerSwitch(void) {}
void Initialize_Channels() {}
void Initialize_SPIFlash() {}
void Initialize_UART() {}
void SignOn() {}

void lcd_drawstart(void) {
    Fl::check();
    Fl::flush();
}
void lcd_drawstop(void) {
    Fl::check();
}

static Fl_Window *window;
u16 range_xstart, range_xend, range_ystart, range_yend;
u16 x, y;

void Initialize_LCD()
{
  window = new Fl_Window(320,240);
  window->end();
  window->show();
  Fl::wait();
}

void lcd_set_draw_area(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1)
{
    range_xstart = x0;
    range_ystart = y0;
    range_xend   = x1;
    range_yend   = y1;
    x = x0;
    y = y0;
}

void lcd_draw_pixel(unsigned int color)
{
    u8 r, g, b;
    r = (color >> 8) & 0xf8;
    g = (color >> 3) & 0xfc;
    b = (color << 3) & 0xf8;
    Fl_Color c = fl_rgb_color(r, g, b);
    fl_color(c);
    fl_point(x, y);
    x++;
    if(x > range_xend) {
        x = range_xstart;
        y++;
    }
}

u32 ScanButtons()
{
    return 12345;
}

int CheckPowerSwitch()
{
    return 0;
}

void PowerDown() {exit(0);}

u16 ReadThrottle()
{
    return 1234;
}

u16 ReadRudder()
{
    return 0;
}

u16 ReadElevator()
{
    return 0;
}

u16 ReadAileron()
{
    return 0;
}

u32 ReadFlashID()
{
    return 0;
}

