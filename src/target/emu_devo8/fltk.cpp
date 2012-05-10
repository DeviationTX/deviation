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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/timeb.h>
#include <time.h>

#include <FL/Fl.H>
#include <FL/x.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Output.H>
#include <FL/fl_draw.H>

extern "C" {
#include "target.h"
}

#define ALT_DRAW

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
    u8  mouse;
    u16 mousex, mousey;
    Fl_Output *raw[20];
    Fl_Output *final[20];
    u32 last_redraw;
#ifdef ALT_DRAW
    u8  image[320*240*3];
#else
    Fl_Offscreen image;
#endif
} gui;

static struct {
    s32 xscale;
    s32 yscale;
    s32 xoffset;
    s32 yoffset;
} calibration = {0x10000, 0x10000, 0, 0};

static Fl_Window *main_window;
static Fl_Box    *image;
void update_channels(void *);

#define WINDOW Fl_Window
class mywin : public WINDOW
{
public:
    mywin(int W, int H) : WINDOW(W,H) {
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
            redraw();
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
        case FL_PUSH:
        case FL_DRAG:
            gui.mouse = 1;
            gui.mousex = calibration.xscale * Fl::event_x() / 0x10000 + calibration.xoffset;
            gui.mousey = calibration.yscale * Fl::event_y() / 0x10000 + calibration.yoffset;
            return 1;
        case FL_RELEASE:
            gui.mouse = 0;
            return 1;
        }
        return WINDOW::handle(event);
    }
};
class image_box : public Fl_Box
{
public:
    image_box(int X, int Y, int W, int H) : Fl_Box(X, Y, W, H) {
    }
    void draw() {
#ifdef ALT_DRAW
      fl_draw_image(gui.image, 0, 0, 320,240, 3, 0);
#else
      fl_copy_offscreen(x(), y(), w(), h(), gui.image, 0, 0);
#endif
    }
};
void update_channels(void *params)
{
    (void)params;
    char str[80];
    int changed = 0;
    if(gui.raw[0]) {
        sprintf(str, "%6.2f%%", gui.throttle * 10.0);
        if(strcmp(str, gui.raw[0]->value()))
            changed |= gui.raw[0]->value(str);
        sprintf(str, "%6.2f%%", gui.rudder * 10.0);
        if(strcmp(str, gui.raw[1]->value()))
            changed |= gui.raw[1]->value(str);
        sprintf(str, "%6.2f%%", gui.elevator * 10.0);
        if(strcmp(str, gui.raw[2]->value()))
            changed |= gui.raw[2]->value(str);
        sprintf(str, "%6.2f%%", gui.aileron * 10.0);
        if(strcmp(str, gui.raw[3]->value()))
            changed = gui.raw[3]->value(str);
    }
}
extern "C" {
extern void event_loop(void *);
void start_event_loop() {
    Fl::add_idle(event_loop);
    Fl::run();
}

void close_window(Fl_Widget *widget, void *param)
{
    exit(0);
}

void LCD_Init()
{
  int i;
  Fl::visual(FL_RGB);
  main_window = new mywin(640,240);
  image = new image_box(0, 0, 320, 240);
  //fl_font(fl_font(), 5);
  memset(&gui, 0, sizeof(gui));
  for(i = 0; i < 10; i++) {
      //This will leak memory for the labels, but it won't be much
      char *str;
      asprintf(&str, "Ch %d", i);
      gui.raw[i] = new Fl_Output(370, 20 * i + 20, 50, 15, str);
      gui.raw[i]->textsize(10);
      gui.final[i] = new Fl_Output(430, 20 * i + 20, 50, 15);
      gui.final[i]->textsize(10);
      asprintf(&str, "Ch %d", i + 10);
      gui.raw[i+10] = new Fl_Output(530, 20 * i + 20, 50, 15, str);
      gui.raw[i+10]->textsize(10);
      gui.final[i+10] = new Fl_Output(590, 20 * i + 20, 50, 15);
      gui.final[i+10]->textsize(10);
  }
  new Fl_Box(370, 0, 50, 20,"Raw");
  new Fl_Box(430, 0, 50, 20,"Final");
  new Fl_Box(530, 0, 50, 20,"Raw");
  new Fl_Box(590, 0, 50, 20,"Final");
  //Fl_Box box(320, 0, 320, 240);
  //Fl_Output out(320, 0, 30, 10);
  main_window->end();
  main_window->show();
  main_window->callback(close_window);
  //Fl::add_handler(&handler);
  Fl::add_check(update_channels);
  Fl::wait();
  gui.last_redraw = CLOCK_getms();
#ifndef ALT_DRAW
  main_window->make_current();
  gui.image = fl_create_offscreen(320, 240);
  fl_begin_offscreen(gui.image);
  fl_color(FL_BLACK);
  fl_rectf(0, 0, 320, 240);
  fl_end_offscreen();
#endif
}

struct touch SPITouch_GetCoords() {
    //struct touch t = {gui.mousex * 256 / 320, gui.mousey, 0, 0};
    struct touch t = {gui.mousex, gui.mousey, 0, 0};
    return t;
}

int SPITouch_IRQ()
{
#ifndef HAS_EVENT_LOOP
    Fl::check();
#endif
    return gui.mouse;
}

void LCD_DrawStart(void) {
#ifndef HAS_EVENT_LOOP
    Fl::check();
    Fl::flush();
#endif
}
void LCD_DrawStop(void) {
    image->redraw();
    //Fl::redraw();
#ifndef HAS_EVENT_LOOP
    Fl::check();
#endif
}


void LCD_SetDrawArea(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1)
{
    gui.xstart = x0;
    gui.ystart = y0;
    gui.xend   = x1;
    gui.yend   = y1;
    gui.x = x0;
    gui.y = y0;
}

void LCD_DrawPixel(unsigned int color)
{
    u8 r, g, b;
    r = (color >> 8) & 0xf8;
    g = (color >> 3) & 0xfc;
    b = (color << 3) & 0xf8;
#ifdef ALT_DRAW
    gui.image[3*(320*gui.y+gui.x)] = r;
    gui.image[3*(320*gui.y+gui.x)+1] = g;
    gui.image[3*(320*gui.y+gui.x)+2] = b;
    gui.x++;
#else
    fl_begin_offscreen(gui.image);
    Fl_Color c = fl_rgb_color(r, g, b);
    fl_color(c);
    fl_point(gui.x++, gui.y);
    fl_end_offscreen();
#endif
    if(gui.x > gui.xend) {
        gui.x = gui.xstart;
        gui.y++;
    }
}
void LCD_DrawPixelXY(unsigned int x, unsigned int y, unsigned int color)
{
    u8 r, g, b;
    r = (color >> 8) & 0xf8;
    g = (color >> 3) & 0xfc;
    b = (color << 3) & 0xf8;
#ifdef ALT_DRAW
    gui.image[3*(320*y+x)] = r;
    gui.image[3*(320*y+x)+1] = g;
    gui.image[3*(320*y+x)+2] = b;
#else
    fl_begin_offscreen(gui.image);
    Fl_Color c = fl_rgb_color(r, g, b);
    fl_color(c);
    fl_point(x, y);
    fl_end_offscreen();
#endif
}

u32 ScanButtons()
{
    return ~gui.buttons;
}

int PWR_CheckPowerSwitch()
{
    return gui.powerdown;
}

void PWR_Shutdown() {exit(0);}

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

void SPITouch_Calibrate(s32 xscale, s32 yscale, s32 xoff, s32 yoff)
{
    calibration.xscale = xscale;
    calibration.yscale = yscale;
    calibration.xoffset = xoff;
    calibration.yoffset = yoff;
}

u32 CLOCK_getms()
{
    struct timeb tp;
  
    ftime(&tp);
    return (tp.time * 1000) + tp.millitm;
}
}

