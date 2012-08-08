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

#include "target.h"
#define ENABLE_GUIOBJECT
#include "gui.h"
#include "config/display.h"

guiObject_t *GUI_CreateBarGraph(u16 x, u16 y, u16 width, u16 height,
                      s16 min, s16 max, u8 direction,
                      s16 (*Callback)(void *data), void *cb_data)
{
    struct guiObject   *obj   = GUI_GetFreeObj();
    struct guiBarGraph *graph;
    struct guiBox    *box;

    if (obj == NULL)
        return NULL;

    box = &obj->box;
    graph = &obj->o.bar;

    box->x = x;
    box->y = y;
    box->width = width;
    box->height = height;

    obj->Type = BarGraph;
    OBJ_SET_TRANSPARENT(obj, 0);
    connect_object(obj);

    graph->min = min;
    graph->max = max;
    graph->direction = direction;
    graph->CallBack = Callback;
    graph->cb_data = cb_data;

    return obj;
}

void GUI_DrawBarGraph(struct guiObject *obj)
{
#define TRANSPARENT_BARGRAPH
#define TRIM_THICKNESS 10
#define TRIM_MARGIN 1
    struct guiBox *box = &obj->box;
    struct guiBarGraph *graph = &obj->o.bar;
    struct disp_bargraph *disp;
    int height = box->height - 2;
    int width  = box->width - 2;
    int x = box->x + 1;
    int y = box->y + 1;

    disp = graph->direction == BAR_HORIZONTAL ||
           graph->direction == BAR_VERTICAL
             ? &Display.bargraph
             : &Display.trim;

    LCD_DrawRect(box->x, box->y, box->width, box->height, disp->outline_color);
    
    s32 val = graph->CallBack(graph->cb_data);
    if (val < graph->min)
        val = graph->min;
    else if (val > graph->max)
        val = graph->max;
    s32 center = (graph->max + graph->min) / 2;
    u16 color = val > center
                ? disp->fg_color_pos
                : val < center
                  ? disp->fg_color_neg
                  : disp->fg_color_zero;
    switch(graph->direction) {
    case BAR_HORIZONTAL: {
        val = width * (val - graph->min) / (graph->max - graph->min);
        LCD_FillRect(x, y, val, height, color);
        if (Display.flags & BAR_TRANSPARENT) {
            GUI_DrawBackground(x + val, y, width - val, height);
        } else {
            LCD_FillRect(x + val, y, width - val, height, disp->bg_color);
        }
        break;
    }
    case BAR_VERTICAL: {
        val = height * (val - graph->min) / (graph->max - graph->min);
        LCD_FillRect(x, y + (height - val), width, val, color);
        if (Display.flags & BAR_TRANSPARENT) {
            GUI_DrawBackground(x, y, width, height - val);
        } else {
            LCD_FillRect(x, y, width, height - val, disp->bg_color);
        }
        break;
    }
    case TRIM_HORIZONTAL: {
        val = (TRIM_THICKNESS / 2) + (width - TRIM_THICKNESS) * (val - graph->min) / (graph->max - graph->min);
        if (Display.flags & TRIM_TRANSPARENT) {
            GUI_DrawBackground(x, y, width, height);
        } else {
            LCD_FillRect(x, y, width, height, disp->bg_color);
        }
//        LCD_DrawFastHLine(x, y + height / 2, width, 0x0000); //Main axis
        LCD_DrawFastVLine(x + width / 2, y, height, disp->outline_color); //Center
        LCD_FillRect(x + val - TRIM_THICKNESS / 2,
                     y + TRIM_MARGIN,
                     TRIM_THICKNESS, height - TRIM_MARGIN * 2, color);
        break;
    }
    case TRIM_VERTICAL: {
        val = (TRIM_THICKNESS / 2) + (height - TRIM_THICKNESS) * (val - graph->min) / (graph->max - graph->min);
        if (Display.flags & TRIM_TRANSPARENT) {
            GUI_DrawBackground(x, y, width, height);
        } else {
            LCD_FillRect(x, y, width, height, disp->bg_color);
        }
//        LCD_DrawFastVLine(x + width / 2, y, height, 0xFFFF); //Main axis
        LCD_DrawFastHLine(x, y + height / 2, width, disp->outline_color); //Center
        LCD_FillRect(x + TRIM_MARGIN,
                     y + (height - val) - TRIM_THICKNESS / 2,
                     width - TRIM_MARGIN * 2, TRIM_THICKNESS, color);
        break;
    }
    }
}

