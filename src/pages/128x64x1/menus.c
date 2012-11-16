/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Deviation is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Deviation.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "common.h"
#include "pages.h"
#include "gui/gui.h"
#include "config/model.h"
#include "config/tx.h"

struct menu_pages {
    const char *name;
    u8 menu_depth;
    u8 menu_num;
    const char *pagename;
    u8 pagepos;
};

struct menu_pages menus[] = {
    //main menu
    {_tr_noop("Model config"),       MENUTYPE_MAINMENU, 0, "Menu", (0 << 4) | MENUTYPE_SUBMENU},
    {_tr_noop("Transmitter config"), MENUTYPE_MAINMENU, 1, "Menu", (1 << 4) | MENUTYPE_SUBMENU},
    {_tr_noop("USB"),                MENUTYPE_MAINMENU, 0, "USB",      0},
    {_tr_noop("Telemetry"),          MENUTYPE_MAINMENU, 1, "TeleMoni", PREVIOUS_ITEM},
    // sub menu items 1
    {_tr_noop("Mixer"),              MENUTYPE_SUBMENU,  0, "Mixer",    0},
    {_tr_noop("Model setup"),        MENUTYPE_SUBMENU,  0, "ModelCon", PREVIOUS_ITEM},
    {_tr_noop("Timers"),             MENUTYPE_SUBMENU,  0, "Timer",    PREVIOUS_ITEM},
    {_tr_noop("Telemetry config"),   MENUTYPE_SUBMENU,  0, "TeleConf", PREVIOUS_ITEM},
    {_tr_noop("Trims"),              MENUTYPE_SUBMENU,  0, "Trim",     PREVIOUS_ITEM},
    {_tr_noop("Main page config"),   MENUTYPE_SUBMENU,  0, "MainConf", PREVIOUS_ITEM},
    // sub menu item 2
    {_tr_noop("Basic config"),       MENUTYPE_SUBMENU,  1, "TxConfig", PREVIOUS_ITEM},
    {_tr_noop("Monitor"),            MENUTYPE_SUBMENU,  1, "Monitor",  PREVIOUS_ITEM},
    {_tr_noop("About Deviation"),    MENUTYPE_SUBMENU,  1, "About",    PREVIOUS_ITEM},
    {0, 0, 0, 0, 0}
};

static struct menu_page * const mp = &pagemem.u.menu_page;
#define VIEW_ID 0
static s8 current_selected[3] = {0, 0, 0};  // 0 is used for main menu, 1& 2 are used for sub menu
static s16 view_origin_relativeY;
static u8 menu_type_flag;   // don't put these items into pagemem, which shared the same union struct with other pages and might be changed
static char *last_menu_name;

static u8 action_cb(u32 button, u8 flags, void *data);
static const char *idx_string_cb(guiObject_t *obj, const void *data);
static void menu_press_cb(guiObject_t *obj, s8 press_type, const void *data);
static const char *menu_name_cb(guiObject_t *obj, const void *data);

void PAGE_MenuInit(int page)
{
    PAGE_SetModal(0);
    PAGE_SetActionCB(action_cb);
    GUI_RemoveAllObjects();

    if (page != -1)
        menu_type_flag = (u8)page;
    MenuItemType menu_item_type = menu_type_flag & 0x0f;
    u8 group = (menu_type_flag >> 4) &0x0f;
    if (menu_item_type == MENUTYPE_MAINMENU) {
        PAGE_ShowHeader(_tr("Main menu"));
        mp->current_selected = &current_selected[0];
    }
    else if (menu_item_type ==  MENUTYPE_SUBMENU) {
        PAGE_ShowHeader(_tr(menus[group].name));
        mp->current_selected = &current_selected[group + 1]; // current_selected[0] is used for main menu;1& 2 are used for sub menu
    }

    // Create a logical view
    u8 view_origin_absoluteX = 0;
    u8 view_origin_absoluteY = ITEM_SPACE;
    GUI_SetupLogicalView(VIEW_ID, 0, 0, LCD_WIDTH -ARROW_WIDTH, LCD_HEIGHT - view_origin_absoluteY ,
            view_origin_absoluteX, view_origin_absoluteY);

    u8 row = 0;
    u8 idx = 1;
    guiObject_t *obj;
    labelDesc.style = LABEL_LEFTCENTER;
    for(u8 i=0; menus[i].name != 0; i++) {
        if (menus[i].menu_depth != menu_item_type)
            continue;
        if (menu_item_type == MENUTYPE_SUBMENU &&  group != menus[i].menu_num)
                continue;
        GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
            16, ITEM_HEIGHT,  &TINY_FONT, idx_string_cb, NULL, (void *)(long)idx);
        obj =GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 17), GUI_MapToLogicalView(VIEW_ID, row),
            0, ITEM_HEIGHT, &labelDesc, menu_name_cb, menu_press_cb, (const void *)(long)i);
        GUI_SetSelectable(obj, 1);
        if (idx == 1)
            GUI_SetSelected(obj);

        row += ITEM_SPACE;
        idx++;
    }

    mp->total_items = idx-1;
    if (mp->total_items > PAGE_ITEM_MAX) {
        mp->scroll_bar = GUI_CreateScrollbar(LCD_WIDTH - ARROW_WIDTH, ITEM_HEIGHT,
                LCD_HEIGHT- ITEM_HEIGHT, mp->total_items, NULL, NULL, NULL);
    }
    if (*mp->current_selected >= mp->total_items)  // when users customize sub menu item to main menu item, this scenario happans
        *mp->current_selected = mp->total_items - 1;
    if (*mp->current_selected > 0) {
        s8 temp = *mp->current_selected;
        *mp->current_selected = 0;
        PAGE_NavigateItems(temp, VIEW_ID, mp->total_items, mp->current_selected, &view_origin_relativeY, mp->scroll_bar);
    }
}

const char *menu_name_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    long i = (long)data;
    return _tr(menus[i].name);
}

const char *idx_string_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 idx = (long)data;
    sprintf(mp->tmpstr, "%d.", idx);
    return mp->tmpstr;
}

static u8 action_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            MenuItemType menu_item_type = menu_type_flag & 0x0f;
            u8 group = (menu_type_flag >> 4) &0x0f;
            if (menu_item_type == MENUTYPE_SUBMENU) { // from sub menu back to main menu
                u8 page = group << 4 | MENUTYPE_MAINMENU;
                PAGE_ChangeByName("Menu", page);
            } else  // from main menu back to main page
                PAGE_ChangeByName("MainPage", 0);
        } else if (CHAN_ButtonIsPressed(button, BUT_UP)) {
            PAGE_NavigateItems(-1, VIEW_ID, mp->total_items, mp->current_selected, &view_origin_relativeY, mp->scroll_bar);
        } else if (CHAN_ButtonIsPressed(button, BUT_DOWN)) {
            PAGE_NavigateItems(1, VIEW_ID, mp->total_items, mp->current_selected, &view_origin_relativeY, mp->scroll_bar);
        } else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}

void menu_press_cb(guiObject_t *obj, s8 press_type, const void *data)
{
    (void)obj;
    (void)press_type;
    long i = (long)data;
    last_menu_name = (char *)menus[i].name;
    menu_type_flag = (menus[i].menu_num << 4)| menus[i].menu_depth;
    PAGE_ChangeByName(menus[i].pagename, menus[i].pagepos);
}
