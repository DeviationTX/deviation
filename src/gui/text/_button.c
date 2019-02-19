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

#include "lcd_page_props.h"

const struct ImageMap *_button_image_map(enum ButtonType type)
{
    (void)type;
    return &image_map[0];
}

void _DrawButton(struct guiObject *obj)
{
    struct guiButton *button = (struct guiButton *)obj;
    struct guiBox *box = &obj->box;
    const char *txt;

    if (button->strCallback)
        txt = button->strCallback(obj, button->cb_data);
    else
        txt = (const char *)button->cb_data;
    u16 text_w, text_h;
    unsigned w = box->width;
    unsigned h = box->height;
    unsigned x = obj->box.x;
    
    LCD_GetStringDimensions((u8 *) txt, &text_w, &text_h);
    if (box->width == 0)
        w = text_w;
    if (box->height == 0)
        h = text_h;

    GUI_DrawLabelHelper(x, obj->box.y, w, h, txt, &button->desc, obj == objSELECTED);
    return;
}
