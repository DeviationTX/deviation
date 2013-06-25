#ifndef _RTC_CONFIG_H_
#define _RTC_CONFIG_H_

#include "rtc.h"
typedef enum {
    PLL, HSE, HSI, LSE, LSI
} clocksource_t;

struct rtc_page {
    u16 value[6];
    char tmpstr[12];
    void (*return_page)(int page);
    int return_val;
};

#endif
