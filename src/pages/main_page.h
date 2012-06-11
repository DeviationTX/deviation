#ifndef _MAIN_PAGE_H_
#define _MAIN_PAGE_H_

#define TRIMS_TO_SHOW 4
struct main_page {
    guiObject_t *trimObj[TRIMS_TO_SHOW];
    guiObject_t *throttleObj;
    s16 throttle;
    s8 trims[TRIMS_TO_SHOW];
    char tmpstr[8];
};

#endif
