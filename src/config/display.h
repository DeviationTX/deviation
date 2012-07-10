#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include "gui/gui.h"

#define DEFAULT_FONT  (Display.default_font)
#define THROTTLE_FONT (Display.label[0])
#define BATTERY_FONT  (Display.label[1])
#define FONT1         (Display.label[2])
#define NUM_LABELS 3

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
    u16 bar_color;
};

struct display_settings {
    struct FontDesc default_font;
    struct FontDesc label[NUM_LABELS];
    struct disp_keyboard keyboard;
    struct disp_listbox listbox;
};

extern struct display_settings Display;


u8 CONFIG_ReadDisplay();
#endif
