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
#define DIALOG_TXTOFFSET_X 2

void _draw_dialog_box(struct guiBox *box, int x, const char *title)
{
	(void) x;
	(void) title;
    u16 i,j;
    char spaces[LCD_WIDTH+1];
	for(i = 0; i <= box->height; i++) {
    	for(j = 0; j <= (box->width-box->x) || j <= LCD_WIDTH; j++)
    		spaces[j] = 0x20;
    	spaces[box->width+1] = 0;
    	LCD_PrintStringXY(box->x, box->y+i, spaces);
  	}
}

void _dialog_draw_background(u16 x, u16 y, u16 w, u16 h)
{
	u16 i,j;
    char spaces[LCD_WIDTH+1];
	for(i = 0; i <= h; i++) {
    	for(j = 0; j <= (w-x) || j <= LCD_WIDTH; j++)
    		spaces[j] = 0x20;
    	spaces[w+1] = 0;
    	LCD_PrintStringXY(x, y+i, spaces);
  	}
}
