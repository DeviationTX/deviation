#ifndef _FLTK_H_
#define _FLTK_H_

#include "gui/gui.h"
#include "emu.h"

#ifndef Fl_Output_H
#define Fl_Output void
#endif

#ifndef PIXELS_PER_LOGICAL_X
    #define PIXELS_PER_LOGICAL_X 1
#endif
#ifndef PIXELS_PER_LOGICAL_Y
    #define PIXELS_PER_LOGICAL_Y 1
#endif
#if !defined ZOOM_X || !defined ZOOM_Y
    #define ZOOM_X 1
    #define ZOOM_Y 1
#endif

#define IMAGE_X   LCD_WIDTH  * PIXELS_PER_LOGICAL_X
#define IMAGE_Y   LCD_HEIGHT * PIXELS_PER_LOGICAL_Y

#define SCREEN_X (IMAGE_X * ZOOM_X)
#define SCREEN_Y (IMAGE_Y * ZOOM_Y)

#define SCREEN_RESIZE (IMAGE_X != SCREEN_X || IMAGE_Y != SCREEN_Y)
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
    u8  image[IMAGE_X*IMAGE_Y*3];
#if SCREEN_RESIZE
    u8 scaled_img[SCREEN_X*SCREEN_Y*3];
#endif
    u8 init;
} gui;

extern struct Gui gui;
void set_stick_positions();

#endif
