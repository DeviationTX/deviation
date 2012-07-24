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

guiObject_t *GUI_CreateImageOffset(u16 x, u16 y, u16 width, u16 height, u16 x_off, u16 y_off, const char *file,
    void (*CallBack)(guiObject_t *obj, s8 press_type, void *data), void *cb_data)
{
    struct guiObject *obj = GUI_GetFreeObj();
    struct guiImage  *image;
    struct guiBox    *box;

    if (obj == NULL)
        return NULL;

    box = &obj->box;
    image = &obj->o.image;

    image->file = file;
    image->x_off = x_off;
    image->y_off = y_off;
    image->callback = CallBack;
    image->cb_data = cb_data;

    box->x = x;
    box->y = y;
    box->width = width;
    box->height = height;

    obj->Type = Image;
    OBJ_SET_TRANSPARENT(obj, LCD_ImageIsTransparent(file));
    OBJ_SET_SELECTABLE(obj, CallBack ? 1 :0);
    connect_object(obj);

    return obj;

}

void GUI_DrawImage(struct guiObject *obj)
{
    struct guiImage *image = &obj->o.image;
    struct guiBox *box = &obj->box;
    LCD_DrawWindowedImageFromFile(box->x, box->y,
            image->file, box->width, box->height,
            image->x_off, image->y_off);
}


u8 GUI_TouchImage(struct guiObject *obj, struct touch *coords, s8 press_type)
{
    (void)coords;
    struct guiImage *image = &obj->o.image;
    image->callback(obj, press_type, image->cb_data);
    return 1;
}

void GUI_ChangeImage(struct guiObject *obj, const char *file, u16 x_off, u16 y_off)
{
    struct guiImage *image = &obj->o.image;
    if (image->file != file || image->x_off != x_off || image->y_off != y_off) {
        image->file = file;
        image->x_off = x_off;
        image->y_off = y_off;
        OBJ_SET_DIRTY(obj, 1);
    }
}
