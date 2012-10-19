#ifndef _SUB_MENU_H_
#define _SUB_MENU_H_
#include "buttons.h"

struct sub_menu_page {
    struct buttonAction action;
    guiObject_t *menuItemObj[PAGE_ITEM_COUNT];
    guiObject_t *scrollBar1;
    guiObject_t *scrollBar2;
    char tmpstr[8];
};

#endif
