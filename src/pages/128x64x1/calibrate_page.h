#ifndef _CALIBRATE_PAGE_H_
#define _CALIBRATE_PAGE_H_

struct calibrate_page {
    u8 selected;
    void(*return_page)(int page);


	struct buttonAction action;
    guiObject_t *label;
    char tmpstr[60];
};
#endif
