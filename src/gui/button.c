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

guiObject_t *GUI_CreateButton(u16 x, u16 y, enum ButtonType type,
    const char *(*strCallback)(struct guiObject *, const void *), u16 fontColor,
    void (*CallBack)(struct guiObject *obj, const void *data), const void *cb_data)
{
    struct guiObject *obj    = GUI_GetFreeObj();
    struct guiButton *button;
    struct guiBox    *box;

    if (obj == NULL)
        return NULL;

    box = &obj->box;
    button = &obj->o.button;

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

    button->fontColor = fontColor;
    button->strCallback = strCallback;
    button->CallBack = CallBack;
    button->cb_data = cb_data;
    button->enable = 1;

    return obj;
}

guiObject_t *GUI_CreateIcon(u16 x, u16 y, const struct ImageMap *image,
        void (*CallBack)(struct guiObject *obj, const void *data), const void *cb_data)
{
    struct guiObject *obj    = GUI_GetFreeObj();
    struct guiButton *button;
    struct guiBox    *box;

    if (obj == NULL)
        return NULL;

    box = &obj->box;
    button = &obj->o.button;
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

guiObject_t *GUI_CreateButtonPlateText(u16 x, u16 y, u16 width, u16 height, const struct LabelDesc *desc,
    const char *(*strCallback)(struct guiObject *, const void *), u16 fontColor,
    void (*CallBack)(struct guiObject *obj, const void *data), const void *cb_data)
{
    struct guiObject  *obj = GUI_CreateButton(x, y, BUTTON_DEVO10, strCallback, fontColor, CallBack, cb_data);
    struct guiButton *button = &obj->o.button;
    struct guiBox *box = &obj->box;
    button->desc = *desc;
    box->width = width;
    box->height = height;
    return obj;
}

void GUI_DrawButton(struct guiObject *obj)
{
    struct guiButton *button = &obj->o.button;
    struct guiBox *box = &obj->box;
    const char *txt;
    u16 x_off, y_off;

    if (button->image->file != NULL) {
        GUI_DrawImageHelper(box->x, box->y, button->image, obj == objTOUCHED ? DRAW_PRESSED : DRAW_NORMAL);
        if (button->strCallback) {
            u16 text_w, text_h;
            LCD_SetFont(DEFAULT_FONT.font); //Set Font here so callback can calculate size
            txt = button->strCallback(obj, button->cb_data);
            if (txt) {
                LCD_GetStringDimensions((u8 *) txt, &text_w, &text_h);
                x_off = (box->width - text_w) / 2 + box->x;
                y_off = (box->height - text_h) / 2 + box->y + 1;
                LCD_SetFontColor(button->fontColor);
                LCD_PrintStringXY(x_off, y_off, txt);
            }
        }
    } else {  // plate-text button for Devo 10
#define BUTTON_ROUND 3
        LCD_SetFont(button->desc.font); //Set Font here so callback can calculate size
        if (button->strCallback)
            txt = button->strCallback(obj, button->cb_data);
        else
            txt = (const char *)button->cb_data;
        u16 text_w, text_h;
        u16 w = box->width;
        u16 h = box->height;
        LCD_GetStringDimensions((u8 *) txt, &text_w, &text_h);
        if (box->width == 0)
            w = text_w;
        if (box->height == 0)
            h = text_h;
        if (obj == objSELECTED) {
            LCD_FillRoundRect(obj->box.x, obj->box.y, w, h , BUTTON_ROUND, 1);
            LCD_SetFontColor(0);
        }  else {
            LCD_FillRoundRect(obj->box.x, obj->box.y, w, h , BUTTON_ROUND, 0); // clear the background
            if (button->enable)
                LCD_DrawRoundRect(obj->box.x, obj->box.y, w, h , BUTTON_ROUND,  1);
            /* Bracket style button, disable temporarily
            u16 y1 = obj->box.y + 2;
            u16 y2 = obj->box.y + obj->box.height -3;
            u16 x1 = obj->box.x + obj->box.width - 1;
            LCD_DrawLine(obj->box.x, y1, obj->box.x + 2, obj->box.y, 1);
            LCD_DrawLine(obj->box.x, y2, obj->box.x + 2, obj->box.y + obj->box.height -1, 1);
            LCD_DrawLine(obj->box.x, y1, obj->box.x, y2, 1);
            LCD_DrawLine(x1, y1, x1 - 2, obj->box.y, 1);
            LCD_DrawLine(x1, y2, x1 - 2, obj->box.y + obj->box.height -1, 1);
            LCD_DrawLine(x1, y1, x1, y2, 1); */
            LCD_SetFontColor(0xffff);
        }
        // bug fix: if the string width is wider than box width, e.g. changing to Chinese, x_off might be very big(actuall
        //it is negative)
        if (box->width > text_w)
            x_off = (box->width - text_w) / 2 + box->x +1;
        else
            x_off = box->x +1;
        if (box->height > text_h)
            y_off = (box->height - text_h) / 2 + box->y + 1;
        else
            y_off = box->y;
        LCD_PrintStringXY(x_off, y_off, txt);
    }
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
    struct guiButton *button = &obj->o.button;
    if (button->enable != enable) {
        button->enable = enable;
        OBJ_SET_DIRTY(obj, 1);
    }
}

u8 GUI_IsButtonEnabled(struct guiObject *obj)
{
    struct guiButton *button = &obj->o.button;
    return button->enable;
}
