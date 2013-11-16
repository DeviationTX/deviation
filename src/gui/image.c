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

guiObject_t *GUI_CreateImageOffset(guiImage_t *image, u16 x, u16 y, u16 width, u16 height, u16 x_off, u16 y_off, const char *file,
    void (*CallBack)(guiObject_t *obj, s8 press_type, const void *data), const void *cb_data)
{
    struct guiObject *obj = (guiObject_t *)image;
    struct guiBox    *box;
    CLEAR_OBJ(image);

    box = &obj->box;

    image->file = file;
    image->x_off = x_off;
    image->y_off = y_off;
    image->callback = CallBack;
    image->cb_data = cb_data;
    image->crc = Crc(file, strlen(file));

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
#define SELECT_BORDER_OFFSET 1
    struct guiImage *image = (struct guiImage *)obj;
    struct guiBox *box = &obj->box;

    //  clear the whole widget, including its selected border for devo10/7e
    if (LCD_DEPTH == 1)
        GUI_DrawBackground(box->x -SELECT_BORDER_OFFSET, box->y -SELECT_BORDER_OFFSET,
            box->width + SELECT_BORDER_OFFSET + SELECT_BORDER_OFFSET, box->height + SELECT_BORDER_OFFSET + SELECT_BORDER_OFFSET);

    LCD_DrawWindowedImageFromFile(box->x, box->y,
            image->file, box->width, box->height,
            image->x_off, image->y_off);

    if (LCD_DEPTH == 1 && GUI_GetSelected() == obj)
        LCD_DrawRect(box->x -SELECT_BORDER_OFFSET, box->y -SELECT_BORDER_OFFSET,
            box->width + SELECT_BORDER_OFFSET + SELECT_BORDER_OFFSET, box->height + SELECT_BORDER_OFFSET + SELECT_BORDER_OFFSET, 1);
}


u8 GUI_TouchImage(struct guiObject *obj, struct touch *coords, s8 press_type)
{
    (void)coords;
    struct guiImage *image = (struct guiImage *)obj;
    image->callback(obj, press_type, image->cb_data);
    return 1;
}

void _GUI_ChangeImage(struct guiImage *image, const char *file, u16 x_off, u16 y_off, u8 replace)
{
    guiObject_t *obj = (guiObject_t *)image;
    //Use a CRC for comparison because the filename may change without the pointer changing
    u32 crc = Crc(file, strlen(file));
    if (image->file != file || image->crc != crc || image->x_off != x_off || image->y_off != y_off) {
        if (replace) {
            // draw background where the old picture was bigger
            struct guiBox *box = &obj->box;
            u16 w, h;
            LCD_ImageDimensions(file, &w, &h);
            if (h < box->height) GUI_DrawBackground(box->x, box->y + h, box->width, box->height - h);    // remove lower left part of old image
            if (w < box->width) GUI_DrawBackground(box->x + w, box->y, box->width - w, h < box->height ? h : box->height);    // remove upper right part of old image
            box->width = w;
            box->height = h;
        }
        image->crc = crc;
        image->file = file;
        image->x_off = x_off;
        image->y_off = y_off;
        OBJ_SET_TRANSPARENT(obj, LCD_ImageIsTransparent(file));
        OBJ_SET_DIRTY(obj, 1);
    }
}

void GUI_ChangeImage(struct guiImage *image, const char *file, u16 x_off, u16 y_off)
{
    _GUI_ChangeImage(image, file, x_off, y_off, 0);
}

void GUI_ReplaceImage(struct guiImage *image, const char *file, u16 x_off, u16 y_off)
{
    _GUI_ChangeImage(image, file, x_off, y_off, 1);
}
