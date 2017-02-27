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
    u16 offset = 0;
    LCD_SetFont(desc->font);
    LCD_GetStringDimensions((const u8 *)str, &txt_w, &txt_h); txt_w++;
    if (obj_w == 0)
        obj_w = txt_w;
    if (obj_h == 0)
        obj_h = txt_h;
    if (offset && obj_y >= offset) {
        obj_y -= offset;
    }
    GUI_DrawBackground(obj_x, obj_y, obj_w, obj_h);
    if (desc->style == LABEL_BOX || desc->style == LABEL_BRACKET || desc->style == LABEL_SQUAREBOX) {
        // draw round rect for the textsel widget when it is pressable
        if (is_selected) {
            if (desc->style == LABEL_SQUAREBOX ||obj_w < 5)
                LCD_FillRect(obj_x, obj_y, obj_w, obj_h , 1);
            else
                LCD_FillRoundRect(obj_x, obj_y, obj_w, obj_h , 3, 1);
        }  else {
            if (desc->style == LABEL_SQUAREBOX)
                if (desc->fill_color == 0)
                    LCD_DrawRect(obj_x, obj_y, obj_w, obj_h, 1);
                else
                    LCD_FillRect(obj_x, obj_y, obj_w, obj_h, desc->fill_color);
            else if (desc->style == LABEL_BRACKET) {
                struct pos { int i1; int i2; int i3; int i4;};
                struct { union { int p[4]; struct pos pos;} u;} x[2], y[2];
                //int x[2][4];
                //int y[2][4];
                if (obj_h > 2 * obj_w) {
                    x[0].u.pos = (struct pos){ 0, 2, obj_w -3, obj_w -1};
                    y[0].u.pos = (struct pos){ 2, 0, 0, 2};
                    x[1].u.pos = x[0].u.pos;
                    y[1].u.pos = (struct pos){ obj_h -3, obj_h -1, obj_h -1, obj_h -3};
                } else {
                    x[0].u.pos = (struct pos){ 2, 0, 0, 2};
                    y[0].u.pos = (struct pos){ 0, 2, obj_h -3, obj_h -1};
                    x[1].u.pos = (struct pos){ obj_w -3, obj_w -1, obj_w -1,  obj_w -3};
                    y[1].u.pos = y[0].u.pos;
                }
                for(int oc = 0; oc < 2; oc++) {
                    for(int i = 0; i < 3; i++) {
                        LCD_DrawLine(obj_x + x[oc].u.p[i], obj_y+y[oc].u.p[i], obj_x+x[oc].u.p[i+1], obj_y + y[oc].u.p[i+1], 1);
                    }
                }
            } else
                LCD_DrawRoundRect(obj_x, obj_y, obj_w, obj_h , 3,  1);
        }
    }
    else if (desc->style == LABEL_INVERTED || is_selected) {
        LCD_FillRect(obj_x, obj_y, obj_w, obj_h, 0xffff);
    }
    else if (desc->style == LABEL_FILL) {
        LCD_FillRect(obj_x, obj_y, obj_w, obj_h, desc->fill_color);
    }

    if (desc->align == ALIGN_RIGHT) {
        txt_x = obj_x + obj_w - txt_w;
    } else if (obj_w > txt_w && !(desc->align == ALIGN_LEFT)) {
        txt_x = obj_x + 1 + (obj_w - txt_w + 1) / 2;
    } else {
        txt_x = obj_x + 1;
    }
    txt_y = obj_y + offset + (obj_h - txt_h + 1) / 2;

    if (desc->style == LABEL_UNDERLINE) {
        LCD_DrawFastHLine(--txt_x, txt_y + txt_h - 1, obj_w, 1);
    }
    if (desc->style == LABEL_INVERTED || is_selected) {
        LCD_SetFontColor(~desc->font_color);
    } else {
        LCD_SetFontColor(desc->font_color);
    }
    LCD_PrintStringXY(txt_x, txt_y, str);
}

