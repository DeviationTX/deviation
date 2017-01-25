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
        case BUTTON_MENUITEM: return &image_map[FILE_BTN_MENUITEM]; break;
        case BUTTON_96:    return &image_map[FILE_BTN96_24]; break;
        case BUTTON_48:    return &image_map[FILE_BTN48_24]; break;
        case BUTTON_96x16: return &image_map[FILE_BTN96_16]; break;
        case BUTTON_64x16: return &image_map[FILE_BTN64_16]; break;
        case BUTTON_48x16: return &image_map[FILE_BTN48_16]; break;
        case BUTTON_32x16: return &image_map[FILE_BTN32_16]; break;
        default: return NULL;
    }
    return NULL;
}

void _DrawButton(struct guiObject *obj)
{
    struct guiBox *box = &obj->box;
    const char *txt;
    u16 x_off, y_off;
    struct guiButton *button = (struct guiButton *)obj;
    GUI_DrawImageHelper(box->x, box->y, button->image, obj == objTOUCHED ? DRAW_PRESSED : DRAW_NORMAL);
    if (button->strCallback) {
        u16 text_w, text_h;
        LCD_SetFont(BUTTON_FONT.font); //Set Font here so callback can calculate size
        txt = button->strCallback(obj, button->cb_data);
        if (txt) {
            LCD_GetStringDimensions((u8 *) txt, &text_w, &text_h);
            x_off = (box->width - text_w) / 2 + box->x;
            y_off = (box->height - text_h) / 2 + box->y + 1;
            LCD_SetFontColor(BUTTON_FONT.font_color);
            LCD_PrintStringXY(x_off, y_off, txt);
        }
    }
}

