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
void _bargraph_trim_horizontal(int x, int y, int width, int height, s32 val, u32 color,
        struct guiBarGraph *graph, struct disp_bargraph *disp, struct guiBox *box)
{
    (void)color;
    (void)disp;
    GUI_DrawBackground(x, y, width, height);  // to clear back ground
    u16 ypos1 = box->y -1;            // lower tic
    u16 ypos2 = ypos1 + box->height;  // upper tic
    u16 xboxw = box->width -1;        // constant   
    LCD_DrawFastVLine(box->x + xboxw / 2, ypos1, 2, 1); //Center
    LCD_DrawFastVLine(box->x + xboxw / 2, ypos2, 2, 1); //Center
    s16 xpos = 0;
    if ((graph->max > 100 && graph->max <= 200) && abs(graph->min) == graph->max) {
        u8 pos100 = xboxw * (100 - graph->min)/(graph->max - graph->min);
        xpos = graph->direction == TRIM_HORIZONTAL ? box->x + pos100: box->x + xboxw - pos100;
        LCD_DrawFastVLine(xpos, ypos1, 2, 1); // +100% position
        LCD_DrawFastVLine(xpos, ypos2, 2, 1);

        pos100 = xboxw * (-100 - graph->min)/(graph->max - graph->min);
        xpos = graph->direction == TRIM_HORIZONTAL ? box->x + pos100: box->x + xboxw - pos100;
        LCD_DrawFastVLine(xpos, ypos1, 2, 1); // -100% position
        LCD_DrawFastVLine(xpos, ypos2, 2, 1);
    }
    s32 val_scale = xboxw * (val - graph->min) / (graph->max - graph->min);
    xpos = graph->direction == TRIM_HORIZONTAL
              ? box->x + val_scale
              : box->x + xboxw - val_scale;
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
    u16 xpos1 = box->x -1;          // left tic
    u16 xpos2 = xpos1 + box->width; // right tic    
    u16 yboxh = box->height -1;     // constant    
    LCD_DrawFastHLine(xpos1, box->y + yboxh / 2, 2, 1); //Center
    LCD_DrawFastHLine(xpos2, box->y + yboxh / 2, 2, 1); //Center
    s16 ypos = 0;
    if ((graph->max > 100 && graph->max <= 200) && abs(graph->min) == graph->max) {
        u8 pos100 = yboxh * (100 - graph->min)/(graph->max - graph->min);
        ypos = box->y + yboxh - pos100;
        LCD_DrawFastHLine(xpos1, ypos, 2, 1); // +100% position
        LCD_DrawFastHLine(xpos2, ypos, 2, 1);

        pos100 = yboxh * (-100 - graph->min)/(graph->max - graph->min);
        ypos = box->y + yboxh - pos100;
        LCD_DrawFastHLine(xpos1, ypos, 2, 1); // -100% position
        LCD_DrawFastHLine(xpos2, ypos, 2, 1);
    }
    s32 center = (graph->max + graph->min) / 2;
    s32 val_scale = yboxh * (val - graph->min) / (graph->max - graph->min);
    ypos = box->y + yboxh - val_scale;
    if (val == center) {
        LCD_FillRect(box->x, ypos - 1, box->width, 3, 1);
    } else {
        LCD_DrawFastHLine(box->x, ypos , box->width, 1);
    }
}
