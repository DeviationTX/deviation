#ifndef _MAIN_PAGE_H_
#define _MAIN_PAGE_H_
#include "buttons.h"

struct main_page {
    struct buttonAction action;
    u8 ignore_release;
    s16 battery;
    s8 trims[6];
    s32 boxval[8];
    s16 barval[8];
    char tmpstr[8];
};

#endif
