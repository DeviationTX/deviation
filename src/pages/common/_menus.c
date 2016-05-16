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


static struct menu_obj * const gui = &gui_objs.u.menu;

static struct menu_page * const mp = &pagemem.u.menu_page;
static u16 current_selected[3] = {0, 0, 0};  // 0 is used for main menu, 1& 2 are used for sub menu

static const char *idx_string_cb(guiObject_t *obj, const void *data);
static const char *menu_name_cb(guiObject_t *obj, const void *data);
static int row_cb(int absrow, int relrow, int y, void *data);


static int menu_get_next_rowidx(unsigned *i)
{
    for (; *i < PAGEID_LAST; (*i)++) {
        unsigned menu = 0xffff;
        #define PAGEDEF(_id, _init, _event, _exit, _menu, _name) \
        case _id: menu = _menu; break;
        switch(*i) {
            #include "pagelist.h"
        }
        #undef PAGEDEF
        if ((menu & 0xf0) != mp->menu_id)
            continue;
        if ((menu & Model.mixer_mode) != Model.mixer_mode)
            continue;
        return menu;
    }
    return -1;
}

void _menu_init(int page)
{
    PAGE_SetModal(0);
    PAGE_RemoveAllObjects();
    PAGE_ShowHeader(_tr(PAGE_GetName(page)));

    unsigned idx = 0;
    unsigned i = 0;
    while(1) {
        menu_get_next_rowidx(&i);
        if (i >= PAGEID_LAST)
            break;
        idx++;
        i++;
    }
    GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, LCD_WIDTH, LCD_HEIGHT - HEADER_HEIGHT,
                     LINE_SPACE, idx, row_cb, NULL, NULL, NULL);
}

void PAGE_MenuInit(int page)
{
    (void)page;
    mp->menu_id = MENUID_MAIN;
    _menu_init(PAGEID_MENU);
    PAGE_SetScrollable(&gui->scrollable, &current_selected[0]);
}
void PAGE_ModelMenuInit(int page)
{
    (void)page;
    mp->menu_id = MENUID_MODEL;
    _menu_init(PAGEID_MODELMNU);
    PAGE_SetScrollable(&gui->scrollable, &current_selected[1]);
}
void PAGE_TxMenuInit(int page)
{
    (void)page;
    mp->menu_id = MENUID_TX;
    _menu_init(PAGEID_TXMENU);
    PAGE_SetScrollable(&gui->scrollable, &current_selected[2]);
}

const char *menu_name_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    long i = (long)data;
    return PAGE_GetName(i);
}

const char *idx_string_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 idx = (long)data;
    sprintf(tempstring, "%d.", idx);
    return tempstring;
}

