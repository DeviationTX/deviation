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

#ifndef OVERRIDE_PLACEMENT
#include "common.h"
#include "../pages.h"
#include "gui/gui.h"
#include "config/model.h"
#include "mixer.h"
#include "mixer_standard.h"
#include "standard.h"

enum {
    LABEL_X      = 0,
    LABEL2_WIDTH = 30,
    LABEL3_X     = 31,
    LABEL3_WIDTH = 36,
    MESSAGE_Y    = 10,
    HEADER_X     = 0,
    HEADER_W     = 60,
    SCROLL_Y     = 0,
    SCROLL_W     = 76,
    GRAPH_X      = 77,
    #define GRAPH_Y HEADER_HEIGHT
    GRAPH_W      = 50,
    GRAPH_H      = 50,
};
#endif //OVERRIDE_PLACEMENT

#if HAS_STANDARD_GUI
#include "../../common/standard/_drexp_page.c"

enum {
    ITEM_NORMAL,
    ITEM_IDLE1,
    ITEM_IDLE2,
    ITEM_LAST,
};

void update_graph(int graph)
{
    (void)graph;
    GUI_Redraw(&gui->graph);
}

static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;

    GUI_CreateLabelBox(&gui->label[relrow], LABEL_X, y,
        LABEL3_X + LABEL3_WIDTH, LINE_HEIGHT, &LABEL_FONT, NULL, NULL, STDMIX_ModeName(absrow - PITTHROMODE_NORMAL));
    y += LINE_SPACE;
    GUI_CreateTextSelectPlate(&gui->value1[relrow], LABEL_X, y,
        LABEL2_WIDTH, LINE_HEIGHT, &TINY_FONT, NULL, set_dr_cb, (void *)(long)(absrow - PITTHROMODE_NORMAL));
    GUI_CreateTextSelectPlate(&gui->value2[relrow], LABEL_X + LABEL3_X, y,
        LABEL3_WIDTH, LINE_HEIGHT, &TINY_FONT, NULL, set_exp_cb, (void *)(long)(absrow - PITTHROMODE_NORMAL));
    
    return 2;
}

void PAGE_DrExpInit(int page)
{
    (void)page;
    // draw a underline only:
    GUI_CreateRect(&gui->rect, 0, HEADER_WIDGET_HEIGHT, LCD_WIDTH, 1, &DEFAULT_FONT);
    memset(mp, 0, sizeof(*mp));
    int count = get_mixers();
    int expected = INPUT_NumSwitchPos(mapped_std_channels.switches[SWITCHFUNC_DREXP_AIL + drexp_type]);
    if (count != expected) {
        GUI_CreateLabelBox(&gui->u.msg, 0, MESSAGE_Y, 0, LINE_HEIGHT, &DEFAULT_FONT, NULL, NULL, "Invalid model ini!"); // must be invalid model ini
        return;
    }
    GUI_CreateTextSelectPlate(&gui->u.type, HEADER_X, 0, HEADER_W, HEADER_WIDGET_HEIGHT,
                     &TEXTSEL_FONT, NULL, set_type_cb, (void *)NULL);
    GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT + SCROLL_Y, SCROLL_W, LCD_HEIGHT - HEADER_HEIGHT - SCROLL_Y,
                     2 * LINE_SPACE, count, row_cb, NULL, NULL, NULL);

    GUI_CreateXYGraph(&gui->graph, GRAPH_X, GRAPH_Y, GRAPH_W, GRAPH_H,
                      CHAN_MIN_VALUE, CHAN_MIN_VALUE * 1251 / 1000,
                      CHAN_MAX_VALUE, CHAN_MAX_VALUE * 1251 / 1000,
                      0, 0, show_curve_cb, curpos_cb, NULL, NULL);

    GUI_Select1stSelectableObj();
}

static void _refresh_page()
{
    PAGE_RemoveAllObjects();
    PAGE_DrExpInit(0);
}

void PAGE_DrExpCurvesEvent()
{
    if (OBJ_IS_USED(&gui->graph)) {
        if(MIXER_GetCachedInputs(mp->raw, CHAN_MAX_VALUE / 100)) { // +/-1%
            GUI_Redraw(&gui->graph);
        }
    }
}

#endif //HAS_STANDARD_GUI
