#ifndef _RTC_CONFIG_H_
#define _RTC_CONFIG_H_

#include "rtc.h"
typedef enum {
    PLL, HSE, HSI, LSE, LSI
} clocksource_t;

struct rtc_page {
    u16 value[6];
    int clocksource;
    int prescaler;
    int clocksourceready;
    int min[6], max[6];     // for reordering the values are set dynamically
    u8 order[6];
};

#endif
