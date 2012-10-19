#ifndef _MAIN_MENU_H_
#define _MAIN_MENU_H_
#include "buttons.h"

#define MENU_ITEM_WIDTH (LCD_WIDTH/2-2)
#define MENU_ITEM_HEIGHT 15
#define MENU_ITEM_START_ROW (MENU_ITEM_HEIGHT+1)
#define PAGE_ITEM_COUNT 3  // how many menu items can be shown in current page view
#define MAIN_MENU_ITEM_COUNT (PAGE_ITEM_COUNT + PAGE_ITEM_COUNT) 
struct main_menu_page {
    struct buttonAction action;
    guiObject_t *menuItemObj[MAIN_MENU_ITEM_COUNT];
};

#endif

extern const char *PAGE_menuitem_cb(guiObject_t *obj, const void *data); // external to others
