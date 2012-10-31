#ifndef _TELEMCONFIG_PAGE_H_
#define _TELEMCONFIG_PAGE_H_
#include "telemetry.h"
struct telemconfig_page {
    guiObject_t *valueObj[6];
    char str[25];
};
#endif
