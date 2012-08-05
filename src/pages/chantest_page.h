#ifndef _CHANTEST_PAGE_H_
#define _CHANTEST_PAGE_H_
#if NUM_CHANNELS > NUM_INPUTS
    #define NUM_TEST_BARS NUM_CHANNELS
#else
    #define NUM_TEST_BARS NUM_INPUTS
#endif
struct chantest_page {
    u8 type;
    u8 num_bars;
    void(*return_page)(int page);
    guiObject_t *bar[NUM_TEST_BARS];
    guiObject_t *value[NUM_TEST_BARS];
    s8 pctvalue[NUM_TEST_BARS];
    char tmpstr[10];
};

#endif
