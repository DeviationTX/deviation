#ifndef __CHANTEST_PAGE_H_
#define __CHANTEST_PAGE_H_
#include "buttons.h"
#define NUM_TEST_BARS ((NUM_CHANNELS > NUM_INPUTS) ? NUM_CHANNELS : NUM_INPUTS)
#define NUM_VALUES ((NUM_TX_BUTTONS > NUM_TEST_BARS) ? NUM_TX_BUTTONS : NUM_TEST_BARS)

typedef enum  {
    MONITOR_CHANNELOUTPUT = 0,
    MONITOR_RAWINPUT,
    MONITOR_BUTTONTEST,
} monitor_type;

struct chantest_page {
    monitor_type type;
    u8 num_bars;
    u8 is_locked;
    void(*return_page)(int page);
    int return_val;
    guiObject_t *bar[NUM_TEST_BARS];
    guiObject_t *value[NUM_VALUES];
    s16 pctvalue[NUM_TEST_BARS];
    char tmpstr[31];
    struct buttonAction action;
};

#endif
