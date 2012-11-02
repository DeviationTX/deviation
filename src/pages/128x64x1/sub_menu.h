#ifndef _SUB_MENU_H_
#define _SUB_MENU_H_
#include "buttons.h"

struct sub_menu_page {
    struct buttonAction action;
    u8 menu_item_count;
    u8 sub_menu_item;  // global variable to let other page get back to the right sub menu
    char **menu_item_name;
    guiObject_t *menuItemObj[PAGE_ITEM_COUNT];
    guiObject_t *scroll_bar;
    char tmpstr[8];
};

#endif
