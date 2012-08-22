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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/timeb.h>
#include <time.h>
#ifndef WIN32
#include <signal.h>
#include <unistd.h>
#endif


#include <FL/Fl.H>
#include <FL/x.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Output.H>
#include <FL/fl_draw.H>

extern "C" {
#include "target.h"
#include "fltk.h"
#include "mixer.h"
}


static const u16 keymap[] = { 'A', 'Q', 'S', 'W', 'D', 'E', 'F', 'R', 'G', 'T', 'H', 'Y', FL_Left, FL_Right, FL_Down, FL_Up, 13/*FL_Enter*/, FL_Escape, 0 };

static struct {
    s32 xscale;
    s32 yscale;
    s32 xoffset;
    s32 yoffset;
} calibration = {0x10000, 0x10000, 0, 0};

static Fl_Window *main_window;
static Fl_Box    *image;
static u16 (*timer_callback)(void);
#ifdef WIN32
static u16 alarmtime;
static u32 lastalarm;
#endif
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
        if(Fl::event_shift() && key >= 'a') {
            key -= 'a' - 'A';
        }
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
        const char *k;
        switch(event) {
        case FL_FOCUS:
        case FL_UNFOCUS:
            return 1;
        case FL_KEYDOWN:
        //case FL_SHORTCUT:
            redraw();
            k = Fl::event_text();
            key = get_button(k[0]);
            if(key < 0)
                key = get_button(Fl::event_key());
            if(key >= 0) {
                gui.buttons |= (1 << key);
                return 1;
            }
			if (!strcmp(k, "\\") || !strcmp(k, "\'")) {
                gui.powerdown = 1;
                return 1;
            }
            switch(Fl::event_key()) {
            case '\'':
            case '\\':
                gui.powerdown = 1;
                return 1;
            case 'q':
                if(++gui.elevator > 10)
                    gui.elevator = 10;
                return 1;
            case 'a':
                if(--gui.elevator < 0)
                    gui.elevator = 0;
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
                if(++gui.throttle > 10)
                    gui.throttle = 10;
                return 1;
            case 'd':
                if(--gui.throttle < 0)
                    gui.throttle = 0;
                return 1;
            case 'r':
                if(++gui.aileron > 10)
                    gui.aileron = 10;
                return 1;
            case 'f':
                if(--gui.aileron < 0)
                    gui.aileron = 0;
                return 1;
            case 'z':
                gui.gear = ! gui.gear;
                return 1;
            case 'x':
                gui.rud_dr = ! gui.rud_dr;
                return 1;
            case 'c':
                gui.ele_dr = ! gui.ele_dr;
                return 1;
            case 'v':
                gui.ail_dr = ! gui.ail_dr;
                return 1;
            case 'b':
                gui.mix = (gui.mix + 1) % 3;
                return 1;
            case 'n':
                gui.fmod = (gui.fmod + 1) % 3;
                return 1;
            }
        case FL_KEYUP:
            k = Fl::event_text();
            key = get_button(k[0]);
            if(key < 0)
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
    if(! gui.raw[0])
        return;

    for(int i = 0; i < 14; i++) {
        sprintf(str, "%6.2f%%", 100.0 * (float)CHAN_ReadInput(i + 1) / CHAN_MAX_VALUE);
        if(strcmp(str, gui.raw[i]->value()))
            changed |= gui.raw[i]->value(str);
    }
    for(int i = 0; i < 12; i++) {
        sprintf(str, "%6.2f%%", 100.0 * (float)Channels[i] / CHAN_MAX_VALUE);
        if(strcmp(str, gui.final[i]->value()))
            changed |= gui.final[i]->value(str);
    }
}
extern "C" {
extern void EventLoop(void *);
void start_event_loop() {
    Fl::add_idle(EventLoop);
    Fl::run();
}

void close_window(Fl_Widget *widget, void *param)
{
    (void)widget;
    (void)param;
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
  for(i = 0; i < 7; i++) {
      char *label;
      label = (char *)malloc(10);
      INPUT_SourceName(label, i + 1);
      gui.raw[i] = new Fl_Output(395, 20 * i + 5, 60, 15, i < 4 ? tx_stick_names[i] : label);
      gui.raw[i]->textsize(10);
      label = (char *)malloc(10);
      INPUT_SourceName(label, i + 8);
      gui.raw[i+7] = new Fl_Output(535, 20 * i + 5, 60, 15, label);
      gui.raw[i+7]->textsize(10);
  }
  for(i = 0; i < 4; i++) {
      //This will leak memory for the labels, but it won't be much
      char *str;
      str = (char *)malloc(5);
      sprintf(str, "Ch%d", i + 1);
      gui.final[i] = new Fl_Output(370, 20 * i + 20*7 + 15, 50, 15, str);
      gui.final[i]->textsize(10);
      str = (char *)malloc(5);
      sprintf(str, "Ch%d", i + 5);
      gui.final[i+4] = new Fl_Output(470, 20 * i + 20*7+ 15, 50, 15, str);
      gui.final[i+4]->textsize(10);
      str = (char *)malloc(5);
      sprintf(str, "Ch%d", i + 9);
      gui.final[i+8] = new Fl_Output(580, 20 * i + 20*7+ 15, 50, 15, str);
      gui.final[i+8]->textsize(10);
  }
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

void LCD_DrawStart(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, enum DrawDir dir)
{
    gui.xstart = x0;
    gui.ystart = y0;
    gui.xend   = x1;
    gui.yend   = y1;
    gui.x = x0;
    if (dir == DRAW_NWSE) {
        gui.y = y0;
        gui.dir = 1;
    } else if (dir == DRAW_SWNE) {
        gui.y = y1;
        gui.dir = -1;
    }
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
#else
    fl_begin_offscreen(gui.image);
    Fl_Color c = fl_rgb_color(r, g, b);
    fl_color(c);
    fl_point(gui.x, gui.y);
    fl_end_offscreen();
#endif
    gui.x++;
    if(gui.x > gui.xend) {
        gui.x = gui.xstart;
        gui.y += gui.dir;
    }
}
void LCD_DrawPixelXY(unsigned int x, unsigned int y, unsigned int color)
{
    gui.xstart = x; //This is to emulate how the LCD behaves
    gui.x = x;
    gui.y = y;
    LCD_DrawPixel(color);
}

u32 ScanButtons()
{
    //Force rescan
    Fl::check();
    return gui.buttons;
}

int PWR_CheckPowerSwitch()
{
    return gui.powerdown;
}

void PWR_Shutdown() {exit(0);}

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

#ifndef WIN32
void ALARMhandler(int sig)
{
    (void)sig;
    signal(SIGALRM, SIG_IGN);          /* ignore this signal       */
    if(timer_callback) {
        u16 us = timer_callback();
        if (us > 0) {
            printf("Sleeping: %dus\n", us);
            u16 alarmtime = 1; //us > 1000 ? us /= 1000 : 1;
            signal(SIGALRM, ALARMhandler);     /* reinstall the handler    */
            alarm(alarmtime);
        }
    }
}

void CLOCK_StartTimer(u16 us, u16 (*cb)(void))
{
    u16 alarmtime = us > 1000 ? us /= 1000 : 1;
    timer_callback = cb;
    signal(SIGALRM, ALARMhandler);
    alarm(alarmtime);
}

void CLOCK_StopTimer()
{
    signal(SIGALRM, SIG_IGN);          /* ignore this signal       */
    alarm(0);
}
#else
void CLOCK_StartTimer(u16 us, u16 (*cb)(void))
{
    struct timeb tp;
    ftime(&tp);
    lastalarm = (tp.time * 1000) + tp.millitm;
    alarmtime = us;
    timer_callback = cb;
}

void CLOCK_StopTimer()
{
    timer_callback = NULL;
}
#endif
void CLOCK_Init()
{
    timer_callback = NULL;
}

u32 CLOCK_getms()
{
    struct timeb tp;
    u32 t;
    ftime(&tp);
    t = (tp.time * 1000) + tp.millitm;
#ifdef WIN32
    if (timer_callback && alarmtime && (t - lastalarm > alarmtime)) {
        lastalarm = t;
        alarmtime = timer_callback();
    }
#endif        
    return t;
}
}
