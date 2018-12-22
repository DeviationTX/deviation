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

#ifdef _DEVO_F12E_TARGET_H_
void GUI_DrawLabelHelper(u16 obj_x, u16 obj_y, u16 obj_width, u16 obj_height,
        const char *str, const struct LabelDesc *desc, u8 is_selected) {

    u16 txt_w, txt_h;
    u16 cx, cy;

    LCD_GetStringDimensions((const u8 *)str, &txt_w, &txt_h);
    LCD_GetCharDimensions(LCD_SELECT_CHAR, &cx, &cy);

    txt_w += cx;
    if (obj_width == 0)
        obj_width = txt_w;
    if (obj_height == 0)
        obj_height = txt_h;

    // Caculate alignment
    u8 style = 0;
    if (desc && (desc->style == LABEL_INVERTED || (desc->style != LABEL_FILL && is_selected))) {
        // switch the forecolor and backcolor
        style |= FONT_INVERTED;
    }
    else if (desc && desc->style == LABEL_UNDERLINE)
        style |= FONT_UNDERLINE;
    LCD_SetFontStyle(style);
    printf("|%s| [%dx%d] - 0x%x\n", str, txt_w, obj_width, style);
    LCD_PrintStringXY(obj_x, obj_y, str);
    if (style)
        LCD_SetFontStyle(0);
}
#else
void GUI_DrawLabelHelper(u16 obj_x, u16 obj_y, u16 obj_width, u16 obj_height,
        const char *str, const struct LabelDesc *desc, u8 is_selected) {

    u16 txt_w, txt_h;
    u16 cx, cy;

    LCD_GetStringDimensions((const u8 *)str, &txt_w, &txt_h);
    LCD_GetCharDimensions(LCD_SELECT_CHAR, &cx, &cy);

    txt_w += cx;
    if (obj_width == 0)
        obj_width = txt_w;
    if (obj_height == 0)
        obj_height = txt_h;

    if (desc && (desc->style == LABEL_INVERTED || (desc->style != LABEL_FILL && is_selected))) {
        LCD_PrintCharXY(obj_x, obj_y, LCD_SELECT_CHAR);
        obj_x += cx;
    }
    else {
        GUI_DrawBackground(obj_x, obj_y, obj_width, obj_height);
    }
    //printf("%s, %d, %d\n", tempstring, txt_w, obj_width);
    LCD_PrintStringXY(obj_x, obj_y, str);
}
#endif
