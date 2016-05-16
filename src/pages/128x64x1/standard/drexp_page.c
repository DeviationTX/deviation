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
#include "../pages.h"
#include "gui/gui.h"
#include "config/model.h"
#include "mixer.h"
#include "mixer_standard.h"
#include "standard.h"

#if HAS_STANDARD_GUI
#include "../../common/standard/_drexp_page.c"

guiObject_t *scroll_bar;

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
    u8 w1 = 30;
    u8 w2 = 36;

    GUI_CreateLabelBox(&gui->label[relrow], 0, y,
        0, LINE_HEIGHT, &DEFAULT_FONT, NULL, NULL, STDMIX_ModeName(absrow - PITTHROMODE_NORMAL));
    y += LINE_SPACE;
    GUI_CreateTextSelectPlate(&gui->value1[relrow], 0, y,
        w1, LINE_HEIGHT, &TINY_FONT, NULL, set_dr_cb, (void *)(long)(absrow - PITTHROMODE_NORMAL));
    GUI_CreateTextSelectPlate(&gui->value2[relrow], w1+1, y,
        w2, LINE_HEIGHT, &DEFAULT_FONT, NULL, set_exp_cb, (void *)(long)(absrow - PITTHROMODE_NORMAL));
    return 2;
}

void PAGE_DrExpInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    PAGE_RemoveAllObjects();
    PAGE_ShowHeader("");
    memset(mp, 0, sizeof(*mp));
    int count = get_mixers();
    int expected = INPUT_NumSwitchPos(mapped_std_channels.switches[SWITCHFUNC_DREXP_AIL+drexp_type]);
    if (count != expected) {
        GUI_CreateLabelBox(&gui->u.msg, 0, 10, 0, LINE_HEIGHT, &DEFAULT_FONT, NULL, NULL, "Invalid model ini!");// must be invalid model ini
        return;
    }
    GUI_CreateTextSelectPlate(&gui->u.type, 0, 0, 60, HEADER_WIDGET_HEIGHT,
                     &DEFAULT_FONT, NULL, set_type_cb, (void *)NULL);
    GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, 76, LCD_HEIGHT - HEADER_HEIGHT,
                     2 * LINE_SPACE, count, row_cb, NULL, NULL, NULL);

    GUI_CreateXYGraph(&gui->graph, 77, HEADER_HEIGHT, 50, 50,
            CHAN_MIN_VALUE, CHAN_MIN_VALUE, CHAN_MAX_VALUE, CHAN_MAX_VALUE,
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
