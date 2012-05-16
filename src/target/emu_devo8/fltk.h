#ifndef _FLTK_H_
#define FLTK_H_

#define ALT_DRAW

#ifndef Fl_Output_H
#define Fl_Output void
#endif

struct Gui {
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

extern struct Gui gui;
#endif
