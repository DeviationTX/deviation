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

#define gui (&gui_objs.u.mainlayout)
#define pc Model.pagecfg2

#define NUMERIC_LABELS
#define HEADER_Y 10
#include "../common/_main_layout.c"

static u16 current_selected = 0;

static const char *pos_cb(guiObject_t *obj, const void *data);
static u8 _layaction_cb(u32 button, u8 flags, void *data);
static u8 _action_cb(u32 button, u8 flags, void *data);

void show_layout()
{
    GUI_RemoveAllObjects();
    PAGE_SetActionCB(_layaction_cb);
    selected_x = 0;
    selected_y = 0;
    for (int i = 0 ; i < 5; i++)
        gui->desc[i] = (struct LabelDesc){
            .font = MICRO_FONT.font,
            .font_color = 0xffff,
            .fill_color = 0x0000,
            .outline_color = 0xffff,
            .style = LABEL_SQUAREBOX,
        };
    gui->desc[1].style = LABEL_BRACKET; //Special case for trims

    GUI_CreateLabelBox(&gui->editelem, LCD_WIDTH-30, 1, 29, 10, &SMALL_FONT, NULL, NULL, "Move");
    GUI_CreateLabel(&gui->xlbl, 0, 1, NULL, SMALL_FONT, "X:");
    GUI_CreateLabelBox(&gui->x, 10, 1, 20, 10, &SMALL_FONT, pos_cb, NULL, (void *) 0L);
    GUI_CreateLabel(&gui->ylbl, 32, 1, NULL, SMALL_FONT, "Y:");
    GUI_CreateLabelBox(&gui->y, 42, 1, 20, 10, &SMALL_FONT, pos_cb, NULL, (void *) 1L);
    //gui->y must be the last element!
    GUI_SelectionNotify(notify_cb);

    draw_elements();
    if(OBJ_IS_USED(&gui->elem[0]))
        GUI_SetSelected((guiObject_t *)&gui->elem[0]);
}
void layout_exit() 
{
    GUI_SelectionNotify(NULL);
    PAGE_SetActionCB(NULL);
}
void PAGE_MainLayoutInit(int page)
{
    (void)page;
    show_config();
}
void PAGE_MainLayoutEvent()
{
}
void PAGE_MainLayoutExit()
{
}

void set_selected_for_move(guiLabel_t * obj)
{
    selected_for_move = obj;
    GUI_SetHidden((guiObject_t *)&gui->editelem, obj ? 0 : 1);
}

const char *pos_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    sprintf(tmp, "%d", data ? selected_y : selected_x);
    return tmp;
}

void select_for_move(guiLabel_t *obj)
{
    GUI_SetSelected((guiObject_t *)obj);
    notify_cb((guiObject_t *)obj);
    if (selected_for_move == obj)
        return;
    if (selected_for_move) {
        GUI_Redraw((guiObject_t *)selected_for_move);
    }
    set_selected_for_move(obj);
}

void xpos_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    if (selected_for_move) {
        int x = GUI_TextSelectHelper(selected_x, 0, LCD_WIDTH-selected_w, dir, 1, 10, NULL);
        if (x != selected_x) {
            selected_x = x;
            move_elem();
        }
    }
}

void ypos_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    if (selected_for_move) {
        int y = GUI_TextSelectHelper(selected_y, 10, LCD_HEIGHT-selected_h, dir, 1, 10, NULL);
        if (y != selected_y) {
            selected_y = y;
            move_elem();
        }
    }
}

u8 _action_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT))
            PAGE_ChangeByID(PAGEID_MENU, PREVIOUS_ITEM);
        else if (CHAN_ButtonIsPressed(button, BUT_ENTER) &&(flags & BUTTON_LONGPRESS))
            show_layout();
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}

static u8 _layaction_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    if(CHAN_ButtonIsPressed(button, BUT_EXIT) && !(flags & BUTTON_RELEASE)) {
        if (selected_for_move) {
            set_selected_for_move(NULL);
        } else {
            show_config();
        }
        return 1;
    }
    if(! GUI_GetSelected() || ! selected_for_move || flags & BUTTON_RELEASE)
        return 0;
    if(CHAN_ButtonIsPressed(button, BUT_LEFT)) {
        xpos_cb(NULL, (flags & BUTTON_LONGPRESS) ? -2 : -1, (void *)selected_for_move->cb_data);
        return 1;
    }
    if(CHAN_ButtonIsPressed(button, BUT_RIGHT)) {
        xpos_cb(NULL, (flags & BUTTON_LONGPRESS) ? 2 : 1, (void *)selected_for_move->cb_data);
        return 1;
    }
    if(CHAN_ButtonIsPressed(button, BUT_UP)) {
        ypos_cb(NULL, (flags & BUTTON_LONGPRESS) ? -2 : -1, (void *)selected_for_move->cb_data);
        return 1;
    }
    if(CHAN_ButtonIsPressed(button, BUT_DOWN)) {
        ypos_cb(NULL, (flags & BUTTON_LONGPRESS) ? 2 : 1, (void *)selected_for_move->cb_data);
        return 1;
    }
    return 0;
}

/* configure objects */
#undef gui
#define gui (&gui_objs.u.mainconfig)
static int size_cb(int absrow, void *data)
{
    (void)data;
    return 1;
    //return (absrow >= ITEM_MENU) ? 2 : 1;
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
    }
    sprintf(tmp,"%s%d", str, idx+1);
    return tmp;
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
    if (absrow == num_elems) {
        GUI_CreateTextSelectPlate(&gui->value[relrow], 0, y_ts,
                 LCD_WIDTH-x-4, ITEM_HEIGHT, &DEFAULT_FONT, NULL, newelem_cb, NULL);
        GUI_CreateButtonPlateText(&gui->col1[relrow].button, LCD_WIDTH-x-4, y,  50,
                 ITEM_HEIGHT, &DEFAULT_FONT, add_dlgbut_str_cb, 0x0000, newelem_press_cb, (void *)1);
        return 2;
    }
    if (absrow == num_elems + 1) {
        GUI_CreateButtonPlateText(&gui->col1[relrow].button, 0, y,  LCD_WIDTH-4, ITEM_HEIGHT,
                 &DEFAULT_FONT, add_dlgbut_str_cb, 0x0000, add_dlgbut_cb, (void *)0);
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
    PAGE_ShowHeader(_tr("Layout: Long-Press ENT"));
    PAGE_SetActionCB(_action_cb);
    memset(gui, 0, sizeof(struct mainconfig_obj));
    long count = 0;
    for (count = 0; count < NUM_ELEMS; count++) {
        if (! ELEM_USED(pc.elem[count]))
            break;
    }
    GUI_CreateScrollable(&gui->scrollable, 0, ITEM_HEIGHT + 1, LCD_WIDTH, LCD_HEIGHT - ITEM_HEIGHT -1,
                     ITEM_SPACE, count+2, row_cb, getobj_cb, size_cb, (void *)count);
    GUI_SetSelected(GUI_ShowScrollableRowOffset(&gui->scrollable, current_selected));
}
