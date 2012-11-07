#ifndef _MAIN_MENU_H_
#define _MAIN_MENU_H_
#include "buttons.h"

#define MENU_ITEM_WIDTH (LCD_WIDTH/2-2)
#define PAGE_ITEM_COUNT 4  // how many menu items can be shown in current page view
//#define MAIN_MENU_ITEM_COUNT (PAGE_ITEM_COUNT + PAGE_ITEM_COUNT)
struct main_menu_page {
    struct buttonAction action;
    char **menu_item_name;
    u8 main_menu_count;
    guiObject_t *menuItemObj[PAGE_ITEM_COUNT + PAGE_ITEM_COUNT];
};

#endif

extern const char *PAGE_menuitem_cb(guiObject_t *obj, const void *data); // external to others
