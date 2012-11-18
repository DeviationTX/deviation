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
#define _WIN32_WINNT 0x0500

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/timeb.h>
#include <time.h>
#include <errno.h>
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
#include <FL/fl_ask.H>

extern "C" {
#include "common.h"
#include "fltk.h"
#include "mixer.h"
#include "config/tx.h"
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
            case 't':
                if(++gui.aux4 > 10)
                    gui.aux4 = 10;
                return 1;
            case 'g':
                if(--gui.aux4 < 0)
                    gui.aux4 = 0;
                return 1;
            case 'y':
                if(++gui.aux5 > 10)
                    gui.aux5 = 10;
                return 1;
            case 'h':
                if(--gui.aux5 < 0)
                    gui.aux5 = 0;
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
      fl_draw_image(gui.image, x(), y(), w(), h(), 3, 0);
    }
};

void update_channels(void *params)
{
    (void)params;
    char str[80];
    int changed = 0;
    if(! gui.raw[0])
        return;

    for(int i = 0; i < INP_LAST -1; i++) {
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

void set_stick_positions()
{
    gui.throttle = 5;
    gui.elevator = 5;
    gui.aileron  = 5;
    gui.rudder   = 5;
    switch(Transmitter.mode) {
    case MODE_1:
    case MODE_3:
       gui.throttle = 0;
       break;
    case MODE_2:
    case MODE_4:
       gui.elevator = 0;
       break;
    }
}

void close_window(Fl_Widget *widget, void *param)
{
    (void)widget;
    (void)param;
    char tmp[256];
    sprintf(tmp, "%s && %s\n", _tr("Save"), _tr("Exit"));
    if(0 == fl_choice(" ", tmp, _tr("Exit"), NULL)) {
        CONFIG_SaveModelIfNeeded();
        CONFIG_SaveTxIfNeeded();
    }
    exit(0);
}

void LCD_Init()
{
  int i;
  Fl::visual(FL_RGB);
  // 85 is for 4 rows' height
  int lcdScreenWidth = LCD_WIDTH * LCD_WIDTH_MULT;
  int lcdScreenHeight = LCD_HEIGHT * LCD_HEIGHT_MULT;

  int height = (lcdScreenHeight > INP_LAST + (INP_LAST - 1) * 10 + 85) ?
                lcdScreenHeight : INP_LAST + (INP_LAST - 1) * 10 + 85;
  main_window = new mywin(lcdScreenWidth + 320,height);
  image = new image_box(0, (height - lcdScreenHeight) / 2, lcdScreenWidth, lcdScreenHeight);
  //fl_font(fl_font(), 5);
  memset(&gui, 0, sizeof(gui));
  int mid = (INP_LAST - 1) / 2;
  for(i = 0; i < mid; i++) {
      char *label;
      label = (char *)malloc(10);
      INPUT_SourceName(label, i + 1);
      gui.raw[i] = new Fl_Output(lcdScreenWidth + 90, 20 * i + 5, 60, 15, i < 4 ? tx_stick_names[i] : label);
      gui.raw[i]->textsize(10);
      label = (char *)malloc(10);
      INPUT_SourceName(label, i + mid + 1);
      gui.raw[i+mid] = new Fl_Output(lcdScreenWidth + 230, 20 * i + 5, 60, 15, label);
      gui.raw[i+mid]->textsize(10);
  }
  for(i = 0; i < 4; i++) {
      //This will leak memory for the labels, but it won't be much
      char *str;
      str = (char *)malloc(5);
      sprintf(str, "Ch%d", i + 1);
      gui.final[i] = new Fl_Output(lcdScreenWidth + 50, height - (4 - i) * 20 - 5, 50, 15, str);
      gui.final[i]->textsize(10);
      str = (char *)malloc(5);
      sprintf(str, "Ch%d", i + 5);
      gui.final[i+4] = new Fl_Output(lcdScreenWidth + 150, height - (4 - i) * 20 - 5, 50, 15, str);
      gui.final[i+4]->textsize(10);
      str = (char *)malloc(5);
      sprintf(str, "Ch%d", i + 9);
      gui.final[i+8] = new Fl_Output(lcdScreenWidth + 260, height - (4 - i) * 20 - 5, 50, 15, str);
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
  gui.init = 1;
}

struct touch SPITouch_GetCoords() {
    //struct touch t = {gui.mousex * 256 / 320, gui.mousey, 0, 0};
    struct touch t = {gui.mousex, gui.mousey, 0, 0};
    return t;
}

int SPITouch_IRQ()
{
#ifndef HAS_EVENT_LOOP
    //Fl::check();
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
    //Fl::check();
    return gui.buttons;
}

int PWR_CheckPowerSwitch()
{
    Fl::check();
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

u32 msecs = 0;
enum {
    TIMER_ENABLE = LAST_PRIORITY,
    NUM_MSEC_CALLBACKS,
};
u32 msec_cbtime[NUM_MSEC_CALLBACKS];
u8 timer_enable;
void ALARMhandler()
{
    msecs++;
    //if(msecs % 1000 == 0) printf("msecs %d\n", msecs);

    if(timer_callback && timer_enable & (1 << TIMER_ENABLE) && msecs == msec_cbtime[TIMER_ENABLE]) {
        u16 us = timer_callback();
        if (us > 0) {
            msec_cbtime[TIMER_ENABLE] += us;
        }
    }
    if(timer_enable & (1 << MEDIUM_PRIORITY) && msecs == msec_cbtime[MEDIUM_PRIORITY]) {
        medium_priority_cb();
        priority_ready |= 1 << MEDIUM_PRIORITY;
        msec_cbtime[MEDIUM_PRIORITY] += MEDIUM_PRIORITY_MSEC;
    }
    if(timer_enable & (1 << LOW_PRIORITY) && msecs == msec_cbtime[LOW_PRIORITY]) {
        priority_ready |= 1 << LOW_PRIORITY;
        msec_cbtime[LOW_PRIORITY] += LOW_PRIORITY_MSEC;
    }
}

#ifndef WIN32
void _ALARMhandler(int sig) {
    (void)sig;
    ALARMhandler();
}

void CLOCK_Init()
{
    int Ret;
    timer_callback = NULL;
    signal(SIGALRM, _ALARMhandler);
     //create a new timer.
    timer_t timerid;
    struct sigevent sig;
    sig.sigev_notify = SIGEV_SIGNAL;
    sig.sigev_signo= SIGALRM;

    Ret = timer_create(CLOCK_REALTIME, &sig, &timerid);
    if(Ret != 0) {
        printf("timer_create() failed with %d\n", errno);
        exit(1);
    }
    struct itimerspec in, out;
    in.it_value.tv_sec = 0;
    in.it_value.tv_nsec = 100 * 1000 * 1000; //100msec
    in.it_interval.tv_sec = 0;
    in.it_interval.tv_nsec = 1 * 1000 * 1000; //1msec
    //issue the periodic timer request here.
    Ret = timer_settime(timerid, 0, &in, &out);
    if(Ret != 0) {
        printf("timer_settime() failed with %d\n", errno);
        exit(1);
    }
}
#else
HANDLE mainThread;
void CALLBACK TimerProc(void* lpParametar, BOOLEAN TimerOrWaitFired)
{
    (void)lpParametar;
    (void)TimerOrWaitFired;
    SuspendThread(mainThread);
    ALARMhandler();
    ResumeThread(mainThread);
}

void CLOCK_Init()
{
    mainThread = OpenThread(THREAD_ALL_ACCESS, FALSE, GetCurrentThreadId());
    timer_callback = NULL;
    HANDLE m_timerHandle;
    BOOL success = CreateTimerQueueTimer(&m_timerHandle, NULL, TimerProc,
                                         NULL, 100, 1, WT_EXECUTEINTIMERTHREAD);
    if(!success) {
        printf("Couldn't start timer\n");
        exit(1);
    }
}
#endif
void CLOCK_StartTimer(u16 us, u16 (*cb)(void))
{
    timer_callback = cb;
    msec_cbtime[TIMER_ENABLE] = msecs + us;
    timer_enable |= 1 << TIMER_ENABLE;
}

void CLOCK_StopTimer()
{
    timer_enable &= ~(1 << TIMER_ENABLE);
}
void CLOCK_SetMsecCallback(int cb, u32 msec)
{
    msec_cbtime[cb] = msecs + msec;
    timer_enable |= 1 << cb;
}

u32 CLOCK_getms()
{
    return msecs;
#if 0
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
#endif
}
}
