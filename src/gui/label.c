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

guiObject_t *GUI_CreateLabelBox(u16 x, u16 y, u16 width, u16 height, const struct LabelDesc *desc,
             const char *(*strCallback)(guiObject_t *, const void *),
             void (*pressCallback)(guiObject_t *obj, s8 press_type, const void *data),
             const void *data)
{
    struct guiObject *obj = GUI_GetFreeObj();
    struct guiLabel  *label;
    struct guiBox    *box;

    if (obj == NULL)
        return NULL;

    box = &obj->box;
    label = &obj->o.label;
    box->x = x;
    box->y = y;
    box->width = width;
    box->height = height;

    obj->Type = Label;
    OBJ_SET_TRANSPARENT(obj, 0);  //Deal with transparency during drawing
    OBJ_SET_SELECTABLE(obj, pressCallback ? 1 :0);
    connect_object(obj);

    label->desc = *desc;
    if (x == 0 || y == 0)
        label->desc.style = LABEL_NO_BOX;
    label->strCallback = strCallback;
    label->pressCallback = pressCallback;
    label->cb_data = data;
    if (! label->desc.font)
        label->desc.font = DEFAULT_FONT.font;
    label->blink_count = 0;
    return obj;
}

void GUI_DrawLabel(struct guiObject *obj)
{
    struct guiLabel *label = &obj->o.label;
    const char *str;
    u16 txt_w, txt_h;
    u16 txt_x, txt_y;
    if (label->strCallback)
        str = label->strCallback(obj, label->cb_data);
    else
        str = (const char *)label->cb_data;
    LCD_SetFont(label->desc.font);
    LCD_GetStringDimensions((const u8 *)str, &txt_w, &txt_h);
    if (label->desc.style == LABEL_NO_BOX) {
        txt_x = obj->box.x;
        txt_y = obj->box.y;
        u16 old_w = obj->box.width;
        u16 old_h = obj->box.height;
        if (old_w < txt_w)
            old_w = txt_w;
        if (old_h < txt_h)
            old_h = txt_h;
        obj->box.width = txt_w;
        obj->box.height = txt_h;
        GUI_DrawBackground(obj->box.x, obj->box.y, old_w, old_h);
    } else if (label->desc.style == LABEL_UNDERLINE) {
        txt_x = obj->box.x;
        txt_y = obj->box.y;
        u16 old_w = obj->box.width;
        u16 old_h = obj->box.height;
        if (old_w < txt_w)
            old_w = txt_w;
        if (old_h < txt_h)
            old_h = txt_h;
        GUI_DrawBackground(obj->box.x, obj->box.y, old_w, old_h);
        LCD_DrawFastHLine(obj->box.x, old_h, old_w, 1);
    } else {
        if (label->desc.style == LABEL_INVERTED || obj == objSELECTED) {
            u16 w = obj->box.width;
            u16 h = obj->box.height;
            if (w < txt_w)
                w = txt_w;
            if (h < txt_h)
                h = txt_h;
            LCD_FillRect(obj->box.x, obj->box.y, w, h, 1);
        } else if (label->desc.style == LABEL_TRANSPARENT || label->desc.style == LABEL_CENTER || label->desc.style == LABEL_LEFT) {
           GUI_DrawBackground(obj->box.x, obj->box.y, obj->box.width, obj->box.height);
        } else {
            LCD_FillRect(obj->box.x, obj->box.y, obj->box.width, obj->box.height, label->desc.fill_color);
        }
        if (label->desc.style == LABEL_TRANSPARENT || label->desc.fill_color != label->desc.outline_color) {
            LCD_DrawRect(obj->box.x, obj->box.y, obj->box.width, obj->box.height, label->desc.outline_color);
        }
        if (obj->box.height > txt_h)
            txt_y = obj->box.y + (obj->box.height - txt_h) / 2;
        else
            txt_y = obj->box.y;
        if (obj->box.width > txt_w && label->desc.style != LABEL_LEFT)
            txt_x = obj->box.x + (obj->box.width - txt_w) / 2;
        else
            txt_x = obj->box.x;
    }
    label->blink_count++;
    if (label->desc.style == LABEL_BLINK ) {
        if (label->blink_count > 10) {
            label->blink_count = 0;
            LCD_SetFontColor(label->desc.font_color);
        } else {
            LCD_SetFontColor(~label->desc.font_color);
        }
    } else if (label->desc.style == LABEL_INVERTED || obj == objSELECTED) {
        LCD_SetFontColor(~label->desc.font_color);
    } else {
        LCD_SetFontColor(label->desc.font_color);
    }
    LCD_PrintStringXY(txt_x, txt_y, str);
}

u8 GUI_TouchLabel(struct guiObject *obj, struct touch *coords, s8 press_type)
{
    (void)coords;
    struct guiLabel *label = &obj->o.label;
    label->pressCallback(obj, press_type, label->cb_data);
    return 1;
}
void GUI_SetLabelDesc(struct guiObject *obj, struct LabelDesc *desc)
{
    struct guiLabel *label = &obj->o.label;
    if (memcmp(&label->desc, desc, sizeof(struct LabelDesc)) != 0)
        OBJ_SET_DIRTY(obj, 1);
    label->desc = *desc;
}
