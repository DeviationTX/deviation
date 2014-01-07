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

static struct menu_obj * const gui = &gui_objs.u.menu;

struct menu_pages {
    enum PageID id;
    const char *name;
    u8 menu_depth;
    u8 menu_num;
    MixerMode mixer_mode;
    u8 pagepos;
};

struct menu_pages menus[] = {
    //main menu
    {PAGEID_MENU, _tr_noop("Model menu"),       MENUTYPE_MAINMENU, 0, MIXER_ALL, (0 << 4) | MENUTYPE_SUBMENU},
    {PAGEID_MENU, _tr_noop("Transmitter menu"), MENUTYPE_MAINMENU, 1, MIXER_ALL, (1 << 4) | MENUTYPE_SUBMENU},
    {PAGEID_USB,      NULL, MENUTYPE_MAINMENU, 0, MIXER_ALL, 0},
    {PAGEID_ABOUT,    NULL, MENUTYPE_MAINMENU, 1, MIXER_ALL, PREVIOUS_ITEM},
    // sub menu items 1
    {PAGEID_MODEL,    NULL, MENUTYPE_SUBMENU,  0, MIXER_ALL, PREVIOUS_ITEM},
    {PAGEID_MIXER,    NULL, MENUTYPE_SUBMENU,  0, MIXER_ADVANCED, 0},

#if HAS_STANDARD_GUI
    {PAGEID_REVERSE,  NULL, MENUTYPE_SUBMENU,  0, MIXER_STANDARD, PREVIOUS_ITEM},
    {PAGEID_DREXP,    NULL, MENUTYPE_SUBMENU,  0, MIXER_STANDARD, PREVIOUS_ITEM},
    {PAGEID_SUBTRIM,  NULL, MENUTYPE_SUBMENU,  0, MIXER_STANDARD, PREVIOUS_ITEM},
    {PAGEID_TRAVELADJ,NULL, MENUTYPE_SUBMENU,  0, MIXER_STANDARD, PREVIOUS_ITEM},
    {PAGEID_THROCURVES,NULL,MENUTYPE_SUBMENU,  0, MIXER_STANDARD, PREVIOUS_ITEM},
    {PAGEID_PITCURVES,NULL, MENUTYPE_SUBMENU,  0, MIXER_STANDARD, PREVIOUS_ITEM},
    {PAGEID_THROHOLD, NULL, MENUTYPE_SUBMENU,  0, MIXER_STANDARD, PREVIOUS_ITEM},
    {PAGEID_GYROSENSE,NULL, MENUTYPE_SUBMENU,  0, MIXER_STANDARD, PREVIOUS_ITEM},
    {PAGEID_SWASH,    NULL, MENUTYPE_SUBMENU,  0, MIXER_STANDARD, PREVIOUS_ITEM},
    {PAGEID_FAILSAFE, NULL, MENUTYPE_SUBMENU,  0, MIXER_STANDARD, PREVIOUS_ITEM},
    {PAGEID_SWITCHASSIGN, NULL, MENUTYPE_SUBMENU,  0, MIXER_STANDARD, PREVIOUS_ITEM},
#endif

    {PAGEID_TIMER,    NULL, MENUTYPE_SUBMENU,  0, MIXER_ALL,PREVIOUS_ITEM},
#if HAS_TELEMETRY
    {PAGEID_TELEMCFG, NULL, MENUTYPE_SUBMENU,  0, MIXER_ALL, PREVIOUS_ITEM},
#endif
    {PAGEID_TRIM,     NULL, MENUTYPE_SUBMENU,  0, MIXER_ADVANCED, PREVIOUS_ITEM},
#if HAS_DATALOG
    {PAGEID_DATALOG,  NULL, MENUTYPE_SUBMENU,  0, MIXER_ALL, PREVIOUS_ITEM},
#endif
    {PAGEID_MAINCFG,  NULL, MENUTYPE_SUBMENU,  0, MIXER_ALL, PREVIOUS_ITEM},
    // sub menu item 2
    {PAGEID_TXCFG,    NULL, MENUTYPE_SUBMENU,  1, MIXER_ALL, PREVIOUS_ITEM},
    {PAGEID_CHANMON,  NULL, MENUTYPE_SUBMENU,  1, MIXER_ALL, PREVIOUS_ITEM},
#if HAS_TELEMETRY
    {PAGEID_TELEMMON, NULL, MENUTYPE_SUBMENU,  1, MIXER_ALL, PREVIOUS_ITEM},
#endif
};

static struct menu_page * const mp = &pagemem.u.menu_page;
static const int VIEW_ID = 0;
static u16 current_selected[3] = {0, 0, 0};  // 0 is used for main menu, 1& 2 are used for sub menu
static u8 menu_type_flag;   // don't put these items into pagemem, which shared the same union struct with other pages and might be changed

static u8 action_cb(u32 button, u8 flags, void *data);
static const char *idx_string_cb(guiObject_t *obj, const void *data);
static void menu_press_cb(guiObject_t *obj, s8 press_type, const void *data);
static const char *menu_name_cb(guiObject_t *obj, const void *data);

static guiObject_t *getobj_cb(int relrow, int col, void *data)
{
    (void)col;
    (void)data;
    return  (guiObject_t *)&gui->name[relrow];
}
static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;
    labelDesc.style = LABEL_LEFTCENTER;
    MenuItemType menu_item_type = menu_type_flag & 0x0f;
    u8 group = (menu_type_flag >> 4) &0x0f;
    u8 idx = 0;
    for(u8 i=0; i < sizeof(menus) / sizeof(struct menu_pages); i++) {
        if (menus[i].menu_depth != menu_item_type)
            continue;
        if (menus[i].mixer_mode != MIXER_ALL && menus[i].mixer_mode != Model.mixer_mode)
            continue;
        if (menu_item_type == MENUTYPE_SUBMENU &&  group != menus[i].menu_num)
            continue;
        if (idx == absrow) {
            GUI_CreateLabelBox(&gui->idx[relrow], 0, y,
                0, 0, &DEFAULT_FONT, idx_string_cb, NULL, (void *)(absrow+ 1L));
            GUI_CreateLabelBox(&gui->name[relrow], ITEM_SPACE*2, y,
                0, 0, &DEFAULT_FONT, menu_name_cb, menu_press_cb, (const void *)(long)i);
            break;
        }
        idx++;
    }
    return 1;
}

void PAGE_MenuInit(int page)
{
    PAGE_SetModal(0);
    PAGE_SetActionCB(action_cb);
    PAGE_RemoveAllObjects();

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

    u8 idx = 0;
    for(u8 i=0; i < sizeof(menus) / sizeof(struct menu_pages); i++) {
        if (menus[i].menu_depth != menu_item_type)
            continue;
        if (menus[i].mixer_mode != MIXER_ALL && menus[i].mixer_mode != Model.mixer_mode)
            continue;
        if (menu_item_type == MENUTYPE_SUBMENU &&  group != menus[i].menu_num)
            continue;
        idx++;
    }

    GUI_CreateScrollable(&gui->scrollable, 0, ITEM_HEIGHT, LCD_WIDTH, LCD_HEIGHT,
                     ITEM_SPACE, idx, row_cb, getobj_cb, NULL, NULL);

    GUI_SetSelected(GUI_ShowScrollableRowOffset(&gui->scrollable, *mp->current_selected));
}

void PAGE_MenuExit()
{
    *mp->current_selected = GUI_ScrollableGetObjRowOffset(&gui->scrollable, GUI_GetSelected());
}

const char *menu_name_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    long i = (long)data;
    if (menus[i].name != NULL)
        return _tr(menus[i].name);
    return PAGE_GetName(menus[i].id);
}

const char *idx_string_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 idx = (long)data;
    sprintf(tempstring, "%d.", idx);
    return tempstring;
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
                PAGE_ChangeByID(PAGEID_MENU, page);
            } else  // from main menu back to main page
                PAGE_ChangeByID(PAGEID_MAIN, 0);
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
    if (press_type == -1) {
        long i = (long)data;
        menu_type_flag = (menus[i].menu_num << 4)| menus[i].menu_depth;
        PAGE_ChangeByID(menus[i].id, menus[i].pagepos);
    }
}
