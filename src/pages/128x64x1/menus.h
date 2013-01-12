#ifndef _MENUS_H__
#define _MENUS_H__

typedef enum
{
    MENUTYPE_MAINMENU = 0,
    MENUTYPE_SUBMENU = 1,
} MenuItemType;

struct menu_page {
    u16 *current_selected;
    char tmpstr[20];
};

#endif /* MENUS_H_ */
