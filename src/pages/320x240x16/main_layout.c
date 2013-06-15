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
static const char *newelem_cb(guiObject_t *obj, int dir, void *data);
static void newelem_press_cb(guiObject_t *obj, void *data);
static const char *xpos_cb(guiObject_t *obj, int dir, void *data);
static const char *ypos_cb(guiObject_t *obj, int dir, void *data);
static void touch_cb(guiObject_t *obj, s8 press, const void *data);
static void notify_cb(guiObject_t *obj);
static void move_elem();

extern int GetWidgetLoc(void *ptr, u16 *x, u16 *y, u16 *w, u16 *h);

enum {
    SMALLBOX_ELEM,
    BIGBOX_ELEM,
    TOGGLE_ELEM,
    BAR_ELEM,
    VTRIM_ELEM,
    HTRIM_ELEM,
    LAST_ELEMTYPE,
};

u8 newelem;
u16 selected_x, selected_y;
char tmp[20];

void PAGE_MainLayoutInit(int page)
{
     (void)page;
     if (Model.mixer_mode == MIXER_STANDARD)
         PAGE_ShowHeader_ExitOnly(NULL, MODELMENU_Show);
     else
         PAGE_ShowHeader(NULL);
    newelem = 0;
    selected_x = 0;
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
    const struct LabelDesc trimrect = {0, 0, RGB888_to_RGB565(0xaa, 0x44, 0x44), 0, LABEL_FILL};
    const struct LabelDesc boxrect  = {0, 0, RGB888_to_RGB565(0x44, 0xaa, 0x44), 0, LABEL_FILL};
    const struct LabelDesc barrect  = {0, 0, RGB888_to_RGB565(0x44, 0x44, 0xaa), 0, LABEL_FILL};
    const struct LabelDesc tglrect  = {0, 0, RGB888_to_RGB565(0x44, 0x44, 0x44), 0, LABEL_FILL};
    const struct LabelDesc icorect = {0, 0, RGB888_to_RGB565(0x33, 0x33, 0x33), 0, LABEL_FILL};
    u16 x, y, w, h;
    int i;
    guiObject_t *obj = gui->y.header.next;
    if (obj)
        GUI_RemoveHierObjects(obj);
    if (GetWidgetLoc(&pc.modelico, &x, &y, &w, &h)) {
        GUI_CreateLabelBox(&gui->modelico, x, y, w, h, &icorect, NULL, touch_cb, "");
    }
    for(i = 0; i < NUM_TRIM_ELEMS; i++) {
        if (! GetWidgetLoc(&pc.trim[i], &x, &y, &w, &h))
            break;
        GUI_CreateLabelBox(&gui->trim[i], x, y, w, h, &trimrect, NULL, touch_cb, "");
    }
    for(i = 0; i < NUM_BOX_ELEMS; i++) {
        if (! GetWidgetLoc(&pc.box[i], &x, &y, &w, &h))
            break;
        GUI_CreateLabelBox(&gui->box[i], x, y, w, h, &boxrect, NULL, touch_cb, "");
    }
    for(i = 0; i < NUM_BAR_ELEMS; i++) {
        if (! GetWidgetLoc(&pc.bar[i], &x, &y, &w, &h))
            break;
        GUI_CreateLabelBox(&gui->bar[i], x, y, w, h, &barrect, NULL, touch_cb, "");
    }
    for(i = 0; i < NUM_TOGGLE_ELEMS; i++) {
        if (! GetWidgetLoc(&pc.tgl[i], &x, &y, &w, &h))
            break;
        GUI_CreateLabelBox(&gui->tgl[i], x, y, w, h, &tglrect, NULL, touch_cb, "");
    }
}

const char *newelem_cb(guiObject_t *obj, int dir, void *data)
{   
    (void)data;
    (void)obj;
    newelem = GUI_TextSelectHelper(newelem, 0, LAST_ELEMTYPE-1, dir, 1, 1, NULL);
    switch(newelem) {
        case SMALLBOX_ELEM: return _tr("small-box");
        case BIGBOX_ELEM:   return _tr("big-box");
        case TOGGLE_ELEM:   return _tr("toggle");
        case BAR_ELEM:      return _tr("bargraph");
        case VTRIM_ELEM:    return _tr("V-trim");
        case HTRIM_ELEM:    return _tr("H-trim");
    }
    return "";
}

void newelem_press_cb(guiObject_t *obj, void *data)
{
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
    int y = GUI_TextSelectHelper(selected_y, 0, LCD_WIDTH, dir, 1, 10, NULL);
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
        GUI_SetSelected(obj);
        return;
    }
    for(i = 0; i < NUM_TRIM_ELEMS; i++) {
        if (obj == (guiObject_t *)&gui->trim[i]) {
            pc.trim[i].x = selected_x;
            pc.trim[i].y = selected_y;
            draw_elements();
            GUI_SetSelected(obj);
            return;
        }
    }
    for(i = 0; i < NUM_BOX_ELEMS; i++) {
        if (obj == (guiObject_t *)&gui->box[i]) {
            pc.box[i].x = selected_x;
            pc.box[i].y = selected_y;
            draw_elements();
            GUI_SetSelected(obj);
            return;
        }
    }
    for(i = 0; i < NUM_BAR_ELEMS; i++) {
        if (obj == (guiObject_t *)&gui->bar[i]) {
            pc.bar[i].x = selected_x;
            pc.bar[i].y = selected_y;
            draw_elements();
            GUI_SetSelected(obj);
            return;
        }
    }
    for(i = 0; i < NUM_TOGGLE_ELEMS; i++) {
        if (obj == (guiObject_t *)&gui->tgl[i]) {
            pc.tgl[i].x = selected_x;
            pc.tgl[i].y = selected_y;
            draw_elements();
            GUI_SetSelected(obj);
            return;
        }
    }
}
void touch_cb(guiObject_t *obj, s8 press, const void *data)
{
    if(press < 0)
        GUI_SetSelected(obj);
}
