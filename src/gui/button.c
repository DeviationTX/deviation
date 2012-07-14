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

guiObject_t *GUI_CreateButton(u16 x, u16 y, enum ButtonType type, const char *text,
        u16 fontColor, void (*CallBack)(struct guiObject *obj, void *data), void *cb_data)
{
    struct guiObject *obj    = GUI_GetFreeObj();
    struct guiButton *button;
    struct guiBox    *box;
    u16 text_w, text_h;

    if (obj == NULL)
        return NULL;

    box = &obj->box;
    button = &obj->o.button;

    switch (type) {
        case BUTTON_90: button->image = &image_map[FILE_BTN90_24]; break;
        case BUTTON_45: button->image = &image_map[FILE_BTN46_24]; break;
        case BUTTON_96x16: button->image = &image_map[FILE_BTN96_16]; break;
        case BUTTON_64x16: button->image = &image_map[FILE_BTN64_16]; break;
        case BUTTON_48x16: button->image = &image_map[FILE_BTN48_16]; break;
        case BUTTON_32x16: button->image = &image_map[FILE_BTN32_16]; break;
    }

    box->x = x;
    box->y = y;
    box->width = button->image->width;
    box->height = button->image->height;

    obj->Type = Button;
    OBJ_SET_TRANSPARENT(obj, TEST_SELECT); //No need to set transparency since the image cannot be overlapped, and the file can't change
    OBJ_SET_USED(obj, 1);
    connect_object(obj);

    LCD_GetStringDimensions((u8 *) text, &text_w, &text_h);
    button->text = text;
    button->text_x_off = (box->width - text_w) / 2 + x;
    button->text_y_off = (box->height - text_h) / 2 + y + 2;
    button->fontColor = fontColor;
    button->CallBack = CallBack;
    button->cb_data = cb_data;

    return obj;
}

void GUI_DrawButton(struct guiObject *obj)
{
    struct guiButton *button = &obj->o.button;
    struct guiBox *box = &obj->box;

    GUI_DrawImageHelper(box->x, box->y, button->image, obj == objTOUCHED ? DRAW_PRESSED : DRAW_NORMAL);
    LCD_SetFontColor(button->fontColor);
    LCD_PrintStringXY(button->text_x_off, button->text_y_off, button->text);
}
