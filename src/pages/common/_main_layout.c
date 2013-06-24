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

#include "main_layout.h"

extern int GetWidgetLoc(void *ptr, u16 *x, u16 *y, u16 *w, u16 *h);

static const char *label_cb(guiObject_t *obj, const void *data);
static void touch_cb(guiObject_t *obj, s8 press, const void *data);

static void select_for_move(guiLabel_t *obj);
static void notify_cb(guiObject_t *obj);

void draw_elements()
{
    u16 x, y, w, h;
    int i;
    set_selected_for_move(-1);
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
                desc = 2;
#ifndef NUMERIC_LABELS
                strCallback = boxlabel_cb;
#endif
                data = (void *)(long)i;
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
    return GetBoxSource(lp.tmp, pc.elem[i].src);
}

const char *label_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    int idx = (long)data;
    sprintf(lp.tmp, "%d", idx+1);
    return lp.tmp;
}

void touch_cb(guiObject_t *obj, s8 press, const void *data)
{
    //press = -1 : release
    //press = 0  : short press
    //press = 1  : long press
    (void)data;
    if (lp.long_press) {
        if(press == -1)
            lp.long_press = 0;
        return;
    }
    if(press == 0) {
        select_for_move((guiLabel_t *)obj);
    }
    if(press == 1) {
        show_config();
        lp.long_press = 1;
    }
}

int guielem_idx(guiObject_t *obj)
{
    return ((unsigned long)obj - (unsigned long)gui->elem) / sizeof(guiLabel_t);
}

void move_elem()
{
    guiObject_t *obj = GUI_GetSelected();
    if ((guiLabel_t *)obj < gui->elem)
        return;
    int idx = guielem_idx(obj);
    ELEM_SET_X(pc.elem[idx], lp.selected_x);
    ELEM_SET_Y(pc.elem[idx], lp.selected_y);
    draw_elements();
    select_for_move((guiLabel_t *)obj);
}

void notify_cb(guiObject_t *obj)
{
    if ((guiLabel_t *)obj < gui->elem)
        return;
    int idx = guielem_idx(obj);
    lp.selected_x = ELEM_X(pc.elem[idx]);
    lp.selected_y = ELEM_Y(pc.elem[idx]);
    GetElementSize(ELEM_TYPE(pc.elem[idx]), &lp.selected_w, &lp.selected_h);
    GUI_Redraw((guiObject_t *)&gui->x);
    GUI_Redraw((guiObject_t *)&gui->y);
}
