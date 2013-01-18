#ifndef _TELEMTEST_PAGE_H_
#define _TELEMTEST_PAGE_H_
#include "telemetry.h"
struct telemtest_page {
    void(*return_page)(int page);
    int return_val;
    char str[60];
    struct Telemetry telem;
};
#endif
