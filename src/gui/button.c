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

#include "_button.c"

guiObject_t *GUI_CreateButton(guiButton_t *button, u16 x, u16 y, enum ButtonType type,
    const char *(*strCallback)(struct guiObject *, const void *),
    void (*CallBack)(struct guiObject *obj, const void *data), const void *cb_data)
{
    struct guiHeader *obj = (guiObject_t *)button;
    struct guiBox    *box;
    CLEAR_OBJ(button);

    box = &obj->box;

    button->image = _button_image_map(type);

    box->x = x;
    box->y = y;
    box->width = button->image->width;
    box->height = button->image->height;

    obj->Type = Button;
    //Even though the image cannot be overlapped, the file can change under press and select states
    //So we need transparency set
    OBJ_SET_TRANSPARENT(obj, 1);
    OBJ_SET_SELECTABLE(obj, 1);
    connect_object(obj);

    // bug fix: must set a default font, otherwise language other than English might not be displayed in some dialogs
    button->desc.font = DEFAULT_FONT.font;
    button->strCallback = strCallback;
    button->CallBack = CallBack;
    button->cb_data = cb_data;
    button->enable = 1;

    return obj;
}

guiObject_t *GUI_CreateIcon(guiButton_t *button, u16 x, u16 y, const struct ImageMap *image,
        void (*CallBack)(struct guiObject *obj, const void *data), const void *cb_data)
{
    struct guiHeader *obj = (guiObject_t *)button;
    struct guiBox    *box;
    CLEAR_OBJ(button);

    box = &obj->box;
    button->image = image;

    box->x = x;
    box->y = y;
    box->width = button->image->width;
    box->height = button->image->height;

    obj->Type = Button;
    //Even though the image cannot be overlapped, the file can change under press and select states
    //So we need transparency set
    OBJ_SET_TRANSPARENT(obj, 1);
    OBJ_SET_SELECTABLE(obj, 1);
    connect_object(obj);

    button->strCallback = NULL;
    button->CallBack = CallBack;
    button->cb_data = cb_data;
    button->enable = 1;

    return obj;
}

guiObject_t *GUI_CreateButtonPlateText(guiButton_t *button, u16 x, u16 y, u16 width, u16 height,
    const struct LabelDesc *desc, const char *(*strCallback)(struct guiObject *, const void *),
    void (*CallBack)(struct guiObject *obj, const void *data), const void *cb_data)
{
    guiObject_t *obj = GUI_CreateButton(button, x, y, BUTTON_DEVO10, strCallback, CallBack, cb_data);
    struct guiBox *box = &obj->box;
    button->desc = *desc;
    box->width = width;
    box->height = height;
    return obj;
}

void GUI_DrawButton(struct guiObject *obj)
{
    _DrawButton(obj);
}

int GUI_ButtonWidth(enum ButtonType type)
{
    const struct ImageMap *map = _button_image_map(type);
    return map->width;
}

int GUI_ButtonHeight(enum ButtonType type)
{
    const struct ImageMap *map = _button_image_map(type);
    return map->height;
}

void GUI_ButtonEnable(struct guiObject *obj, u8 enable)
{
    struct guiButton *button = (struct guiButton *)obj;
    if (button->enable != enable) {
        button->enable = enable;
        OBJ_SET_DIRTY(obj, 1);
    }
}

u8 GUI_IsButtonEnabled(struct guiObject *obj)
{
    struct guiButton *button = (struct guiButton *)obj;
    return button->enable;
}
