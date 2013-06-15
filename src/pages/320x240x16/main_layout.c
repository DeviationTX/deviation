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
static void newelem_press_cb(guiObject_t *obj, void *data);
static const char *xpos_cb(guiObject_t *obj, int dir, void *data);
static const char *ypos_cb(guiObject_t *obj, int dir, void *data);
static void touch_cb(guiObject_t *obj, s8 press, const void *data);
static void notify_cb(guiObject_t *obj);
static void move_elem();
static void select_for_move(guiLabel_t *obj);
static u8 _action_cb(u32 button, u8 flags, void *data);

extern int GetWidgetLoc(void *ptr, u16 *x, u16 *y, u16 *w, u16 *h);
guiLabel_t *selected_for_move;
struct buttonAction action;

enum {
    SMALLBOX_ELEM,
    BIGBOX_ELEM,
    TOGGLE_ELEM,
    BAR_ELEM,
    VTRIM_ELEM,
    HTRIM_ELEM,
    MODELICO_ELEM,
    LAST_ELEMTYPE,
};

#define VTRIM_W      10
#define VTRIM_H     140
#define HTRIM_W     125
#define HTRIM_H      10
#define MODEL_ICO_W  96
#define MODEL_ICO_H  96
#define GRAPH_H      59
#define GRAPH_W      10
#define BOX_W       113
#define SMALLBOX_H   24
#define BIGBOX_H     40

u8 newelem;
u16 selected_x, selected_y;
char tmp[20];

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
     //if (Model.mixer_mode == MIXER_STANDARD)
         PAGE_ShowHeader_ExitOnly(NULL, MODELMENU_Show);
     //else
     //    PAGE_ShowHeader(NULL);
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
    GUI_CreateTextSelect(&gui->newelem, 36, 12, TEXTSELECT_96, newelem_press_cb, newelem_cb, NULL);
    GUI_CreateLabel(&gui->xlbl, 136, 12, NULL, DEFAULT_FONT, _tr("X"));
    GUI_CreateTextSelect(&gui->x, 144, 12, TEXTSELECT_64, NULL, xpos_cb, NULL);
    GUI_CreateLabel(&gui->ylbl, 212, 12, NULL, DEFAULT_FONT, _tr("Y"));
    GUI_CreateTextSelect(&gui->y, 220, 12, TEXTSELECT_64, NULL, ypos_cb, NULL);

    GUI_SelectionNotify(notify_cb);
    draw_elements();
}
void PAGE_MainLayoutEvent()
{
}
void PAGE_MainLayoutExit()
{
    GUI_SelectionNotify(NULL);
}
void draw_elements()
{
    u16 x, y, w, h;
    int i;
    selected_for_move = NULL;
    guiObject_t *obj = gui->y.header.next;
    if (obj)
        GUI_RemoveHierObjects(obj);
    if (GetWidgetLoc(&pc.modelico, &x, &y, &w, &h)) {
        GUI_CreateLabelBox(&gui->modelico, x, y, w, h, &gui->desc[0], NULL, touch_cb, _tr("Model"));
    }
    for(i = 0; i < NUM_TRIM_ELEMS; i++) {
        if (! GetWidgetLoc(&pc.trim[i], &x, &y, &w, &h))
            break;
        GUI_CreateLabelBox(&gui->trim[i], x, y, w, h, &gui->desc[1], label_cb, touch_cb, (void *)(long)i);
    }
    for(i = 0; i < NUM_BOX_ELEMS; i++) {
        if (! GetWidgetLoc(&pc.box[i], &x, &y, &w, &h))
            break;
        GUI_CreateLabelBox(&gui->box[i], x, y, w, h, &gui->desc[2], boxlabel_cb, touch_cb, (void *)(long)i);
    }
    for(i = 0; i < NUM_BAR_ELEMS; i++) {
        if (! GetWidgetLoc(&pc.bar[i], &x, &y, &w, &h))
            break;
        GUI_CreateLabelBox(&gui->bar[i], x, y, w, h, &gui->desc[3], label_cb, touch_cb, (void *)(long)i);
    }
    for(i = 0; i < NUM_TOGGLE_ELEMS; i++) {
        if (! GetWidgetLoc(&pc.tgl[i], &x, &y, &w, &h))
            break;
        GUI_CreateLabelBox(&gui->tgl[i], x, y, w, h, &gui->desc[4], label_cb, touch_cb, (void *)(long)i);
    }
}

const char *boxlabel_cb(guiObject_t *obj, const void *data)
{
    int i = (long)data;
    if (pc.box[i].src <= NUM_TIMERS)
        return TIMER_Name(tmp, pc.box[i].src - 1);
    else if( pc.box[i].src - NUM_TIMERS <= NUM_TELEM)
        return TELEMETRY_Name(tmp, pc.box[i].src - NUM_TIMERS);
    return INPUT_SourceName(tmp, pc.box[i].src ? pc.box[i].src - (NUM_TELEM + NUM_TIMERS) + NUM_INPUTS : 0);
    return tmp;
}
const char *label_cb(guiObject_t *obj, const void *data)
{
    int idx = (long)data;
    sprintf(tmp, "%d", idx);
    return tmp;
}

const char *newelem_cb(guiObject_t *obj, int dir, void *data)
{   
    (void)data;
    (void)obj;
    newelem = GUI_TextSelectHelper(newelem, 0, LAST_ELEMTYPE-1, dir, 1, 1, NULL);
    switch(newelem) {
        case SMALLBOX_ELEM: return _tr("Small-box");
        case BIGBOX_ELEM:   return _tr("Big-box");
        case TOGGLE_ELEM:   return _tr("Toggle");
        case BAR_ELEM:      return _tr("Bargraph");
        case VTRIM_ELEM:    return _tr("V-trim");
        case HTRIM_ELEM:    return _tr("H-trim");
        case MODELICO_ELEM: return _tr("Model");
    }
    return "";
}

void newelem_press_cb(guiObject_t *obj, void *data)
{
    int i;
    u16 x,y,w,h;
    switch(newelem) {
        case MODELICO_ELEM:
            if (! GetWidgetLoc(&pc.modelico, &x, &y, &w, &h)) {
                    pc.modelico = (struct elem_modelico){
                                 (LCD_WIDTH - MODEL_ICO_W) /2,
                                 40 + (LCD_HEIGHT - 40 - MODEL_ICO_H) / 2};
                    draw_elements();
                    select_for_move(&gui->modelico);
                 
            }
            break;
        case SMALLBOX_ELEM:
        case BIGBOX_ELEM:
            for(i = 0; i < NUM_BOX_ELEMS; i++) {
                if (! GetWidgetLoc(&pc.box[i], &x, &y, &w, &h)) {
                    h =  newelem == BIGBOX_ELEM ? BIGBOX_H : SMALLBOX_H;
                    pc.box[i] = (struct elem_box){0,
                                 (LCD_WIDTH - BOX_W) /2,
                                 40 + (LCD_HEIGHT - 40 - h) / 2,
                                 newelem == SMALLBOX_ELEM ? 0 : 1};
                    draw_elements();
                    select_for_move(&gui->box[i]);
                    return;
                }
            }
            break;
        case BAR_ELEM:
            for(i = 0; i < NUM_BAR_ELEMS; i++) {
                if (! GetWidgetLoc(&pc.bar[i], &x, &y, &w, &h)) {
                    h = GRAPH_H;
                    pc.bar[i] = (struct elem_bar){0,
                                 (LCD_WIDTH - GRAPH_W) /2,
                                 40 + (LCD_HEIGHT - 40 - h) / 2};
                    draw_elements();
                    select_for_move(&gui->bar[i]);
                    return;
                }
            }
            break;
        case TOGGLE_ELEM:
            for(i = 0; i < NUM_TOGGLE_ELEMS; i++) {
                if (! GetWidgetLoc(&pc.tgl[i], &x, &y, &w, &h)) {
                    h = TOGGLEICON_HEIGHT;
                    pc.tgl[i] = (struct elem_toggle){0,
                                 (LCD_WIDTH - TOGGLEICON_WIDTH) /2,
                                 40 + (LCD_HEIGHT - 40 - h) / 2, {0, 0, 0}};
                    draw_elements();
                    select_for_move(&gui->tgl[i]);
                    return;
                }
            }
            break;
        case VTRIM_ELEM:
        case HTRIM_ELEM:
            for(i = 0; i < NUM_TRIM_ELEMS; i++) {
                if (! GetWidgetLoc(&pc.trim[i], &x, &y, &w, &h)) {
                    h = newelem == VTRIM_ELEM ? VTRIM_H : HTRIM_H;
                    w = newelem == VTRIM_ELEM ? VTRIM_W : HTRIM_W;
        
                    pc.trim[i] = (struct elem_trim){0,
                                 (LCD_WIDTH - w) /2,
                                 40 + (LCD_HEIGHT - 40 - h) / 2, newelem == VTRIM_ELEM};
                    draw_elements();
                    select_for_move(&gui->trim[i]);
                    return;
                }
            }
            break;
    }
            
}
const char *xpos_cb(guiObject_t *obj, int dir, void *data)
{
    int x = GUI_TextSelectHelper(selected_x, 0, LCD_WIDTH, dir, 1, 10, NULL);
    if (x != selected_x) {
        selected_x = x;
        move_elem();
    }
    sprintf(tmp, "%d", selected_x);
    return tmp;
}
const char *ypos_cb(guiObject_t *obj, int dir, void *data)
{
    int y = GUI_TextSelectHelper(selected_y, 40, LCD_WIDTH, dir, 1, 10, NULL);
    if (y != selected_y) {
        selected_y = y;
        move_elem();
    }
    sprintf(tmp, "%d", selected_y);
    return tmp;
    return "0";
}

void notify_cb(guiObject_t *obj)
{
    int i;
    if (obj == (guiObject_t *)&gui->modelico) {
        selected_x = pc.modelico.x;
        selected_y = pc.modelico.y;
        GUI_Redraw((guiObject_t *)&gui->x);
        GUI_Redraw((guiObject_t *)&gui->y);
        return;
    }
    for(i = 0; i < NUM_TRIM_ELEMS; i++) {
        if (obj == (guiObject_t *)&gui->trim[i]) {
            selected_x = pc.trim[i].x;
            selected_y = pc.trim[i].y;
            GUI_Redraw((guiObject_t *)&gui->x);
            GUI_Redraw((guiObject_t *)&gui->y);
            return;
        }
    }
    for(i = 0; i < NUM_BOX_ELEMS; i++) {
        if (obj == (guiObject_t *)&gui->box[i]) {
            selected_x = pc.box[i].x;
            selected_y = pc.box[i].y;
            GUI_Redraw((guiObject_t *)&gui->x);
            GUI_Redraw((guiObject_t *)&gui->y);
            return;
        }
    }
    for(i = 0; i < NUM_BAR_ELEMS; i++) {
        if (obj == (guiObject_t *)&gui->bar[i]) {
            selected_x = pc.bar[i].x;
            selected_y = pc.bar[i].y;
            GUI_Redraw((guiObject_t *)&gui->x);
            GUI_Redraw((guiObject_t *)&gui->y);
            return;
        }
    }
    for(i = 0; i < NUM_TOGGLE_ELEMS; i++) {
        if (obj == (guiObject_t *)&gui->tgl[i]) {
            selected_x = pc.tgl[i].x;
            selected_y = pc.tgl[i].y;
            GUI_Redraw((guiObject_t *)&gui->x);
            GUI_Redraw((guiObject_t *)&gui->y);
            return;
        }
    }
}

void move_elem()
{
    int i;
    guiObject_t *obj = GUI_GetSelected();
    if (obj == (guiObject_t *)&gui->modelico) {
        pc.modelico.x = selected_x;
        pc.modelico.y = selected_y;
        draw_elements();
        select_for_move((guiLabel_t *)obj);
        return;
    }
    for(i = 0; i < NUM_TRIM_ELEMS; i++) {
        if (obj == (guiObject_t *)&gui->trim[i]) {
            pc.trim[i].x = selected_x;
            pc.trim[i].y = selected_y;
            draw_elements();
            select_for_move((guiLabel_t *)obj);
            return;
        }
    }
    for(i = 0; i < NUM_BOX_ELEMS; i++) {
        if (obj == (guiObject_t *)&gui->box[i]) {
            pc.box[i].x = selected_x;
            pc.box[i].y = selected_y;
            draw_elements();
            select_for_move((guiLabel_t *)obj);
            return;
        }
    }
    for(i = 0; i < NUM_BAR_ELEMS; i++) {
        if (obj == (guiObject_t *)&gui->bar[i]) {
            pc.bar[i].x = selected_x;
            pc.bar[i].y = selected_y;
            draw_elements();
            select_for_move((guiLabel_t *)obj);
            return;
        }
    }
    for(i = 0; i < NUM_TOGGLE_ELEMS; i++) {
        if (obj == (guiObject_t *)&gui->tgl[i]) {
            pc.tgl[i].x = selected_x;
            pc.tgl[i].y = selected_y;
            draw_elements();
            select_for_move((guiLabel_t *)obj);
            return;
        }
    }
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
    if(press < 0) {
        select_for_move((guiLabel_t *)obj);
    }
}

static u8 _action_cb(u32 button, u8 flags, void *data)
{
    if(! GUI_GetSelected() || ! selected_for_move)
        return 0;
    if(CHAN_ButtonIsPressed(button, BUT_EXIT)) {
        selected_for_move->desc.font_color ^= 0xffff;
        selected_for_move->desc.fill_color ^= 0xffff;
        GUI_Redraw((guiObject_t *)selected_for_move);
        selected_for_move = NULL;
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
