#ifndef _RTC_CONFIG_H_
#define _RTC_CONFIG_H_

typedef enum {
    PLL, HSE, HSI, LSE, LSI
} clocksource_t;

struct rtc_page {
    u16 value[6];
};

#endif
