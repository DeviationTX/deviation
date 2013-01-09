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

#include "common.h"
#define ENABLE_GUIOBJECT
#include "gui.h"
#include "config/display.h"

guiObject_t *GUI_CreateXYGraph(guiXYGraph_t *graph, u16 x, u16 y, u16 width, u16 height,
                      s16 min_x, s16 min_y, s16 max_x, s16 max_y,
                      u16 gridx, u16 gridy,
                      s16 (*Callback)(s16 xval, void *data),
                      u8 (*point_cb)(s16 *x, s16 *y, u8 pos, void *data),
                      u8 (*touch_cb)(s16 x, s16 y, void *data),
                      void *cb_data)
{
    struct guiObject  *obj   = (guiObject_t *)graph;
    struct guiBox    *box;

    box = &obj->box;

    box->x = x;
    box->y = y;
    box->width = width;
    box->height = height;

    obj->Type = XYGraph;
    OBJ_SET_TRANSPARENT(obj, 0);
    connect_object(obj);

    graph->min_x = min_x;
    graph->min_y = min_y;
    graph->max_x = max_x;
    graph->max_y = max_y;
    graph->grid_x = gridx;
    graph->grid_y = gridy;
    graph->CallBack = Callback;
    graph->point_cb = point_cb;
    graph->touch_cb = touch_cb;
    graph->cb_data = cb_data;

    return obj;
}

void GUI_DrawXYGraph(struct guiObject *obj)
{
    struct guiBox *box = &obj->box;
    struct guiXYGraph *graph = (struct guiXYGraph *)obj;
    u32 x, y;

    #define VAL_TO_X(xval) \
        (u32)(box->x + (((s32)(xval)) - graph->min_x) * box->width / (1 + graph->max_x - graph->min_x))
    #define VAL_TO_Y(yval) \
        (u32)(box->y + box->height - (((s32)(yval)) - graph->min_y) * box->height / (1 + graph->max_y - graph->min_y))
    LCD_FillRect(box->x, box->y, box->width, box->height, 0x0000);
    if (graph->grid_x) {
        int xval;
        for (xval = graph->min_x + graph->grid_x; xval < graph->max_x; xval += graph->grid_x) {
            if (! xval)
                continue;
            x = VAL_TO_X(xval);
            //LCD_DrawDashedVLine(x, box->y, box->height, 5, RGB888_to_RGB565(0x30, 0x30, 0x30));
            LCD_DrawFastVLine(x, box->y, box->height, RGB888_to_RGB565(0x30, 0x30, 0x30));
        }
    }
    if (graph->grid_y) {
        int yval;
        for (yval = graph->min_y + graph->grid_y; yval < graph->max_y; yval += graph->grid_y) {
            if (! yval)
                continue;
            y = VAL_TO_Y(yval);
            //LCD_DrawDashedHLine(box->x, y, box->width, 5, RGB888_to_RGB565(0x30, 0x30, 0x30));
            LCD_DrawFastHLine(box->x, y, box->width, RGB888_to_RGB565(0x30, 0x30, 0x30));
        }
    }
    if (graph->min_x < 0 && graph->max_x > 0) {
        int x = box->x + box->width * (0 - graph->min_x) / (graph->max_x - graph->min_x);
        LCD_DrawFastVLine(x, box->y, box->height, 0xFFFF);
    }
    if (graph->min_y < 0 && graph->max_y > 0) {
        y = box->y + box->height - box->height * (0 - graph->min_y) / (graph->max_y - graph->min_y);
        LCD_DrawFastHLine(box->x, y, box->width, 0xFFFF);
    }
    u16 lastx = box->x;
    u16 lasty = box->y + box->height -1;
    LCD_DrawStart(box->x, box->y, box->x + box->width - 1, box->y + box->height - 1, DRAW_NWSE);
    for (x = 0; x < box->width; x++) {
        s32 xval, yval;
        xval = graph->min_x + x * (1 + graph->max_x - graph->min_x) / box->width;
        yval = graph->CallBack(xval, graph->cb_data);
        y = (yval - graph->min_y) * box->height / (1 + graph->max_y - graph->min_y);
        //printf("(%d, %d - %d, %d) -> (%d, %d)\n",
        //       (int)lastx, (int)lasty, (int)x, (int)y, (int)xval, (int)yval);
        if (x != 0) {
            LCD_DrawLine(lastx, lasty, x + box->x, box->y + box->height - y - 1, 0xFFE0); //Yellow
        }
        lastx = x + box->x;
        lasty = box->y + box->height - y - 1;
    }
    LCD_DrawStop();
    if (graph->point_cb) {
        u8 pos = 0;
        s16 xval, yval;
        while (graph->point_cb(&xval, &yval, pos++, graph->cb_data)) {
            s16 x1 = VAL_TO_X(xval);
            s16 y1 = VAL_TO_Y(yval);
            s16 x2 = x1 + 2;
            s16 y2 = y1 + 2;
            //bounds check
            x1 = ( x1 < 2 + box->x) ? box->x : x1 - 2;
            y1 = ( y1 < 2 + box->y) ? box->y : y1 - 2;
            if ( x2 >= box->x + box->width)
                x2 = box->x + box->width - 1;
            if ( y2 >= box->y + box->height)
                y2 = box->y + box->height - 1;
            LCD_FillRect(x1, y1, x2 - x1 + 1, y2 - y1 + 1, RGB888_to_RGB565(0x00, 0xFF, 0xFF));
        }
    }
}

u8 GUI_TouchXYGraph(struct guiObject *obj, struct touch *coords, u8 long_press)
{
    struct guiXYGraph *graph = (struct guiXYGraph *)obj;
    (void)long_press;

    if (graph->touch_cb) {
        s32 x, y;
        x = (s32)(coords->x - obj->box.x) * (1 + graph->max_x - graph->min_x) / obj->box.width + graph->min_x;
        y = (s32)(obj->box.height -1 - (coords->y - obj->box.y))
            * (1 + graph->max_y - graph->min_y) / obj->box.height + graph->min_y;
        if(graph->touch_cb(x, y, graph->cb_data)) {
            OBJ_SET_DIRTY(obj, 1);
            return 1;
        }
    }
    return 0;
}
