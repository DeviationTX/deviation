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

guiObject_t *GUI_CreateImageOffset(u16 x, u16 y, u16 width, u16 height, u16 x_off, u16 y_off, const char *file)
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

    box->x = x;
    box->y = y;
    box->width = width;
    box->height = height;

    obj->Type = Image;
    OBJ_SET_TRANSPARENT(obj, LCD_ImageIsTransparent(file));
    OBJ_SET_USED(obj, 1);
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
