#ifndef _MAIN_PAGE_H_
#define _MAIN_PAGE_H_
#include "buttons.h"

struct main_page {
    struct buttonAction action;
    u8 ignore_release;
    s16 battery;
    s32 elem[NUM_ELEMS];
    char tmpstr[8];
#if HAS_RTC
    u32 time;
#endif
};

#endif
