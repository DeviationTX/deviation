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

void _DrawTextSelectHelper(struct guiTextSelect *select, const char *str)
{
    u16 x, y, w, h;
    struct guiBox *box = &select->header.box;
// only used for RTC config in Devo12
#if HAS_RTC
    if (select->type == TEXTSELECT_VERT_64) {
        GUI_DrawImageHelper(box->x,
                box->y + ARROW_HEIGHT, select->button, DRAW_NORMAL);
        if (select->enable & 0x01) {
            GUI_DrawImageHelper(box->x + (box->width - ARROW_WIDTH) / 2, box->y, ARROW_UP,
                    select->state & 0x02 ? DRAW_PRESSED : DRAW_NORMAL);
            GUI_DrawImageHelper(box->x + (box->width - ARROW_WIDTH) / 2, box->y + box->height - ARROW_HEIGHT, ARROW_DOWN,
                    select->state & 0x01 ? DRAW_PRESSED : DRAW_NORMAL);
        }
    }
    else
#endif
    {
        GUI_DrawImageHelper(box->x + ARROW_WIDTH,
                box->y, select->button, DRAW_NORMAL);
        if (select->enable & 0x01) {
            GUI_DrawImageHelper(box->x, box->y, ARROW_LEFT,
                    select->state & 0x01 ? DRAW_PRESSED : DRAW_NORMAL);
            GUI_DrawImageHelper(box->x + box->width - ARROW_WIDTH,
                    box->y, ARROW_RIGHT,
                    select->state & 0x02 ? DRAW_PRESSED : DRAW_NORMAL);
        }
    }
    LCD_SetFont(TEXTSEL_FONT.font);
    LCD_SetFontColor(TEXTSEL_FONT.font_color);
    LCD_GetStringDimensions((const u8 *)str, &w, &h);
    x = box->x + (box->width - w) / 2;
    y = box->y + 1 + (box->height - h) / 2; // one pixel higher (+1) to avoid artifacts
    LCD_PrintStringXY(x, y, str);
}
