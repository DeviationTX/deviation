#ifndef _TIMER_PAGE_H_
#define _TIMER_PAGE_H_
#include "timer.h"
struct timer_page {
    guiObject_t *startObj[NUM_TIMERS];
    guiObject_t *startLabelObj[NUM_TIMERS];
    char timer[9];
    char tmpstr[10];
};
#endif
