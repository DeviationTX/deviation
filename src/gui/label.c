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

#include "_label.c"

guiObject_t *GUI_CreateLabelBox(guiLabel_t *label, u16 x, u16 y, u16 width, u16 height, const struct LabelDesc *desc,
             const char *(*strCallback)(guiObject_t *, const void *),
             void (*pressCallback)(guiObject_t *obj, s8 press_type, const void *data),
             const void *data)
{
    struct guiObject *obj = (guiObject_t *)label;
    struct guiBox    *box;
    CLEAR_OBJ(label);

    box = &obj->box;
    box->x = x;
    box->y = y;
    box->width = width;
    box->height = height;

    obj->Type = Label;
    OBJ_SET_TRANSPARENT(obj, 0);  //Deal with transparency during drawing
    OBJ_SET_SELECTABLE(obj, pressCallback ? 1 :0);
    connect_object(obj);

    label->desc = *desc;
    int underline = label->desc.style == LABEL_UNDERLINE;
    if ((width == 0 || height == 0) && ! underline)
        label->desc.style = LABEL_NO_BOX;
    label->strCallback = strCallback;
    label->pressCallback = pressCallback;
    label->cb_data = data;
    if (! label->desc.font)
        label->desc.font = DEFAULT_FONT.font;
    return obj;
}

void GUI_DrawLabel(struct guiObject *obj)
{
    struct guiLabel *label = (struct guiLabel *)obj;
    const char *str;
    //Set font here so the callback can get its dimensions
    LCD_SetFont(label->desc.font);
    if (label->strCallback)
        str = label->strCallback(obj, label->cb_data);
    else
        str = (const char *)label->cb_data;

    GUI_DrawLabelHelper(obj->box.x, obj->box.y, obj->box.width, obj->box.height, str, &label->desc, obj == objSELECTED);
}

/**
 * this hepler is created to let TextSelect share the label drawing behavior for Devo10
 */
u8 GUI_TouchLabel(struct guiObject *obj, struct touch *coords, s8 press_type)
{
    (void)coords;
    struct guiLabel *label = (struct guiLabel *)obj;
    label->pressCallback(obj, press_type, label->cb_data);
    return 1;
}
void GUI_SetLabelDesc(struct guiLabel *label, struct LabelDesc *desc)
{
    guiObject_t *obj = (guiObject_t *)label;
    if (memcmp(&label->desc, desc, sizeof(struct LabelDesc)) != 0)
        OBJ_SET_DIRTY(obj, 1);
    label->desc = *desc;
}
