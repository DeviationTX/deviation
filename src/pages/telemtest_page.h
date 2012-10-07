#ifndef _TELEMTEST_PAGE_H_
#define _TELEMTEST_PAGE_H_

struct telemtest_page {
    guiObject_t *volt[3];
    guiObject_t *temp[4];
    guiObject_t *rpm[3];
    guiObject_t *line[2];
    char str[25];
};
#endif
