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

#include <stdlib.h>
#include "common.h"
#define ENABLE_GUIOBJECT
#include "gui.h"
#include "config/display.h"

#include "_bargraph.c"

guiObject_t *GUI_CreateBarGraph(guiBarGraph_t *graph, u16 x, u16 y, u16 width, u16 height,
                      s16 min, s16 max, u8 direction,
                      s16 (*Callback)(void *data), void *cb_data)
{
    struct guiHeader   *obj = (guiObject_t *)graph;
    struct guiBox      *box;

    box = &obj->box;

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
    struct guiBox *box = &obj->box;
    struct guiBarGraph *graph = (struct guiBarGraph *)obj;
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
    
    s32 val = graph->CallBack ? graph->CallBack(graph->cb_data) : 0;
    if (val < graph->min)
        val = graph->min;
    else if (val > graph->max)
        val = graph->max;
    u32 color = _bargraph_get_color(val, graph, disp);

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
    case TRIM_HORIZONTAL:
    case TRIM_INVHORIZONTAL:
        _bargraph_trim_horizontal(x, y, width, height, val, color, graph, disp, box);
        break;
    case TRIM_VERTICAL:
        _bargraph_trim_vertical(x, y, width, height, val, color, graph, disp, box);
        break;
    }
}

