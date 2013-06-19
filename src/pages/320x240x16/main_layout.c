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
#include "icons.h"
#include "gui/gui.h"
#include "config/model.h"
#include "telemetry.h"

#define gui (&gui_objs.u.mainlayout)
#define pc Model.pagecfg2

static void draw_elements();
static const char *label_cb(guiObject_t *obj, const void *data);
static const char *boxlabel_cb(guiObject_t *obj, const void *data);
static const char *newelem_cb(guiObject_t *obj, int dir, void *data);
static void add_dlg_cb(guiObject_t *obj, const void *data);
static void newelem_press_cb(guiObject_t *obj, void *data);
static const char *xpos_cb(guiObject_t *obj, int dir, void *data);
static const char *ypos_cb(guiObject_t *obj, int dir, void *data);
static void touch_cb(guiObject_t *obj, s8 press, const void *data);
static void notify_cb(guiObject_t *obj);
static void move_elem();
static void select_for_move(guiLabel_t *obj);
static void show_config();
static u8 _action_cb(u32 button, u8 flags, void *data);
static const char *dlgbut_str_cb(guiObject_t *obj, const void *data);
static void dlgbut_cb(struct guiObject *obj, const void *data);

extern int GetWidgetLoc(void *ptr, u16 *x, u16 *y, u16 *w, u16 *h);
extern void GetElementSize(unsigned type, u16 *w, u16 *h);
extern int MAINPAGE_FindNextElem(unsigned type, int idx);

guiLabel_t *selected_for_move;
struct buttonAction action;
u8 cfg_elem_type;

u8 newelem;
u16 selected_x, selected_y, selected_w, selected_h;
static u8 long_press;
char tmp[20];
u8 erase;

void PAGE_MainLayoutInit(int page)
{
     (void)page;
    BUTTON_RegisterCallback(&action,
          CHAN_ButtonMask(BUT_ENTER)
          | CHAN_ButtonMask(BUT_EXIT)
          | CHAN_ButtonMask(BUT_LEFT)
          | CHAN_ButtonMask(BUT_LEFT)
          | CHAN_ButtonMask(BUT_RIGHT)
          | CHAN_ButtonMask(BUT_UP)
          | CHAN_ButtonMask(BUT_DOWN),
          BUTTON_PRESS | BUTTON_LONGPRESS | BUTTON_PRIORITY, _action_cb, NULL);
     if (Model.mixer_mode == MIXER_STANDARD)
         PAGE_ShowHeader_ExitOnly(NULL, MODELMENU_Show);
     else
         PAGE_ShowHeader(NULL);
    long_press = 0;
    newelem = 0;
    selected_x = 0;
    const u16 color[5] = {
        RGB888_to_RGB565(0xaa, 0x44, 0x44),
        RGB888_to_RGB565(0x44, 0xaa, 0x44),
        RGB888_to_RGB565(0x44, 0x44, 0xaa),
        RGB888_to_RGB565(0x44, 0x44, 0x44),
        RGB888_to_RGB565(0x33, 0x33, 0x33),
        };
    for (int i = 0 ; i < 5; i++)
        gui->desc[i] = (struct LabelDesc){
            .font = 0,
            .font_color = 0xffff,
            .fill_color = color[i],
            .outline_color = 0,
            .style = LABEL_FILL};
    gui->desc[1].font = TINY_FONT.font; //Special case for trims
    GUI_CreateIcon(&gui->newelem, 36, 0, &icons[ICON_PLUS], add_dlg_cb, NULL);
    //GUI_CreateTextSelect(&gui->newelem, 36, 12, TEXTSELECT_96, newelem_press_cb, newelem_cb, NULL);
    GUI_CreateLabel(&gui->xlbl, 80, 10, NULL, TITLE_FONT, _tr("X"));
    GUI_CreateTextSelect(&gui->x, 88, 10, TEXTSELECT_64, NULL, xpos_cb, NULL);
    GUI_CreateLabel(&gui->ylbl, 164, 10, NULL, TITLE_FONT, _tr("Y"));
    GUI_CreateTextSelect(&gui->y, 172, 10, TEXTSELECT_64, NULL, ypos_cb, NULL);

    GUI_SelectionNotify(notify_cb);
    draw_elements();
}
void PAGE_MainLayoutEvent()
{
}
void PAGE_MainLayoutExit()
{
    GUI_SelectionNotify(NULL);
    BUTTON_UnregisterCallback(&action);
}
void PAGE_MainLayoutRestoreDialog(int idx)
{
    GUI_RemoveAllObjects();
    PAGE_MainLayoutInit(0);
    selected_for_move = &gui->elem[idx];
    show_config();
}

//Return the 
int elem_abs_to_rel(int idx)
{
    unsigned type = ELEM_TYPE(pc.elem[idx]);
    int nxt = -1;
    for (int i = 0; i < NUM_ELEMS-1; i++) {
        nxt = MAINPAGE_FindNextElem(type, nxt+1);
        if (nxt == idx)
            return i;
    }
    return 0;
}
int elem_rel_to_abs(int type, int idx)
{
    int nxt = -1;
    for(int i = 0; i < idx+1; i++)
        nxt = MAINPAGE_FindNextElem(type, nxt+1);
    return nxt;
}

int elem_get_count(int type)
{
    int nxt = -1;
    for (int i = 0; i < NUM_ELEMS; i++) {
        nxt = MAINPAGE_FindNextElem(type, nxt+1);
        if (nxt == -1)
            return i;
    }
    return 0;
}

void draw_elements()
{
    u16 x, y, w, h;
    int i;
    selected_for_move = NULL;
    guiObject_t *obj = gui->y.header.next;
    if (obj)
        GUI_RemoveHierObjects(obj);
    for (i = 0; i < NUM_ELEMS; i++) {
        if (! GetWidgetLoc(&pc.elem[i], &x, &y, &w, &h))
            break;
        int type = ELEM_TYPE(pc.elem[i]);
        const char *(*strCallback)(guiObject_t *, const void *) = label_cb;
        void *data = (void *)(long)elem_abs_to_rel(i);
        int desc = 0;
        switch(type) {
            case ELEM_MODELICO:
                desc = 0; strCallback = NULL; data = (void *)_tr("Model");
                break;
            case ELEM_HTRIM:
            case ELEM_VTRIM:
                desc = 1;
                break;
            case ELEM_SMALLBOX:
            case ELEM_BIGBOX:
                desc = 2; strCallback = boxlabel_cb; data = (void *)(long)i;
                break;
            case ELEM_BAR:
                desc = 3;
                break;
            case ELEM_TOGGLE:
                desc = 4;
        }
        GUI_CreateLabelBox(&gui->elem[i], x, y, w, h, &gui->desc[desc], strCallback, touch_cb, data);
    }
}

const char *boxlabel_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    int i = (long)data;
    if (pc.elem[i].src) {
        if (pc.elem[i].src <= NUM_TIMERS)
            return TIMER_Name(tmp, pc.elem[i].src - 1);
        else if( pc.elem[i].src - NUM_TIMERS <= NUM_TELEM)
        return TELEMETRY_Name(tmp, pc.elem[i].src - NUM_TIMERS);
    }
    return INPUT_SourceName(tmp, pc.elem[i].src
               ? pc.elem[i].src - (NUM_TELEM + NUM_TIMERS) + NUM_INPUTS
               : 0);
}
const char *label_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    int idx = (long)data;
    sprintf(tmp, "%d", idx+1);
    return tmp;
}

const char *newelem_cb(guiObject_t *obj, int dir, void *data)
{   
    (void)data;
    (void)obj;
    newelem = GUI_TextSelectHelper(newelem, 0, ELEM_LAST-1, dir, 1, 1, NULL);
    switch(newelem) {
        case ELEM_SMALLBOX: return _tr("Small-box");
        case ELEM_BIGBOX:   return _tr("Big-box");
        case ELEM_TOGGLE:   return _tr("Toggle");
        case ELEM_BAR:      return _tr("Bargraph");
        case ELEM_VTRIM:    return _tr("V-trim");
        case ELEM_HTRIM:    return _tr("H-trim");
        case ELEM_MODELICO: return _tr("Model");
    }
    return "";
}

void newelem_press_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    (void)data;
    int i;
    u16 x,y,w,h;
    for (i = 0; i < NUM_ELEMS; i++)
        if (! OBJ_IS_USED(&gui->elem[i]))
            break;
    if (i == NUM_ELEMS)
        return;
    y = 1;
    GetElementSize(newelem, &w, &h);
    x = (LCD_WIDTH - w) / 2;
    y = (((LCD_HEIGHT - 40) - h) / 2) + 40;
    memset(&pc.elem[i], 0, sizeof(struct elem));
    ELEM_SET_X(pc.elem[i], x);
    ELEM_SET_Y(pc.elem[i], y);
    ELEM_SET_TYPE(pc.elem[i], newelem);
    draw_elements();
    select_for_move(&gui->elem[i]);
}

const char *xpos_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    int x = GUI_TextSelectHelper(selected_x, 0, LCD_WIDTH-selected_w, dir, 1, 10, NULL);
    if (x != selected_x) {
        selected_x = x;
        move_elem();
    }
    sprintf(tmp, "%d", selected_x);
    return tmp;
}
const char *ypos_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    int y = GUI_TextSelectHelper(selected_y, 40, LCD_HEIGHT-selected_h, dir, 1, 10, NULL);
    if (y != selected_y) {
        selected_y = y;
        move_elem();
    }
    sprintf(tmp, "%d", selected_y);
    return tmp;
    return "0";
}

int guielem_idx(guiObject_t *obj)
{
    return ((unsigned long)obj - (unsigned long)gui->elem) / sizeof(guiLabel_t);
}

void notify_cb(guiObject_t *obj)
{
    if ((guiLabel_t *)obj < gui->elem)
        return;
    int idx = guielem_idx(obj);
    selected_x = ELEM_X(pc.elem[idx]);
    selected_y = ELEM_Y(pc.elem[idx]);
    GetElementSize(ELEM_TYPE(pc.elem[idx]), &selected_w, &selected_h);
    GUI_Redraw((guiObject_t *)&gui->x);
    GUI_Redraw((guiObject_t *)&gui->y);
}

void move_elem()
{
    guiObject_t *obj = GUI_GetSelected();
    if ((guiLabel_t *)obj < gui->elem)
        return;
    int idx = guielem_idx(obj);
    ELEM_SET_X(pc.elem[idx], selected_x);
    ELEM_SET_Y(pc.elem[idx], selected_y);
    draw_elements();
    select_for_move((guiLabel_t *)obj);
}

void select_for_move(guiLabel_t *obj)
{
    GUI_SetSelected((guiObject_t *)obj);
    notify_cb((guiObject_t *)obj);
    if (selected_for_move == obj)
        return;
    if (selected_for_move) {
        selected_for_move->desc.font_color ^= 0xffff;
        selected_for_move->desc.fill_color ^= 0xffff;
        GUI_Redraw((guiObject_t *)selected_for_move);
    }
    selected_for_move = obj;
    selected_for_move->desc.font_color ^= 0xffff;
    selected_for_move->desc.fill_color ^= 0xffff;
}

void touch_cb(guiObject_t *obj, s8 press, const void *data)
{
    //press = -1 : release
    //press = 0  : short press
    //press = 1  : long press
    (void)data;
    if (long_press) {
        if(press == -1)
            long_press = 0;
        return;
    }
    if(press < 0) {
        select_for_move((guiLabel_t *)obj);
    }
    if(selected_for_move && press == 1) {
        show_config();
        long_press = 1;
    }
}

static void dialog_ok_cb(u8 state, void * data)
{
    (void)state;
    (void)data;
    guiObject_t *obj = (guiObject_t *)selected_for_move;
    draw_elements();
    if(obj && OBJ_IS_USED(obj))
        select_for_move((guiLabel_t *)obj);
}

static void add_dlg_ok_cb(u8 state, void * data)
{
    if (state == 1) {
        if(erase) {
            for (int i = 0; i < NUM_ELEMS; i++) {
                ELEM_SET_Y(pc.elem[i], 0);
            }
            dialog_ok_cb(state, data);
        } else {
            newelem_press_cb(NULL, NULL);
        }
    } else {
        dialog_ok_cb(state, data);
    }
}

static const char *add_dlgbut_str_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    return data ? _tr("Erase All") : _tr("Load");
}
static void add_dlgbut_cb(struct guiObject *obj, const void *data)
{
    (void)obj;
    if(data) {
        erase = 1;
        GUI_SetHidden((guiObject_t *)&gui->dlgbut[2], 1);
        GUI_SetHidden((guiObject_t *)&gui->dlglbl[2], 0);
    } else {
        PAGE_MainLayoutExit();
        MODELPage_ShowLoadSave(LOAD_LAYOUT, PAGE_MainLayoutInit);
    }
}

#define ADD_DIALOG_W 200
#define ADD_DIALOG_H 130
static void add_dlg_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    erase = 0;
    GUI_CreateDialog(&gui->dialog,
        (LCD_WIDTH - ADD_DIALOG_W) / 2,
        (LCD_HEIGHT - 40 - ADD_DIALOG_H) / 2 + 40,
        ADD_DIALOG_W,
        ADD_DIALOG_H,
        _tr("Add Element"), NULL, add_dlg_ok_cb, dtOkCancel, "");
    GUI_CreateLabel(&gui->dlglbl[0],
        (LCD_WIDTH - ADD_DIALOG_W) / 2 + 10,
        (LCD_HEIGHT - 40 - ADD_DIALOG_H) / 2 + 60 + 10,
        NULL, DEFAULT_FONT, _tr("Type"));
    GUI_CreateTextSelect(&gui->dlgts[0],
        (LCD_WIDTH - ADD_DIALOG_W) / 2 + ADD_DIALOG_W - 10 - 128,
        (LCD_HEIGHT - 40 - ADD_DIALOG_H) / 2 + 60 + 10,
        TEXTSELECT_128, NULL, newelem_cb, NULL);

    GUI_CreateLabel(&gui->dlglbl[1],
        (LCD_WIDTH - ADD_DIALOG_W) / 2 + 10,
        (LCD_HEIGHT - 40 - ADD_DIALOG_H) / 2 + 60 + 40,
        NULL, DEFAULT_FONT, _tr("Template"));
    GUI_CreateButton(&gui->dlgbut[1],
        (LCD_WIDTH - ADD_DIALOG_W) / 2 + ADD_DIALOG_W - 10 - 112,
        (LCD_HEIGHT - 40 - ADD_DIALOG_H) / 2 + 60 + 40,
        BUTTON_96x16, add_dlgbut_str_cb, 0, add_dlgbut_cb, (void *)0L);

    GUI_CreateButton(&gui->dlgbut[2],
        (LCD_WIDTH - ADD_DIALOG_W) / 2 + ADD_DIALOG_W - 10 - 112,
        (LCD_HEIGHT - 40 - ADD_DIALOG_H) / 2 + 60 + 60,
        BUTTON_96x16, add_dlgbut_str_cb, 0, add_dlgbut_cb, (void *)1L);
    GUI_CreateLabelBox(&gui->dlglbl[2],
        (LCD_WIDTH - ADD_DIALOG_W) / 2 + ADD_DIALOG_W - 10 - 112,
        (LCD_HEIGHT - 40 - ADD_DIALOG_H) / 2 + 60 + 60,
        96, 16, &NARROW_FONT, NULL, NULL, _tr("Erased"));
    GUI_SetHidden((guiObject_t *)&gui->dlglbl[2], 1);
}
#define X_SPACE 10
#define Y_SPACE 10
#define DIALOG_WIDTH (2*X_SPACE + 15 + 100 + 64 +20) //space + # + spinbox + button + scrollbar
#define DIALOG_HEIGHT (LCD_HEIGHT - 32 - 2*Y_SPACE)
#define DIALOG_X (LCD_WIDTH - DIALOG_WIDTH) / 2
#define DIALOG_Y (32 + Y_SPACE)
#define SCROLLABLE_X X_SPACE
#define SCROLLABLE_Y 35
#define TEXT_HEIGHT 20

static guiObject_t *getobj_cb(int relrow, int col, void *data)
{
    (void)data;
    col = (2 + col) % 2;
    if (col == 0)
        return (guiObject_t *)&gui->dlgts[relrow];
    return (guiObject_t *)&gui->dlgbut[relrow];
}

static void dlgbut_cb(struct guiObject *obj, const void *data)
{
    (void)obj;
    int idx = (long)data;
    int i;
    //Remove object
    int type = ELEM_TYPE(pc.elem[idx]);
    for(i = idx+1; i < NUM_ELEMS; i++) {
        if (! ELEM_USED(pc.elem[i]))
            break;
        pc.elem[i-1] = pc.elem[i];
    }
         ELEM_SET_Y(pc.elem[i-1], 0);
    idx = MAINPAGE_FindNextElem(type, 0);
    if (idx >= 0) {
        selected_for_move = &gui->elem[idx];
    } else {
        selected_for_move = NULL;
    }
    //close the dialog and reopen with new elements
    GUI_RemoveHierObjects((guiObject_t *)&gui->dialog);
    show_config();
}

const char *dlgts_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    int idx = (long)data;
    int type = ELEM_TYPE(pc.elem[idx]);
    switch (type) {
        case ELEM_SMALLBOX:
        case ELEM_BIGBOX:
        {
            pc.elem[idx].src = GUI_TextSelectHelper(pc.elem[idx].src, 0, NUM_TELEM + NUM_TIMERS + NUM_CHANNELS, dir, 1, 1, NULL);   
            return boxlabel_cb(NULL, data);
        }
        case ELEM_BAR:
            pc.elem[idx].src = GUI_TextSelectHelper(pc.elem[idx].src, 0, NUM_CHANNELS, dir, 1, 1, NULL);   
            return INPUT_SourceName(tmp, pc.elem[idx].src ? pc.elem[idx].src + NUM_INPUTS : 0);
        case ELEM_TOGGLE:
        {
            int val = MIXER_SRC(pc.elem[idx].src);
            int newval = GUI_TextSelectHelper(val, 0, NUM_SOURCES, dir, 1, 1, NULL);
            newval = INPUT_GetAbbrevSource(val, newval, dir);
            if (val != newval) {
                val = newval;
                pc.elem[idx].src = val;
            }
            return INPUT_SourceNameAbbrevSwitch(tmp, pc.elem[idx].src);
        }
        case ELEM_HTRIM:
        case ELEM_VTRIM:
            pc.elem[idx].src = GUI_TextSelectHelper(pc.elem[idx].src, 0, NUM_TRIMS, dir, 1, 1, NULL);
            sprintf(tmp, "%s%d", _tr("Trim"),pc.elem[idx].src + 1);
            return tmp;
    }
    return "";
}

const char *dlgbut_str_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    return _tr("Delete");
}

static void toggle_press_cb(guiObject_t *obj, void *data)
{
    PAGE_MainLayoutExit();
    TGLICO_Select(obj, data);
}

static int row_cb(int absrow, int relrow, int y, void *data)
{
    int type = (long)data;
    long elemidx = elem_rel_to_abs(type, absrow);
    void (*press_cb)(guiObject_t *obj, void *data) = NULL;
    #define X DIALOG_X + SCROLLABLE_X
    if (type == ELEM_MODELICO) {
        GUI_CreateLabelBox(&gui->dlglbl[relrow], X, y, 115, TEXT_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("Model"));
    } else {
        if (type == ELEM_TOGGLE) {
            press_cb = toggle_press_cb;
        }
        GUI_CreateLabelBox(&gui->dlglbl[relrow], X, y, 10, TEXT_HEIGHT, &DEFAULT_FONT, label_cb, NULL, (void *)(long)(absrow));
        GUI_CreateTextSelect(&gui->dlgts[relrow], X + 15, y, TEXTSELECT_96, press_cb, dlgts_cb, (void *)elemidx);
    }
    GUI_CreateButton(&gui->dlgbut[relrow], X + 15 + 100, y, BUTTON_64x16, dlgbut_str_cb, 0, dlgbut_cb, (void *)elemidx);
    return 1;
}

static void show_config()
{
    int count = 0;
    int row_idx = 0;
    long type;
    if(selected_for_move) {
        int selected_idx = guielem_idx((guiObject_t *)selected_for_move);
        type = ELEM_TYPE(pc.elem[selected_idx]);
        row_idx = elem_abs_to_rel(selected_idx);
        count = elem_get_count(type);
    }
    if (! count) {
        dialog_ok_cb(1, NULL);
        return;
    }
    GUI_CreateDialog(&gui->dialog,
         DIALOG_X, DIALOG_Y,
         DIALOG_WIDTH, DIALOG_HEIGHT,
         _tr("Page Config"), NULL, dialog_ok_cb, dtOk, "");
    GUI_CreateScrollable(&gui->scrollable,
         DIALOG_X + SCROLLABLE_X, DIALOG_Y + SCROLLABLE_Y,
         DIALOG_WIDTH - 2 * SCROLLABLE_X, DIALOG_HEIGHT - 2 * SCROLLABLE_Y,
         TEXT_HEIGHT, count, row_cb, getobj_cb, NULL, (void *)type);
    GUI_SetSelected(GUI_ShowScrollableRowCol(&gui->scrollable, row_idx, 0));
}
    
static u8 _action_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    if(! GUI_GetSelected() || ! selected_for_move || GUI_IsModal())
        return 0;
    if(CHAN_ButtonIsPressed(button, BUT_EXIT)) {
        selected_for_move->desc.font_color ^= 0xffff;
        selected_for_move->desc.fill_color ^= 0xffff;
        GUI_Redraw((guiObject_t *)selected_for_move);
        selected_for_move = NULL;
        return 1;
    }
    if(CHAN_ButtonIsPressed(button, BUT_ENTER)) {
        show_config();
        return 1;
    }
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
