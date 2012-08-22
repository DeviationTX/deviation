#ifndef _CHANTEST_PAGE_H_
#define _CHANTEST_PAGE_H_
#include "buttons.h"
#define NUM_TEST_BARS ((NUM_CHANNELS > NUM_INPUTS) ? NUM_CHANNELS : NUM_INPUTS)
#define NUM_VALUES ((NUM_TX_BUTTONS > NUM_TEST_BARS) ? NUM_TX_BUTTONS : NUM_TEST_BARS)

struct chantest_page {
    u8 type;
    u8 num_bars;
    u8 is_locked;
    void(*return_page)(int page);
    guiObject_t *bar[NUM_TEST_BARS];
    guiObject_t *value[NUM_VALUES];
    s8 pctvalue[NUM_TEST_BARS];
    char tmpstr[10];
    struct buttonAction action;
};

#endif
