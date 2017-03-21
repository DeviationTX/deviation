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

#define DIALOG_HEADER_Y 0
#define DIALOG_TXTOFFSET_X 2*ITEM_SPACE
#define OK_WIDTH 3*ITEM_SPACE
#define CANCEL_WIDTH 7*ITEM_SPACE
#define DIALOG_SINGLE_BUTTON_X (x + (width - (dgType == dtOk ? OK_WIDTH : CANCEL_WIDTH)) / 2)
#define DIALOG_SINGLE_BUTTON_Y (y + height - 2*LINE_HEIGHT)
#define DIALOG_DUAL_BUTTON_X1 (x + 3*ITEM_SPACE)
#define DIALOG_DUAL_BUTTON_X2 (x + width - CANCEL_WIDTH - 3*ITEM_SPACE)
#define DIALOG_DUAL_BUTTON_Y (y + height - 2*LINE_HEIGHT)

void _draw_dialog_box(struct guiBox *box, int x, const char *title)
{
    (void) x;
    (void) title;
    int i, j;
    char fill[LCD_WIDTH/ITEM_SPACE + 1];
    int width = box->x + box->width > LCD_WIDTH ? LCD_WIDTH - box->x : box->width;
    fill[0] = ' ';
    fill[width/ITEM_SPACE - 1] = ' ';
    for (i = 1; i < width/ITEM_SPACE - 1; i++) {
         fill[i] = '-';
    }
    fill[width/ITEM_SPACE] = 0;
    LCD_PrintStringXY(box->x, box->y, fill);
    LCD_PrintStringXY(box->x, box->y + box->height - 1*LINE_HEIGHT, fill);
    fill[0] = 'l';
    fill[width/ITEM_SPACE - 1] = 'l';
    for(j = 1; j < width/ITEM_SPACE - 1; j++) {
        fill[j] = ' ';
    }
    fill[width/ITEM_SPACE] = 0;
    for(i = 1; i < box->height/LINE_HEIGHT - 1; i++) {
        LCD_PrintStringXY(box->x, box->y + i*LINE_HEIGHT, fill);
    }
}

void _dialog_draw_background(u16 x, u16 y, u16 w, u16 h)
{
    unsigned i, j;
    char spaces[LCD_WIDTH/ITEM_SPACE + 1];
    if (w + x > LCD_WIDTH)
        w = LCD_WIDTH - x;

    for(j = 0; j < w/ITEM_SPACE; j++)
        spaces[j] = ' ';
    spaces[w/ITEM_SPACE] = 0;

    for(i = 0; i < h/LINE_HEIGHT; i++) {
        LCD_PrintStringXY(x, y + i*LINE_HEIGHT, spaces);
    }
}
