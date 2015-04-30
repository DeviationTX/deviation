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
 
#define TRIM_THICKNESS 10
#define TRIM_MARGIN 1
u32 _bargraph_get_color(s32 val, struct guiBarGraph *graph, struct disp_bargraph *disp)
{
    (void)val;
    (void)graph;
    (void)disp;
    return 1;
}
void _bargraph_trim_horizontal3(int x, int y, int width, int height, s32 val, u32 color,
        struct guiBarGraph *graph, struct disp_bargraph *disp, struct guiBox *box)
{
    (void)color;
    (void)disp;
    GUI_DrawBackground(x, y, width, height);  // to clear back ground
    LCD_DrawFastVLine(box->x + (box->width -1) / 2, box->y -1, 2, 1); //Center
    LCD_DrawFastVLine(box->x + (box->width -1) / 2, box->y + box->height -1, 2, 1); //Center
    s16 xpos = 0;
    if ((graph->max > 100 && graph->max <= 200) && abs(graph->min) == graph->max) {
        u8 pos100 = (box->width -1) * (100 - graph->min)/(graph->max - graph->min);
        xpos = graph->direction == TRIM_HORIZONTAL ? box->x + pos100: box->x + box->width -1 - pos100;
        LCD_DrawFastVLine(xpos, box->y -1, 2, 1); // -100% position
        LCD_DrawFastVLine(xpos, box->y + box->height-1, 2, 1);

        pos100 = (box->width -1) * (-100 - graph->min)/(graph->max - graph->min);
        xpos = graph->direction == TRIM_HORIZONTAL ? box->x + pos100: box->x + box->width -1 - pos100;
        LCD_DrawFastVLine(xpos, box->y -1, 2, 1); // 100% position
        LCD_DrawFastVLine(xpos, box->y + box->height-1, 2, 1);
    }
    s32 val_scale = (box->width -1) * (val - graph->min) / (graph->max - graph->min);
    xpos = graph->direction == TRIM_HORIZONTAL
              ? box->x + val_scale
              : box->x + box->width -1 - val_scale;
    s32 center = (graph->max + graph->min) / 2;
    if (val == center) {
        LCD_FillRect(xpos - 1, box->y, 3, box->height, 1);
    } else {
        LCD_DrawFastVLine(xpos, box->y , box->height, 1);
    }
}

void _bargraph_trim_vertical(int x, int y, int width, int height, s32 val, u32 color,
        struct guiBarGraph *graph, struct disp_bargraph *disp, struct guiBox *box)
{
    (void)color;
    (void)disp;
    GUI_DrawBackground(x, y, width, height);  // to clear back ground
    LCD_DrawFastHLine(box->x -1, box->y + (box->height -1) / 2, 2, 1); //Center
    LCD_DrawFastHLine(box->x + box->width -1, box->y + (box->height -1) / 2, 2, 1); //Center
    s16 ypos = 0;
    if ((graph->max > 100 && graph->max <= 200) && abs(graph->min) == graph->max) {
        u8 pos100 = (box->height -1) * (100 - graph->min)/(graph->max - graph->min);
        ypos = box->y + box->height -1 - pos100;
        LCD_DrawFastHLine(box->x -1, ypos, 2, 1); // -100% position
        LCD_DrawFastHLine(box->x + box->width -1, ypos, 2, 1);

        pos100 = (box->height -1) * (-100 - graph->min)/(graph->max - graph->min);
        ypos = box->y + box->height -1 - pos100;
        LCD_DrawFastHLine(box->x -1, ypos, 2, 1); // -100% position
        LCD_DrawFastHLine(box->x + box->width -1, ypos, 2, 1);
    }
    s32 center = (graph->max + graph->min) / 2;
    s32 val_scale = (box->height -1) * (val - graph->min) / (graph->max - graph->min);
    ypos = box->y + box->height -1 - val_scale;
    if (val == center) {
        LCD_FillRect(box->x, ypos - 1, box->width, 3, 1);
    } else {
        LCD_DrawFastHLine(box->x, ypos , box->width, 1);
    }
}
void  _bargraph_trim_horizontal2(int x, int y, int width, int height)
{
	for(int i=x; i <= width; i++) 
	{
		LCD_PrintStringXY(i, y, "=");
		
	}

	LCD_PrintStringXY(x, y, "[");
	LCD_PrintStringXY(width, y, "]");
	int ancho = (width - x)/2;
	LCD_PrintStringXY(x+ancho, y, "l");
	LCD_PrintStringXY(x+ancho+1, y, "$");
	
	return;
}

void _bargraph_trim_horizontal(int x, int y, int width, int height, s32 val, u32 color,
        struct guiBarGraph *graph, struct disp_bargraph *disp, struct guiBox *box)
{
    (void)color;
    (void)disp;
	for(int i=x; i <= width; i++) 
	{
		LCD_PrintStringXY(i, y, "="); //create the dot line 
		
	}
	LCD_PrintStringXY(x+((width-x)/2), y, "l"); //create the center bar
    //GUI_DrawBackground(x, y, width, height);  // to clear back ground
   //LCD_DrawFastVLine(box->x + (box->width -1) / 2, box->y -1, 2, 1); //Center
   // LCD_DrawFastVLine(box->x + (box->width -1) / 2, box->y + box->height -1, 2, 1); //Center
    s16 xpos = 0;
    if ((graph->max > 100 && graph->max <= 200) && abs(graph->min) == graph->max) {
        u8 pos100 = (box->width -1) * (100 - graph->min)/(graph->max - graph->min);
        xpos = graph->direction == TRIM_HORIZONTAL ? box->x + pos100: box->x + box->width -1 - pos100;
        LCD_DrawFastVLine(xpos, box->y -1, 2, 1); // -100% position
        LCD_DrawFastVLine(xpos, box->y + box->height-1, 2, 1);

        pos100 = (box->width -1) * (-100 - graph->min)/(graph->max - graph->min);
        xpos = graph->direction == TRIM_HORIZONTAL ? box->x + pos100: box->x + box->width -1 - pos100;
        LCD_DrawFastVLine(xpos, box->y -1, 2, 1); // 100% position
        LCD_DrawFastVLine(xpos, box->y + box->height-1, 2, 1);
    }
    s32 val_scale = (box->width -1) * (val - graph->min) / (graph->max - graph->min);
    xpos = graph->direction == TRIM_HORIZONTAL
              ? box->x + val_scale
              : box->x + box->width -1 - val_scale;
    s32 center = (graph->max + graph->min) / 2;
    if (val == center) {
        //LCD_FillRect(xpos - 1, box->y, 3, box->height, 1);
		LCD_PrintStringXY(xpos, box->y, "^");
    } else {
		LCD_PrintStringXY(xpos, box->y, "$");
        //LCD_DrawFastVLine(xpos, box->y , box->height, 1);
    }
}
