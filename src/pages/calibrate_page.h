#ifndef _CALIBRATE_PAGE_H_
#define _CALIBRATE_PAGE_H_

struct calibrate_page {
    u8 enable;
    guiObject_t *textbox;
    struct touch coords;
    char tmpstr[20];
};
#endif
