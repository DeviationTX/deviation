#ifndef _CALIBRATE_PAGE_H_
#define _CALIBRATE_PAGE_H_

struct calibrate_page {
    u8 enable;
    u8 state;
    guiObject_t *textbox;
    guiObject_t *textbox1;
    struct touch coords;
    struct touch coords1;
    char tmpstr[20];
};
#endif
