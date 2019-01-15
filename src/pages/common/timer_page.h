#ifndef __TIMER_PAGE_H_
#define __TIMER_PAGE_H_
#include "timer.h"
struct timer_page {
    char timer[9];
    u8 index;
    u8 second, minute, hour;
    u32 oldvalue;
    u32 newvalue;
    s32 hms;
    s32 addset;
};
#endif
