#ifndef _TELEMTEST_PAGE_H_
#define _TELEMTEST_PAGE_H_
#include "telemetry.h"
struct telemtest_page {
    void(*return_page)(int page);
    int return_val;
    guiObject_t *volt[3];
    guiObject_t *temp[4];
    guiObject_t *rpm[3];
    guiObject_t *gps[5];
    char str[60];
    struct Telemetry telem;
};
#endif
