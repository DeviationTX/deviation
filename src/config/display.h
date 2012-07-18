#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include "gui/gui.h"

#define DEFAULT_FONT   (Display.font[0])
#define MODELNAME_FONT (Display.font[1])
#define THROTTLE_FONT  (Display.font[2])
#define TIMER_FONT     (Display.font[3])
#define BATTERY_FONT   (Display.font[4])
#define MISC1_FONT     (Display.font[5])
#define NUM_LABELS 6

struct disp_keyboard {
    u8 font;
    u16 fill_color;
    u16 fg_key1;
    u16 bg_key1;
    u16 fg_key2;
    u16 bg_key2;
    u16 fg_key3;
    u16 bg_key3;
};

struct disp_listbox {
    u16 font;
    u16 bg_color;
    u16 fg_color;
    u16 bg_select;
    u16 fg_select;
    u16 bg_bar;
    u16 fg_bar;
};

struct display_settings {
    struct LabelDesc font[NUM_LABELS];
    struct disp_keyboard keyboard;
    struct disp_listbox listbox;
    u16 select_color;
    u8 show_bat_icon;
};

extern struct display_settings Display;


u8 CONFIG_ReadDisplay();
#endif
