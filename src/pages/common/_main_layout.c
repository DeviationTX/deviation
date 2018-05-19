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
extern void AdjustIconSize(u16 *x, u16 *y, u16 *h, u16 *w);

static const char *label_cb(guiObject_t *obj, const void *data);
static const char *battlabel_cb(guiObject_t *obj, const void *data);
static const char *powerlabel_cb(guiObject_t *obj, const void *data);
static void touch_cb(guiObject_t *obj, s8 press, const void *data);

static void select_for_move(guiLabel_t *obj);
static void notify_cb(guiObject_t *obj);

void draw_elements()
{
    u16 x, y, w, h;
    int i;
    set_selected_for_move(-1);
    guiObject_t *obj = gui->y.header.next;
    if (obj) {
        u8 redraw_mode = FullRedraw;
        GUI_RemoveHierObjects(obj);
        FullRedraw = redraw_mode;
    }
    for (i = 0; i < NUM_ELEMS; i++) {
        if (! GetWidgetLoc(&pc->elem[i], &x, &y, &w, &h))
            break;
        int type = ELEM_TYPE(pc->elem[i]);
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
                data = (void *)(long)i;
#endif
                break;
            case ELEM_BAR:
                desc = 3;
                break;
            case ELEM_TOGGLE:
                desc = 4;
                break;
            case ELEM_BATTERY:
                desc = 2;
                strCallback = battlabel_cb;
                break;
            case ELEM_TXPOWER:
                desc = 2;
                strCallback = powerlabel_cb;
                break;
        }
        GUI_CreateLabelBox(&gui->elem[i], x, y, w, h, &gui->desc[desc], strCallback, touch_cb, data);
    }
}

const char *boxlabel_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    int i = (long)data;
    return GetBoxSource(tempstring, pc->elem[i].src);
}

const char *label_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    int idx = (long)data;
    sprintf(tempstring, "%d", idx+1);
    return tempstring;
}

static const char *battlabel_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    int idx = (long)data;
    sprintf(tempstring, "B: %d", idx+1);
    return tempstring;
}

static const char *powerlabel_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    int idx = (long)data;
    sprintf(tempstring, "P: %d", idx+1);
    return tempstring;
}

void touch_cb(guiObject_t *obj, s8 press, const void *data)
{
    //press = -1 : release
    //press = 0  : short press
    //press = 1  : long press
    (void)data;
    if (lp->long_press) {
        if(press == -1)
            lp->long_press = 0;
        return;
    }
    if(press == 0) {
        select_for_move((guiLabel_t *)obj);
    }
    if(press == 1) {
        show_config();
        lp->long_press = 1;
    }
}

int guielem_idx(guiObject_t *obj)
{
    return ((unsigned long)obj - (unsigned long)gui->elem) / sizeof(guiLabel_t);
}

void move_elem()
{
    u16 x, y, w, h;
    guiObject_t *obj = GUI_GetSelected();
    if ((guiLabel_t *)obj < gui->elem)
        return;
    int idx = guielem_idx(obj);
    GetWidgetLoc(&pc->elem[idx], &x, &y, &w, &h);
    if (x > 0) {
        x -= 1;
        w += 2;
    }
    if (y > 0) {
        y -= 1;
        h += 2;
    }
    if(x + w > LCD_WIDTH) {
        w = LCD_WIDTH - x;
    }
    if(y + h > LCD_HEIGHT) {
        h = LCD_HEIGHT - y;
    }
    GUI_DrawBackground(x, y, w, h);
    ELEM_SET_X(pc->elem[idx], lp->selected_x);
    ELEM_SET_Y(pc->elem[idx], lp->selected_y);
    draw_elements();
    select_for_move((guiLabel_t *)obj);
}

void notify_cb(guiObject_t *obj)
{
    if ((guiLabel_t *)obj < gui->elem)
        return;
    int idx = guielem_idx(obj);
    lp->selected_x = ELEM_X(pc->elem[idx]);
    lp->selected_y = ELEM_Y(pc->elem[idx]);
    GetElementSize(ELEM_TYPE(pc->elem[idx]), &lp->selected_w, &lp->selected_h);
    if (ELEM_TYPE(pc->elem[idx]) == ELEM_MODELICO)
        AdjustIconSize(&lp->selected_x, &lp->selected_y, &lp->selected_h, &lp->selected_w);
    GUI_Redraw((guiObject_t *)&gui->xlbl);
    GUI_Redraw((guiObject_t *)&gui->x);
    GUI_Redraw((guiObject_t *)&gui->ylbl);
    GUI_Redraw((guiObject_t *)&gui->y);
}
