#ifndef _CHANTEST_PAGE_H_
#define _CHANTEST_PAGE_H_

struct chantest_page {
    guiObject_t *bar[NUM_CHANNELS];
    guiObject_t *value[NUM_CHANNELS];
    s8 pctvalue[NUM_CHANNELS];
    char tmpstr[10];
};

#endif
