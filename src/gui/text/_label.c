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

//char SELECT_CHAR[] = "☼";
static const char SELECT_CHAR = ']';
void GUI_DrawLabelHelper(u16 obj_x, u16 obj_y, u16 obj_width, u16 obj_height,
        const char *str, const struct LabelDesc *desc, u8 is_selected) {
	u16 txt_w, txt_h;
    (void)obj_height;
    LCD_GetStringDimensions((const u8 *)str, &txt_w, &txt_h);

    if (desc && (desc->style == LABEL_INVERTED || (desc->style != LABEL_FILL && is_selected))) {
        txt_w++;
        LCD_PrintCharXY(obj_x++, obj_y, SELECT_CHAR);
    }

    //printf("%s, %d, %d\n", tempstring, txt_w, obj_width);
    LCD_PrintStringXY(obj_x, obj_y, str);
    for(;txt_w < obj_width;txt_w++)
    	LCD_PrintCharXY(obj_x+txt_w, obj_y, ' ');
}
