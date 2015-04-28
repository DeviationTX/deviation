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
#define DIALOG_HEADER_Y 0
#define DIALOG_TXTOFFSET_X 10

void _draw_dialog_box(struct guiBox *box, int x, const char *title)
{
    (void)x;
    (void)title;
    //Don't show title in devo10 as its screen has limited space
    LCD_FillRect(box->x, box->y, box->width, box->height, 0);  // clear the background
    LCD_DrawRoundRect(box->x, box->y, box->width, box->height,  3, 1);
    LCD_DrawRoundRect(box->x +1, box->y +1, box->width -2, box->height -2,  3, 1);
}

void _dialog_draw_background(u16 x, u16 y, u16 w, u16 h)
{
    LCD_FillRect(x, y, w, h, 0);
}
