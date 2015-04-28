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
void _DrawTextSelectHelper(struct guiTextSelect *select, const char *str)
{
    struct guiBox *box = &select->header.box;
        int selected = 0;
        int is_selected = (guiObject_t *)select == objSELECTED;
        GUI_DrawBackground(box->x, box->y, box->width, box->height);
        if (select->enable  & 0x01) {
            u16 x1 = box->x + box->width;
            if (select->enable  & 0x02) {
            if (is_selected) {
                LCD_PrintStringXY(box->x-1, box->y, "<");
                LCD_PrintStringXY(x1, box->y, ">");
            } else {
                LCD_PrintStringXY(box->x-1, box->y, " ");
                LCD_PrintStringXY(x1, box->y, " ");
            }
            } else {
            if (is_selected) {
                LCD_PrintStringXY(box->x-1, box->y, "[");
                LCD_PrintStringXY(x1, box->y, "]");
            } else {
                LCD_PrintStringXY(box->x-1, box->y, " ");
                LCD_PrintStringXY(x1, box->y, " ");
            }
            }
        }  else if (select->enable == 2) {  // ENBALBE == 2 means the textsel can be pressed but not be selected
            //FIXME
        }
        else {
            if (!select->enable)  // avoid drawing button box when it is disable
                //FIXME
            if (is_selected) {
                selected = 1;
            }
        }
        GUI_DrawLabelHelper(box->x + 1 , box->y, box->width - 2 , box->height,
                str, &select->desc, selected);
}
