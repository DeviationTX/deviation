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

#define KEY_ADJUST_Y 1 /* ensure cooridnate is within button */
#define KEY_ADJUST_X 1

void _DrawTextSelectHelper(struct guiTextSelect *select, const char *str)
{
    struct guiBox *box = &select->header.box;
    // plate text select for devo 10, copy most behavior from label.c
    GUI_DrawBackground(box->x, box->y, box->width, box->height);
    u8 arrow_width = ARROW_WIDTH - 1;
    if (select->enable  & 0x01) {
        u16 y = box->y + box->height / 2;  // Bug fix: since the logic view is introduce, a coordinate could be greater than 10000
        u16 x1 = box->x + arrow_width -1;
        LCD_DrawLine(box->x, y, x1, y - 2, 0xffff);
        LCD_DrawLine(box->x, y, x1, y + 2, 0xffff); //"<"
        x1 = box->x + box->width - arrow_width;
        u16 x2 = box->x + box->width -1;
        LCD_DrawLine(x1, y - 2, x2, y, 0xffff);
        LCD_DrawLine(x1, y + 2, x2, y, 0xffff); //">"
    }  else if (select->enable == 2) {  // ENBALBE == 2 means the textsel can be pressed but not be selected
        select->desc.style = LABEL_BOX;
    } else {
        if (!select->enable)  // avoid drawing button box when it is disable
            select->desc.style = LABEL_CENTER;
    }
    GUI_DrawLabelHelper(box->x + arrow_width , box->y, box->width - 2 * arrow_width , box->height,
            str, &select->desc, (guiObject_t *)select == objSELECTED);
}
