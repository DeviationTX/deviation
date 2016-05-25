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

void GUI_DrawLabelHelper(u16 obj_x, u16 obj_y, u16 obj_w, u16 obj_h, const char *str,
        const struct LabelDesc *desc, u8 is_selected)
{
    u16 txt_w, txt_h;
    u16 txt_x, txt_y;
    // This is used to align text vertically
    u16 offset = (desc->font==NARROW_FONT.font) ? 1 : 0;
    LCD_SetFont(desc->font);
    LCD_GetStringDimensions((const u8 *)str, &txt_w, &txt_h); txt_w++;
    if (obj_w == 0)
        obj_w = txt_w;
    if (obj_h == 0)
        obj_h = txt_h;
    if (desc->style == LABEL_LISTBOX) {
        LCD_FillRect(obj_x, obj_y, obj_w, obj_h, is_selected ? Display.listbox.fg_color : Display.listbox.bg_color);
    } else if (desc->style == LABEL_FILL) {
        LCD_FillRect(obj_x, obj_y, obj_w, obj_h, desc->fill_color);
    } else {
        GUI_DrawBackground(obj_x, obj_y, obj_w, obj_h);
    }
    if (desc->style != LABEL_LISTBOX && desc->fill_color != desc->outline_color) {
        LCD_DrawRect(obj_x, obj_y, obj_w, obj_h, desc->outline_color);
        obj_x+=2; obj_w-=4;
    }

    if (desc->style == LABEL_RIGHT) {
        txt_x = obj_x + obj_w - txt_w;
    } else if (obj_w > txt_w && !(desc->style == LABEL_LEFT)) {
        txt_x = obj_x+1 + (obj_w - txt_w + 1) / 2;
    } else {
        txt_x = obj_x+1;
    }
    txt_y = obj_y + offset + (obj_h - txt_h + 1) / 2;

    if (desc->style == LABEL_LISTBOX) {
        LCD_SetFontColor(is_selected ? Display.listbox.fg_select : Display.listbox.bg_select);
    } else {
        if (desc->style != LABEL_FILL && is_selected) {
            LCD_SetFontColor(~desc->font_color);
        } else {
            LCD_SetFontColor(desc->font_color);
        }
    }
    LCD_PrintStringXY(txt_x, txt_y, str);
}

