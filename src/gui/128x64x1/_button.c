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
    u16 x_off, y_off;

#define BUTTON_ROUND 3
    // bug fix: must set the default font, otherwise language other than English might not be displayed
    LCD_SetFont(DEFAULT_FONT.font); //Set Font here so callback can calculate size
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
        LCD_SetFontColor(~DEFAULT_FONT.font_color);
    }  else {
        LCD_FillRoundRect(obj->box.x, obj->box.y, w, h , BUTTON_ROUND, 0); // clear the background
        if (GUI_IsButtonEnabled(obj))
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
        LCD_SetFontColor(DEFAULT_FONT.font_color);
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
