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
enum {
    FLAG_ENABLE = 0x01,
    FLAG_LONGPRESS = 0x02,
};

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

    button->strCallback = strCallback;
    button->CallBack = CallBack;
    button->cb_data = cb_data;
    button->flags |= FLAG_ENABLE;

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
    button->flags |= FLAG_ENABLE;

    return obj;
}

guiObject_t *GUI_CreateButtonPlateText(guiButton_t *button, u16 x, u16 y, u16 width, u16 height, const struct LabelDesc *desc,
    const char *(*strCallback)(struct guiObject *, const void *),
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
    int is_enabled = button->flags & FLAG_ENABLE;
    enable = enable ? FLAG_ENABLE : 0;
    if (enable ^ is_enabled) {
        button->flags = (button->flags & ~FLAG_ENABLE) | enable;
        OBJ_SET_DIRTY(obj, 1);
    }
}

unsigned GUI_IsButtonEnabled(struct guiObject *obj)
{
    struct guiButton *button = (struct guiButton *)obj;
    return button->flags & FLAG_ENABLE;
}

unsigned GUI_IsButtonLongPress(struct guiObject *obj)
{
    struct guiButton *button = (struct guiButton *)obj;
    return button->flags & FLAG_LONGPRESS;
}

int GUI_TouchButton(struct guiObject *obj, int press_type)
{
    //press_type: 1=long_press, -1=release
    struct guiButton *button = (struct guiButton *)obj;
    if (press_type == 0) {
        button->flags &= ~FLAG_LONGPRESS;
    } else if (press_type == 1) {
        button->flags |= FLAG_LONGPRESS;
        return 0;
    }
    OBJ_SET_DIRTY(objTOUCHED, 1);
    if (press_type == -1) {
        if(button->CallBack) {
            button->CallBack(objTOUCHED, button->cb_data);
            //The object may have been destroyed by now, the obj may be invalid
        }
    }
    return 1;
}
