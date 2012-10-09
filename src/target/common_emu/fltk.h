#ifndef _FLTK_H_
#define FLTK_H_

#include "emu.h"

#ifndef Fl_Output_H
#define Fl_Output void
#endif

struct Gui {
    u16 xstart, xend, ystart, yend;
    u16 x, y;
    s8 dir;
    u32 buttons;
    int  throttle;
    int  rudder;
    int  elevator;
    int  aileron;
    int  rud_dr;
    int  ail_dr;
    int  ele_dr;
    int  gear;
    int  mix;
    int  fmod;
    u8  powerdown;
    u8  mouse;
    u16 mousex, mousey;
    Fl_Output *raw[20];
    Fl_Output *final[20];
    u32 last_redraw;
    u8  image[LCD_WIDTH*LCD_HEIGHT*3];
    u8 init;
} gui;

extern struct Gui gui;
void set_stick_positions();

#endif
