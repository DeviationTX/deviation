#ifndef _MAIN_PAGE_H_
#define _MAIN_PAGE_H_

#define TRIMS_TO_SHOW 6
struct main_page {
    guiObject_t *trimObj[TRIMS_TO_SHOW];
    guiObject_t *throttleObj;
    guiObject_t *pitchObj;
    guiObject_t *timerObj;
    guiObject_t *telemetryObj;
    guiObject_t *iconObj;
    guiObject_t *nameObj;
    s16 throttle;
    u8 ignore_release;
    u16 timer[2];
    s8 trims[TRIMS_TO_SHOW];
    char tmpstr[8];
};

#endif
