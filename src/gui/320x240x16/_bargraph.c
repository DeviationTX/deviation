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

#define TRANSPARENT_BARGRAPH
#define TRIM_THICKNESS          7
#define TRIM_INNER_THICKNESS    0
#define TRIM_MARGIN             1


u32 _bargraph_get_color(s32 val, struct guiBarGraph *graph, struct disp_bargraph *disp)
{
    s32 center = (graph->max + graph->min) / 2;
    return val > center
           ? disp->fg_color_pos
           : val < center
             ? disp->fg_color_neg
             : disp->fg_color_zero;
}
void _bargraph_trim_horizontal(int x, int y, int width, int height, s32 val, u32 color,
        struct guiBarGraph *graph, struct disp_bargraph *disp, struct guiBox *box)
{
    (void)box;
    val = (TRIM_THICKNESS / 2) + (width - TRIM_THICKNESS) * (val - graph->min) / (graph->max - graph->min);
    s16 xpos = graph->direction == TRIM_HORIZONTAL ? x + val : x + width - val;
    if (Display.flags & TRIM_TRANSPARENT) {
        GUI_DrawBackground(x, y, width, height);
    } else {
        LCD_FillRect(x, y, width, height, disp->bg_color);
    }

    //Bar trim points
    int hmap[] = {height / 2, 3 * height / 4, height / 2, height, height / 2, 3 * height / 4, height / 2};
    for (int i = 0; i < 7; i++) {
        LCD_DrawFastVLine(x + (i + 1) * width / 8, y, hmap[i], disp->outline_color);
    }
    // Main rectangle
    if (TRIM_THICKNESS) {
        LCD_FillRect(xpos - (TRIM_THICKNESS / 2)-1,
                     y + TRIM_MARGIN,
                     TRIM_THICKNESS, height - TRIM_MARGIN * 2, color);
    }
    // Marking rectangle
    if (TRIM_INNER_THICKNESS) {
        LCD_FillRect(xpos - (TRIM_INNER_THICKNESS / 2)-1,
                     y + TRIM_MARGIN * 2,
                     TRIM_INNER_THICKNESS, height - TRIM_MARGIN * 4, disp->outline_color);
    }
}
void _bargraph_trim_vertical(int x, int y, int width, int height, s32 val, u32 color,
        struct guiBarGraph *graph, struct disp_bargraph *disp, struct guiBox *box)
{
    (void)box;
    val = (TRIM_THICKNESS / 2) + (height - TRIM_THICKNESS) * (val - graph->min) / (graph->max - graph->min);
    if (Display.flags & TRIM_TRANSPARENT) {
        GUI_DrawBackground(x, y, width, height);
    } else {
        LCD_FillRect(x, y, width, height, disp->bg_color);
    }

    //Bar trim points
    int wmap[] = {width / 2, 3 * width / 4, width / 2, width, width / 2, 3 * width / 4, width / 2};
    for (int i = 0; i < 7; i++) {
        LCD_DrawFastHLine(x, y + (i + 1) * height / 8, wmap[i], disp->outline_color);
    }
    // Main rectangle
    if (TRIM_THICKNESS) {
        LCD_FillRect(x + TRIM_MARGIN,
                     y + (height - val) - (TRIM_THICKNESS / 2) - 1,
                     width - TRIM_MARGIN * 2, TRIM_THICKNESS, color);
    }
    // Marking rectangle
    if (TRIM_INNER_THICKNESS) {
    LCD_FillRect(x + TRIM_MARGIN * 2,
                 y + (height - val) - (TRIM_INNER_THICKNESS / 2) - 1,
                 width - TRIM_MARGIN * 4, TRIM_INNER_THICKNESS, disp->outline_color);
    }
}
