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
#define DIALOG_HEADER_Y 24
void _draw_dialog_box(struct guiBox *box, int x, const char *title)
{
    LCD_DrawRect(box->x, box->y, box->width, box->height, DIALOGTITLE_FONT.fill_color);
    LCD_FillRect(box->x, box->y, box->width, DIALOG_HEADER_Y, DIALOGTITLE_FONT.fill_color);
    LCD_FillRect(box->x + 1, box->y + DIALOG_HEADER_Y, box->width - 2, box->height - DIALOG_HEADER_Y - 1, DIALOGBODY_FONT.fill_color);
    LCD_SetFont(DIALOGTITLE_FONT.font);
    LCD_SetFontColor(DIALOGTITLE_FONT.font_color);
    LCD_PrintStringXY(x, (box->y + 5), title);
}

void _dialog_draw_background(u16 x, u16 y, u16 w, u16 h)
{
    LCD_FillRect(x, y, w, h, DIALOGBODY_FONT.fill_color);
}
