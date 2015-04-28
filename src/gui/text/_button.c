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

const struct ImageMap *_button_image_map(enum ButtonType type)
{
    switch (type) {
        case BUTTON_DEVO10: return &image_map[DRAW_BTN32_15]; break;
        default:
            break;
    }
    return NULL;
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
    u16 w = box->width;
    u16 h = box->height;
    LCD_GetStringDimensions((u8 *) txt, &text_w, &text_h);
    if (box->width == 0)
        w = text_w;
    if (box->height == 0)
        h = text_h;
    GUI_DrawLabelHelper(obj->box.x, obj->box.y, w, h, txt, &button->desc, obj == objSELECTED);
    return;
}
