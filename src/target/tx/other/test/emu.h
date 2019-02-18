#ifndef _EMU_H_
#define _EMU_H_

#include "gui/gui.h"

#define LCD_WIDTH_MULT 1
#define LCD_HEIGHT_MULT 1
#define EMU_STRING "Unittest"
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
    u32 last_redraw;
    u8  image[IMAGE_X*IMAGE_Y*3];
#if SCREEN_RESIZE
    u8 scaled_img[SCREEN_X*SCREEN_Y*3];
#endif
    u8 init;
} gui;


