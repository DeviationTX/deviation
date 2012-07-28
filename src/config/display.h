#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include "gui/gui.h"

#define DEFAULT_FONT   (Display.font[0])
#define MODELNAME_FONT (Display.font[1])
#define TITLE_FONT     (Display.font[2])
#define BIGBOX_FONT    (Display.font[3])
#define SMALLBOX_FONT  (Display.font[4])
#define BATTERY_FONT   (Display.font[5])
#define TINY_FONT      (Display.font[6])
#define BOLD_FONT      (Display.font[7])
#define NARROW_FONT    (Display.font[8])
#define SMALL_FONT     (Display.font[9])
#define NUM_LABELS 10

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

struct disp_bargraph {
    u16 bg_color;
    u16 fg_color;
    u16 outline_color;
};
struct disp_scrollbar {
    u16 bg_color;
    u16 fg_color;
};

enum DispFlags {
    BAR_TRANSPARENT   = 0x01,
    TRIM_TRANSPARENT  = 0x02,
    SHOW_BAT_ICON     = 0x80,
};

struct display_settings {
    struct LabelDesc font[NUM_LABELS];
    struct disp_keyboard keyboard;
    struct disp_listbox listbox;
    struct disp_scrollbar scrollbar;
    struct disp_bargraph bargraph;
    struct disp_bargraph trim;
    u16 select_color;
    u8 select_width;
    enum DispFlags flags;
};

extern struct display_settings Display;


u8 CONFIG_ReadDisplay();
#endif
