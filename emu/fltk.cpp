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
#include <stdio.h>
#include <string.h>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/fl_draw.H>

u8 keymap[32] = { '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 0};
static struct {
    u16 xstart, xend, ystart, yend;
    u16 x, y;
    u32 buttons;
    int  throttle;
    int  rudder;
    int  elevator;
    int  aileron;
    u8  powerdown;
} gui;

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

class mywin : public Fl_Window
{
public:
    mywin(int W, int H) : Fl_Window(W,H) {
    }
    int get_button(int key)
    {
        int i = 0;
        while(keymap[i] != 0) {
            if(key == keymap[i])
                return i;
            i++;
        }
        return -1;
    }
    int handle(int event)
    {
        int key;
        switch(event) {
        case FL_FOCUS:
        case FL_UNFOCUS:
            return 1;
        case FL_KEYDOWN:
        //case FL_SHORTCUT:
            key = get_button(Fl::event_key());
            if(key >= 0) {
                gui.buttons |= (1 << key);
                return 1;
            }
            switch(Fl::event_key()) {
            case 'x':
                gui.powerdown = 1;
                return 1;
            case 'q':
                if(++gui.throttle > 10)
                    gui.throttle = 10;
                return 1;
            case 'a':
                if(--gui.throttle < 0)
                    gui.throttle = 0;
                return 1;
            case 'w':
                if(++gui.rudder > 10)
                    gui.rudder = 10;
                return 1;
            case 's':
                if(--gui.rudder < 0)
                    gui.rudder = 0;
                return 1;
            case 'e':
                if(++gui.elevator > 10)
                    gui.elevator = 10;
                return 1;
            case 'd':
                if(--gui.elevator < 0)
                    gui.elevator = 0;
                return 1;
            case 'r':
                if(++gui.aileron > 10)
                    gui.aileron = 10;
                return 1;
            case 'f':
                if(--gui.aileron < 0)
                    gui.aileron = 0;
                return 1;
            }
        case FL_KEYUP:
            key = get_button(Fl::event_key());
            if(key >= 0) {
                gui.buttons &= ~(1 << key);
            }
            return 1;
        }
        return Fl_Window::handle(event);
    }
};
void Initialize_LCD()
{
  window = new mywin(320,240);
  window->end();
  window->show();
  memset(&gui, 0, sizeof(gui));
  //Fl::add_handler(&handler);
  Fl::wait();
}

void lcd_set_draw_area(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1)
{
    gui.xstart = x0;
    gui.ystart = y0;
    gui.xend   = x1;
    gui.yend   = y1;
    gui.x = x0;
    gui.y = y0;
}

void lcd_draw_pixel(unsigned int color)
{
    u8 r, g, b;
    r = (color >> 8) & 0xf8;
    g = (color >> 3) & 0xfc;
    b = (color << 3) & 0xf8;
    Fl_Color c = fl_rgb_color(r, g, b);
    fl_color(c);
    fl_point(gui.x++, gui.y);
    if(gui.x > gui.xend) {
        gui.x = gui.xstart;
        gui.y++;
    }
}

u32 ScanButtons()
{
    return ~gui.buttons;
}

int CheckPowerSwitch()
{
    return gui.powerdown;
}

void PowerDown() {exit(0);}

u16 ReadThrottle()
{
    return ~(((( 1 << 12) - 1) / 10) * gui.throttle);
}

u16 ReadRudder()
{
    return ~(((( 1 << 12) - 1) / 10) * gui.rudder);
}

u16 ReadElevator()
{
    return ~(((( 1 << 12) - 1) / 10) * gui.elevator);
}

u16 ReadAileron()
{
    return ~(((( 1 << 12) - 1) / 10) * gui.aileron);
}

u32 ReadFlashID()
{
    return 0;
}

