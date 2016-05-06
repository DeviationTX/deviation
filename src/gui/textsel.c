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

#include "_textsel.c"

guiObject_t *GUI_CreateTextSelect(guiTextSelect_t *select, u16 x, u16 y, enum TextSelectType type,
        void (*select_cb)(guiObject_t *obj, void *data),
        const char *(*value_cb)(guiObject_t *obj, int value, void *data),
        void *cb_data)
{
    struct guiObject *obj = (guiObject_t *)select;
    struct guiBox *box;
    CLEAR_OBJ(select);

    box = &obj->box;

    select->type = type;
    GUI_TextSelectEnablePress(select, select_cb ? 1 : 0);
    // only used for RTC config in Devo12
#if HAS_RTC
    if (type == TEXTSELECT_VERT_64) {
        box->height = select->button->height + 2 * ARROW_HEIGHT;
        box->width = select->button->width;
    }
    else
#endif
    {
        box->height = select->button->height;
        box->width = select->button->width + 2 * ARROW_WIDTH;
    }

    box->x = x;
    box->y = y;

    obj->Type = TextSelect;
    //Even though the image cannot be overlapped, the file can change under press and select states
    //So we need transparency set
    OBJ_SET_TRANSPARENT(obj, 1);
    OBJ_SET_SELECTABLE(obj, 1);
    connect_object(obj);

    select->state     = 0;
    select->fontColor = 0;
    select->ValueCB   = value_cb;
    select->SelectCB  = select_cb;
    select->InputValueCB = NULL;
    select->cb_data   = cb_data;
    select->enable |= 0x01;

    return obj;
}

guiObject_t *GUI_CreateTextSource(guiTextSelect_t *select, u16 x, u16 y, enum TextSelectType type,
        void (*select_cb)(guiObject_t *obj, void *data),
        const char *(*value_cb)(guiObject_t *obj, int value, void *data),
        const char *(*input_value_cb)(guiObject_t *obj, int src, int value, void *data),
        void *cb_data)
{
    GUI_CreateTextSelect(select, x, y, type, select_cb, value_cb, cb_data);
    select->InputValueCB = input_value_cb;
    return (guiObject_t *)select;
}


guiObject_t *GUI_CreateTextSelectPlate(guiTextSelect_t *select, u16 x, u16 y, u16 width, u16 height, const struct LabelDesc *desc,
        void (*select_cb)(guiObject_t *obj, void *data),
        const char *(*value_cb)(guiObject_t *obj, int value, void *data),
        void *cb_data)
{
    struct guiObject *obj = (guiObject_t *)select;
    struct guiBox *box;

    box = &obj->box;

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
    select->enable |= 0x01;
    select->InputValueCB = NULL;

    GUI_TextSelectEnablePress(select, select_cb ? 1 : 0);

#if LCD_DEPTH == 1
    int underline = select->desc.style == LABEL_UNDERLINE;
#else
   const int underline = 0;
#endif
    if ((width == 0 || height == 0) && ! underline)
        select->desc.style = LABEL_NO_BOX;

    return obj;
}


guiObject_t *GUI_CreateTextSourcePlate(guiTextSelect_t *select, u16 x, u16 y, u16 width, u16 height, const struct LabelDesc *desc,
        void (*select_cb)(guiObject_t *obj, void *data),
        const char *(*value_cb)(guiObject_t *obj, int value, void *data),
        const char *(*input_value_cb)(guiObject_t *obj, int src, int value, void *data),
        void *cb_data)
{
    GUI_CreateTextSelectPlate(select, x, y, width, height, desc, select_cb, value_cb, cb_data);
    select->InputValueCB = input_value_cb;
    return (guiObject_t *)select;
}


void GUI_DrawTextSelect(struct guiObject *obj)
{
    struct guiTextSelect *select = (struct guiTextSelect *)obj;
    //Call the callback 1st in case it calls GUI_TextSelectEnablePress
    const char *str =select->ValueCB(obj, 0, select->cb_data);
    _DrawTextSelectHelper(select, str);
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
    } else if (shortstep > 1) {
        value = (value + (shortstep/2)) / shortstep * shortstep;
    }
    changed = (value == oldval) ? 0 : 1;
    if(_changed)
        *_changed = changed;
    return value;
}

u8 GUI_TouchTextSelect(struct guiObject *obj, struct touch *coords, s8 press_type)
{
    struct guiBox box = obj->box;
    struct guiTextSelect *select = (struct guiTextSelect *)obj;

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
// only used for RTC config in Devo12
#if HAS_RTC
    if (select->type == TEXTSELECT_VERT_64) {
        box.height = ARROW_HEIGHT;
        box.x = obj->box.x + (obj->box.width - ARROW_WIDTH) / 2;
        box.y = obj->box.y + obj->box.height - ARROW_HEIGHT;
    }
#endif
    if (select->enable & 0x01) {
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
// only used for RTC config in Devo12
#if HAS_RTC
        if (select->type == TEXTSELECT_VERT_64) {
            box.y = obj->box.y;
        } else
#endif
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
    }
    if (! press_type && ! select->state && select->SelectCB && (select->enable & 0x02)) {
        OBJ_SET_DIRTY(obj, 1);
        select->state = 0x04;
        return 1;
    }
    return 0;
}

void GUI_PressTextSelect(struct guiObject *obj, u32 button, u8 press_type)
{
    struct touch coords;
    coords.y = obj->box.y + KEY_ADJUST_Y;
    if (button == BUT_RIGHT) {
        coords.x = obj->box.x + obj->box.width + KEY_ADJUST_X - ARROW_WIDTH;
    } else if(button == BUT_LEFT) {
        coords.x = obj->box.x + KEY_ADJUST_X;
    } else if(button == BUT_ENTER) {
        coords.x = obj->box.x + (obj->box.width >> 1);
    } else {
        return;
    }
    GUI_TouchTextSelect(obj, &coords, press_type);
}

void GUI_TextSelectEnablePress(guiTextSelect_t *select, u8 enable)
{
    guiObject_t *obj = (guiObject_t *)select;
    select->enable = enable ? select->enable | 0x02 : select->enable & ~0x02;
    if (select->type == TEXTSELECT_DEVO10) { // plate text for Devo10
        if (enable)
            select->desc.style = LABEL_BOX;
        else
            select->desc.style = LABEL_CENTER;
        return;
    }
    enum ImageNames fileidx;
    switch (select->type) {
        case TEXTSELECT_224: fileidx = FILE_SPIN192; /* enable ? FILE_SPIN192 : FILE_SPIN192;*/ break;
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

void GUI_TextSelectEnable(guiTextSelect_t *select, u8 enable)
{
    guiObject_t *obj = (guiObject_t *)select;
    enable = enable ? 1 : 0;
    if ((0x01 & select->enable) ^ enable) { // bit 0 is different
        select->enable ^= 0x01; //toggle bit 0
        OBJ_SET_DIRTY(obj, 1);
    }
}

u8 GUI_IsTextSelectEnabled(struct guiObject *obj)
{
    struct guiTextSelect *select = (struct guiTextSelect *)obj;
    return select->enable & 0x01;
}
