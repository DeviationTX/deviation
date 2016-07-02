#ifndef __CHANTEST_PAGE_H_
#define __CHANTEST_PAGE_H_
#include "buttons.h"
#define NUM_TEST_BARS ((NUM_CHANNELS > NUM_INPUTS) ? NUM_CHANNELS : NUM_INPUTS)
#define NUM_VALUES ((NUM_TX_BUTTONS > NUM_TEST_BARS) ? NUM_TX_BUTTONS : NUM_TEST_BARS)

typedef enum  {
    MONITOR_MIXEROUTPUT = 0,
    MONITOR_RAWINPUT,
    MONITOR_BUTTONTEST,
} monitor_type;

struct chantest_page {
    monitor_type type;
    u8 num_bars;
    u8 is_locked;
    s16 pctvalue[NUM_TEST_BARS];
    struct buttonAction action;
};

#endif
