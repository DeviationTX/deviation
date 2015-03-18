#ifndef _FLTK_H_
#define _FLTK_H_

#include "gui/gui.h"
#include "emu.h"

#ifndef Fl_Output_H
#define Fl_Output void
#endif

//#define KEYBOARD_LAYOUT_QWERTZ 1

struct Gui {
    u16 xstart, xend, ystart, yend;
    u16 x, y;
    s8 dir;
    u32 buttons;
    int  throttle;
    int  rudder;
    int  elevator;
    int  aileron;
	int  aux2;
	int  aux3;
	int  aux4;
	int  aux5;
	int  aux6;
	int  aux7;
    int  rud_dr;
    int  ail_dr;
    int  ele_dr;
    int  dr;   /* for emu_devo6 */
    int  gear;
    int  mix;
    int  fmod;
	int  hold;
	int  trn;

    u8  powerdown;
    u8  mouse;
    u16 mousex, mousey;
    Fl_Output *raw[INP_LAST-1];
    Fl_Output *final[12];
    u32 last_redraw;
    u8  image[LCD_WIDTH*LCD_WIDTH_MULT*LCD_HEIGHT*LCD_HEIGHT_MULT*3];
    u8 init;
} gui;

extern struct Gui gui;
void set_stick_positions();

#endif
