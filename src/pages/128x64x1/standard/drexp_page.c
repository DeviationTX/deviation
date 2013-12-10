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

static u8 _action_cb(u32 button, u8 flags, void *data);

//static u16 current_selected = 0;
guiObject_t *scroll_bar;
static const int RIGHT_VIEW_HEIGHT = 60;
static const int RIGHT_VIEW_ID  = 1;

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
static guiObject_t *getobj_cb(int relrow, int col, void *data)
{
    (void)data;
    col = (2 + col) % 2;
    return col ? (guiObject_t *)&gui->value2[relrow] : (guiObject_t *)&gui->value1[relrow];
}

static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;
    u8 w1 = 30;
    u8 w2 = 36;
    GUI_CreateLabelBox(&gui->label[relrow], 0, y,
        0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, STDMIX_ModeName(absrow - PITTHROMODE_NORMAL));
    y += ITEM_SPACE;
    GUI_CreateTextSelectPlate(&gui->value1[relrow], 0, y,
        w1, ITEM_HEIGHT, &TINY_FONT, NULL, set_dr_cb, (void *)(long)(absrow - PITTHROMODE_NORMAL));
    GUI_CreateTextSelectPlate(&gui->value2[relrow], w1+1, y,
        w2, ITEM_HEIGHT, &DEFAULT_FONT, NULL, set_exp_cb, (void *)(long)(absrow - PITTHROMODE_NORMAL));
    return 2;
}

void PAGE_DrExpInit(int page)
{
    (void)page;
    PAGE_SetActionCB(_action_cb);
    PAGE_SetModal(0);
    PAGE_RemoveAllObjects();
    memset(mp, 0, sizeof(*mp));
    int count = get_mixers();
    int expected = INPUT_NumSwitchPos(mapped_std_channels.switches[SWITCHFUNC_DREXP_AIL+drexp_type]);
    if (count != expected) {
        GUI_CreateLabelBox(&gui->u.msg, 0, 10, 0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, "Invalid model ini!");// must be invalid model ini
        return;
    }
    GUI_CreateTextSelectPlate(&gui->u.type, 0, 0,
        60, ITEM_HEIGHT, &DEFAULT_FONT, NULL, set_type_cb, (void *)NULL);
    GUI_CreateScrollable(&gui->scrollable, 0, ITEM_SPACE, 76, LCD_HEIGHT - ITEM_SPACE,
                     2 *ITEM_SPACE, count, row_cb, getobj_cb, NULL, NULL);
    //GUI_SetSelected(GUI_ShowScrollableRowOffset(&gui->scrollable, current_selected));

    u16 ymax = CHAN_MAX_VALUE/100 * MAX_SCALAR;
    s16 ymin = -ymax;
    GUI_CreateXYGraph(&gui->graph, 77, 2, 50, RIGHT_VIEW_HEIGHT,
            CHAN_MIN_VALUE, ymin, CHAN_MAX_VALUE, ymax,
            0, 0, show_curve_cb, curpos_cb, NULL, NULL);

    GUI_Select1stSelectableObj();
}

static void _refresh_page()
{
    PAGE_RemoveAllObjects();
    PAGE_DrExpInit(0);
}

static u8 _action_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    //u8 total_items = 2;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            PAGE_ChangeByID(PAGEID_MENU, PREVIOUS_ITEM);
        }
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
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
