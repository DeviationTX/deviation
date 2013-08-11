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
#include "telemetry.h"

#define lp pagemem.u.layout_page
#define pc Model.pagecfg2
#define gui (&gui_objs.u.mainconfig)
#define HEADER_Y 10

#include "../common/_main_config.c"

static u16 current_selected = 0;

static u8 _action_cb(u32 button, u8 flags, void *data);

void PAGE_MainLayoutInit(int page)
{
    (void)page;
    memset(&lp, 0, sizeof(lp));
    show_config();
}
void PAGE_MainLayoutEvent()
{
}
void PAGE_MainLayoutExit()
{
}

static int size_cb(int absrow, void *data)
{
    int num_elems = (long)data;
    return (absrow >= num_elems  && absrow < num_elems + NUM_QUICKPAGES) ? 2 : 1;
}

static guiObject_t *getobj_cb(int relrow, int col, void *data)
{
    (void)data;
    if (OBJ_IS_USED(&gui->col1[relrow].label) && gui->col1[relrow].label.header.Type == Button) {
        if(! OBJ_IS_USED(&gui->value[relrow])) {
            return (guiObject_t *)&gui->col1[relrow].label;
        }
        col = (2 + col) % 2;
        //Both button and text-select
        if (gui->value[relrow].header.box.x == 0) {
            //ts is 1st
            return (col ? (guiObject_t *)&gui->col1[relrow].label : (guiObject_t *)&gui->value[relrow]);
        } else {
            //button is 1st
            return (!col ? (guiObject_t *)&gui->col1[relrow].label : (guiObject_t *)&gui->value[relrow]);
        }
    }
    return (guiObject_t *)&gui->value[relrow];
}

static const char *cfglabel_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    int i = (long)data;
    int type = ELEM_TYPE(pc.elem[i]);
    int idx = elem_abs_to_rel(i);
    const char *str;
    switch(type) {
    case ELEM_VTRIM:
    case ELEM_HTRIM:
        str = _tr("Trimbar");
        break; 
    case ELEM_SMALLBOX:
        str = _tr("Box");
        break;
    case ELEM_TOGGLE:
        str = _tr("Toggle");
        break;
    case ELEM_MODELICO:
        str = _tr("Model");
        break;
    default:
        str = "";
    }
    sprintf(lp.tmp,"%s%d", str, idx+1);
    return lp.tmp;
}

static void switchicon_press_cb(guiObject_t *obj, const void *data)
{
    current_selected = GUI_ScrollableGetObjRowOffset(&gui->scrollable, GUI_GetSelected());
    TGLICO_Select(obj, data);
}

void newelem_press_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    create_element();
    show_config();
}

static const char *dlgts1_cb(guiObject_t *obj, int dir, void *data)
{
    int idx = (long)data;
    if (pc.elem[idx].src == 0 && dir < 0)
        pc.elem[idx].src = -1;
    if ((s8)pc.elem[idx].src == -1 && dir > 0) {
        pc.elem[idx].src = 0;
        dir = 0;
    }
    if ((s8)pc.elem[idx].src < 0) {
        GUI_TextSelectEnablePress((guiTextSelect_t *)obj, 1);
        return _tr("Delete");
    }
    GUI_TextSelectEnablePress((guiTextSelect_t *)obj, 0);
    return dlgts_cb(obj, dir, data);
}

static int row_cb(int absrow, int relrow, int y, void *data)
{
    int num_elems = (long)data;
    int x = 56;
    int y_ts = y;
    //show elements in order
    int row = -1;
    if (absrow == num_elems + NUM_QUICKPAGES) {
        GUI_CreateTextSelectPlate(&gui->value[relrow], 0, y_ts,
                 LCD_WIDTH-x-4, ITEM_HEIGHT, &DEFAULT_FONT, NULL, newelem_cb, NULL);
        GUI_CreateButtonPlateText(&gui->col1[relrow].button, LCD_WIDTH-x-4, y,  50,
                 ITEM_HEIGHT, &DEFAULT_FONT, add_dlgbut_str_cb, 0x0000, newelem_press_cb, (void *)1);
        return 2;
    }
#if ENABLE_LAYOUT_EDIT
    if (absrow == num_elems + NUM_QUICKPAGES + 1) {
        GUI_CreateButtonPlateText(&gui->col1[relrow].button, 0, y,  LCD_WIDTH-4, ITEM_HEIGHT,
                 &DEFAULT_FONT, add_dlgbut_str_cb, 0x0000, add_dlgbut_cb, (void *)0);
        return 1;
    }
#endif
    if (absrow >= num_elems && absrow < num_elems + NUM_QUICKPAGES) {
        GUI_CreateLabelBox(&gui->col1[relrow].label, 0, y,  x, ITEM_HEIGHT, &DEFAULT_FONT, menulabel_cb, NULL, (void *)(long)(absrow - num_elems));
        GUI_CreateTextSelectPlate(&gui->value[relrow], 0, y + ITEM_HEIGHT,
             LCD_WIDTH-4, ITEM_HEIGHT, &DEFAULT_FONT, NULL, menusel_cb, (void *)(long)(absrow - num_elems));
        return 1;
    }
    for(int type = 0; type < ELEM_LAST; type++) {
        if (type == ELEM_BIGBOX || type == ELEM_HTRIM)
            continue;
        long nxt = -1;
        long item = -1;
        while((nxt = MAINPAGE_FindNextElem(type, nxt+1)) >= 0) {
            if(ELEM_TYPE(pc.elem[nxt]) == ELEM_BIGBOX)  //because FindNextElem maps elements
                continue;
            item = nxt;
            row++;
            if(row == absrow)
                break;
        }
        if (nxt == -1)
            continue;
        if (type == ELEM_TOGGLE)
            GUI_CreateButtonPlateText(&gui->col1[relrow].button, 0, y,  50,
                    ITEM_HEIGHT, &DEFAULT_FONT, cfglabel_cb, 0x0000, switchicon_press_cb, (void *)item);
        else
            GUI_CreateLabelBox(&gui->col1[relrow].label, 0, y,  x, ITEM_HEIGHT, &DEFAULT_FONT, cfglabel_cb, NULL, (void *)item);

        GUI_CreateTextSelectPlate(&gui->value[relrow], x, y_ts,
             LCD_WIDTH-x-4, ITEM_HEIGHT, &DEFAULT_FONT, (void(*)(guiObject_t *, void *))dlgbut_cb, dlgts1_cb, (void *)item);
        return 1;
    }
    return 1;
}
void show_config()
{
    PAGE_MainLayoutExit();
    GUI_RemoveAllObjects();
#if ENABLE_LAYOUT_EDIT
    PAGE_ShowHeader(_tr("Layout: Long-Press ENT"));
#endif
    PAGE_SetActionCB(_action_cb);
    memset(gui, 0, sizeof(struct mainconfig_obj));
    long count = 0;
    for (count = 0; count < NUM_ELEMS; count++) {
        if (! ELEM_USED(pc.elem[count]))
            break;
    }
#if ENABLE_LAYOUT_EDIT
    #define ADD_LOAD 2
# else
    #define ADD_LOAD 1
#endif
    GUI_CreateScrollable(&gui->scrollable, 0, ITEM_HEIGHT + 1, LCD_WIDTH, LCD_HEIGHT - ITEM_HEIGHT -1,
                     ITEM_SPACE, count+NUM_QUICKPAGES + ADD_LOAD, row_cb, getobj_cb, size_cb, (void *)count);
    GUI_SetSelected(GUI_ShowScrollableRowOffset(&gui->scrollable, current_selected));
}

static u8 _action_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT))
            PAGE_ChangeByID(PAGEID_MENU, PREVIOUS_ITEM);
#if ENABLE_LAYOUT_EDIT
        else if (CHAN_ButtonIsPressed(button, BUT_ENTER) &&(flags & BUTTON_LONGPRESS))
            show_layout();
#endif //ENABLE_LAYOUT_EDIT
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}
