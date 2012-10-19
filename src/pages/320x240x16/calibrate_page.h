#ifndef _CALIBRATE_PAGE_H_
#define _CALIBRATE_PAGE_H_

struct calibrate_page {
    u8 enable;
    u8 state;
    u8 selected;
    guiObject_t *textbox;
    guiObject_t *textbox1;
    struct touch coords;
    struct touch coords1;
    void(*return_page)(int page);
    char tmpstr[30];
};
#endif
