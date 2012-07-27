#ifndef _MAIN_PAGE_H_
#define _MAIN_PAGE_H_
#include "buttons.h"

struct main_page {
    struct buttonAction action;
    guiObject_t *optsObj;
    guiObject_t *iconObj;
    guiObject_t *nameObj;
    guiObject_t *trimObj[6];
    guiObject_t *boxObj[8];
    guiObject_t *barObj[8];
    u8 ignore_release;
    s8 trims[6];
    s16 boxval[8];
    s16 barval[8];
    char tmpstr[8];
};

#endif
