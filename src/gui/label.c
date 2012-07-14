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

#include "target.h"
#define ENABLE_GUIOBJECT
#include "gui.h"
#include "config/display.h"

guiObject_t *GUI_CreateLabel(u16 x, u16 y, const char *(*Callback)(guiObject_t *, void *), struct FontDesc font, void *data)
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
    box->width = 0;
    box->height = 0;

    obj->Type = Label;
    OBJ_SET_TRANSPARENT(obj, 0);  //Deal with transparency during drawing
    OBJ_SET_USED(obj, 1);
    connect_object(obj);

    label->CallBack = Callback;
    label->cb_data = data;
    label->font = font;
    if (! label->font.font)
        label->font.font = DEFAULT_FONT.font;

    return obj;
}

void GUI_DrawLabel(struct guiObject *obj)
{
    struct guiLabel *label = &obj->o.label;
    const char *str;
    u16 old_w = obj->box.width;
    u16 old_h = obj->box.height;
    if (label->CallBack)
        str = label->CallBack(obj, label->cb_data);
    else
        str = (const char *)label->cb_data;
    u8 old_font = LCD_SetFont(label->font.font);
    LCD_GetStringDimensions((const u8 *)str, &obj->box.width, &obj->box.height);
    if (old_w < obj->box.width)
        old_w = obj->box.width;
    if (old_h < obj->box.height)
        old_h = obj->box.height;
    if (OBJ_IS_SHOWN(obj))
        GUI_DrawBackground(obj->box.x, obj->box.y, old_w, old_h);
    LCD_SetFontColor(label->font.color);
    LCD_PrintStringXY(obj->box.x, obj->box.y, str);
    LCD_SetFont(old_font);
}

