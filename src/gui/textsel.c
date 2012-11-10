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
#define ENABLE_GUIOBJECT
#include "gui.h"
#include "config/display.h"

guiObject_t *GUI_CreateTextSelect(u16 x, u16 y, enum TextSelectType type, u16 fontColor,
        void (*select_cb)(guiObject_t *obj, void *data),
        const char *(*value_cb)(guiObject_t *obj, int value, void *data),
        void *cb_data)
{
    struct guiObject *obj = GUI_GetFreeObj();
    struct guiTextSelect *select;
    struct guiBox *box;

    if (obj == NULL)
        return NULL;

    box = &obj->box;
    select = &obj->o.textselect;

    select->type = type;
    GUI_TextSelectEnablePress(obj, select_cb ? 1 : 0);
    box->height = select->button->height;
    box->width = select->button->width + 2 * ARROW_WIDTH;

    box->x = x;
    box->y = y;

    obj->Type = TextSelect;
    //Even though the image cannot be overlapped, the file can change under press and select states
    //So we need transparency set
    OBJ_SET_TRANSPARENT(obj, 1);
    OBJ_SET_SELECTABLE(obj, 1);
    connect_object(obj);

    select->state     = 0;
    select->fontColor = fontColor;
    select->ValueCB   = value_cb;
    select->SelectCB  = select_cb;
    select->cb_data   = cb_data;
    select->enable = 1;

    return obj;
}

guiObject_t *GUI_CreateTextSelectPlate(u16 x, u16 y, u16 width, u16 height, const struct LabelDesc *desc,
        void (*select_cb)(guiObject_t *obj, void *data),
        const char *(*value_cb)(guiObject_t *obj, int value, void *data),
        void *cb_data)
{
    struct guiObject *obj = GUI_GetFreeObj();
    struct guiTextSelect *select;
    struct guiBox *box;

    if (obj == NULL)
        return NULL;

    box = &obj->box;
    select = &obj->o.textselect;

    select->type = TEXTSELECT_DEVO10;
    box->height = height;
    box->width = width;

    box->x = x;
    box->y = y;

    obj->Type = TextSelect;
    OBJ_SET_SELECTABLE(obj, 1);
    connect_object(obj);

    select->button    = NULL; 
    select->state     = 0;
    select->fontColor = 0xffff;
    select->desc       = *desc;
    select->ValueCB   = value_cb;
    select->SelectCB  = select_cb;
    select->cb_data   = cb_data;
    select->enable = 1;

    GUI_TextSelectEnablePress(obj, select_cb ? 1 : 0);

    if ((width == 0 || height == 0) && select->desc.style != LABEL_UNDERLINE)
        select->desc.style = LABEL_NO_BOX;

    return obj;
}

void GUI_DrawTextSelect(struct guiObject *obj)
{
    u16 x, y, w, h;
    struct guiBox *box = &obj->box;
    struct guiTextSelect *select = &obj->o.textselect;
    //Call the callback 1st in case it calls GUI_TextSelectEnablePress
    const char *str =select->ValueCB(obj, 0, select->cb_data);

    if (select->type != TEXTSELECT_DEVO10) {
        GUI_DrawImageHelper(box->x + ARROW_WIDTH,
                            box->y, select->button, DRAW_NORMAL);
        if (select->enable) {
            GUI_DrawImageHelper(box->x, box->y, ARROW_LEFT,
                                select->state & 0x01 ? DRAW_PRESSED : DRAW_NORMAL);
            GUI_DrawImageHelper(box->x + box->width - ARROW_WIDTH,
                                box->y, ARROW_RIGHT,
                                select->state & 0x02 ? DRAW_PRESSED : DRAW_NORMAL);
        }
        LCD_SetFontColor(select->fontColor);
        LCD_GetStringDimensions((const u8 *)str, &w, &h);
        x = box->x + (box->width - w) / 2;
        y = box->y + 2 + (box->height - h) / 2;
        LCD_PrintStringXY(x, y, str);
    } else {   // plate text select for devo 10, copy most behavior from label.c
        u8 arrow_width = ARROW_WIDTH - 1;
        if (select->enable) {
            u16 y = box->y + obj->box.height / 2;  // Bug fix: since the logic view is introduce, a coordinate could be greater than 10000
            u16 x1 = box->x + arrow_width -1;
            LCD_DrawLine(box->x, y, x1, y - 2, 0xffff);
            LCD_DrawLine(box->x, y, x1, y + 2, 0xffff); //"<"
            //LCD_DrawFastHLine(box->x, y, ARROW_WIDTH, 0xffff); //"-"
            x1 = box->x + box->width - arrow_width;
            u16 x2 = box->x + box->width -1;
            //LCD_DrawFastHLine(x1, y, ARROW_WIDTH, 0xffff); //"+"
            //LCD_DrawFastVLine(x1 +1, y-1, ARROW_WIDTH, 0xffff); //"+"
            LCD_DrawLine(x1, y - 2, x2, y, 0xffff);
            LCD_DrawLine(x1, y + 2, x2, y, 0xffff); //">"
        } else {
            GUI_DrawBackground(box->x, box->y, box->width, box->height);
        }
        GUI_DrawLabelHelper(box->x + arrow_width +1, box->y,box->width - arrow_width -arrow_width -2, obj->box.height,
                str, &select->desc, obj == objSELECTED);
    }
}

s32 GUI_TextSelectHelper(s32 value, s32 min, s32 max, s8 dir, u32 shortstep, u32 longstep, u8 *_changed)
{
    u8 changed;
    s32 oldval = value;
    if (dir > 0) {
        if (value < max) {
           value += (dir > 1) ? longstep : shortstep;
        }
    } else if (dir < 0) {
        if (value > min) {
           value -= (dir < -1) ? longstep : shortstep;
        }
    }
    if (value > max) {
        value = max;
    } else if (value < min) {
        value = min;
    }
    changed = (value == oldval) ? 0 : 1;
    if(_changed)
        *_changed = changed;
    return value;
}

u8 GUI_TouchTextSelect(struct guiObject *obj, struct touch *coords, s8 press_type)
{
    struct guiBox box = obj->box;
    struct guiTextSelect *select = &obj->o.textselect;

    if (press_type < 0) {
        if(! select->state) {
            return 0;
        } else if(select->state & 0x80) {
            select->state = 0;
            OBJ_SET_DIRTY(obj, 1);
            return 1;
        } else if(select->state == 0x01) {
            select->state = 0;
            OBJ_SET_DIRTY(obj, 1);
            select->ValueCB(obj, -1, select->cb_data);
            return 1;
        } else if(select->state == 0x02) {
            select->state = 0;
            OBJ_SET_DIRTY(obj, 1);
            select->ValueCB(obj, 1, select->cb_data);
            return 1;
        } else if(select->state == 0x04) {
            select->state = 0;
            OBJ_SET_DIRTY(obj, 1);
            select->SelectCB(obj, select->cb_data);
            return 1;
        }
        printf("Error: Should not get here\n");
        return 0;
    }
    box.width = ARROW_WIDTH;
    if (coords_in_box(&box, coords)) {
        if (! press_type) {
            if (! select->state) {
                select->state = 0x01;
                OBJ_SET_DIRTY(obj, 1);
            }
        } else if (select->ValueCB) {
            OBJ_SET_DIRTY(obj, 1);
            select->ValueCB(obj, -2, select->cb_data);
            select->state |= 0x80;
        }
        return 1;
    }
    box.x = obj->box.x + obj->box.width - ARROW_WIDTH;
    if (coords_in_box(&box, coords)) {
        if (! press_type) {
            if (! select->state) {
                select->state = 0x02;
                OBJ_SET_DIRTY(obj, 1);
            }
        } else if (select->ValueCB) {
            OBJ_SET_DIRTY(obj, 1);
            select->ValueCB(obj, 2, select->cb_data);
            select->state |= 0x80;
        }
        return 1;
    }
    if (! press_type && ! select->state && select->SelectCB) {
        OBJ_SET_DIRTY(obj, 1);
        select->state = 0x04;
        return 1;
    }
    return 0;
}

void GUI_PressTextSelect(struct guiObject *obj, u32 button, u8 press_type)
{
    struct touch coords;
    coords.y = obj->box.y + 1;
    if (button == BUT_RIGHT) {
        coords.x = obj->box.x + obj->box.width + 1 - ARROW_WIDTH;
    } else if(button == BUT_LEFT) {
        coords.x = obj->box.x + 1;
    } else if(button == BUT_ENTER) {
        coords.x = obj->box.x + (obj->box.width >> 1);
    } else {
        return;
    }
    GUI_TouchTextSelect(obj, &coords, press_type);
}

void GUI_TextSelectEnablePress(struct guiObject *obj, u8 enable)
{
    struct guiTextSelect *select = &obj->o.textselect;
    if (select->type == TEXTSELECT_DEVO10) { // plate text for Devo10
        if (select->enable == 0)
            select->desc.style = LABEL_CENTER;
        else if (enable)
            select->desc.style = LABEL_BOX;
        else
            select->desc.style = LABEL_CENTER;
        return;
    }
    enum ImageNames fileidx;
    switch (select->type) {
        case TEXTSELECT_128: fileidx = enable ? FILE_SPINPRESS96 : FILE_SPIN96; break;
        case TEXTSELECT_96:  fileidx = enable ? FILE_SPINPRESS64 : FILE_SPIN64; break;
        case TEXTSELECT_64:  fileidx = enable ? FILE_SPINPRESS32 : FILE_SPIN32; break;
        default: fileidx = FILE_SPIN32; break;
    }
    if (select->button != &image_map[fileidx]) {
        select->button = &image_map[fileidx];
        OBJ_SET_DIRTY(obj, 1);
    }
}

void GUI_TextSelectEnable(struct guiObject *obj, u8 enable)
{
    struct guiTextSelect *select = &obj->o.textselect;
    if (select->enable != enable) {
        select->enable = enable;
        OBJ_SET_DIRTY(obj, 1);
    }
}

u8 GUI_IsTextSelectEnabled(struct guiObject *obj)
{
    struct guiTextSelect *select = &obj->o.textselect;
    return select->enable;
}
