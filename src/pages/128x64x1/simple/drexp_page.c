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
#include "mixer_simple.h"
#include "simple.h"
#include "../../common/simple/_drexp_page.c"

static u8 _action_cb(u32 button, u8 flags, void *data);
static void navigate_items(s8 direction);

static s8 current_selected = 0;
guiObject_t *scroll_bar;
static u8 current_xygraph;
#define RIGHT_VIEW_HEIGHT 60
#define RIGHT_VIEW_ID 1

void PAGE_DrExpInit(int page)
{
    if (page < 0 && current_selected > 0) // enter this page from childen page , so we need to get its previous mp->current_selected item
        page = current_selected;
    PAGE_SetActionCB(_action_cb);
    PAGE_SetModal(0);
    PAGE_RemoveAllObjects();
    current_selected = 0;
    memset(mp, 0, sizeof(*mp));
    get_mixers();
    if (!mp->mixer_ptr[0] || !mp->mixer_ptr[1] || !mp->mixer_ptr[2]) {
        GUI_CreateLabelBox(0, 10, 0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, "Invalid model ini!");// must be invalid model ini
        return;
    }

    // Create a logical view
    u8 view_origin_absoluteX = 0;
    u8 view_origin_absoluteY = 0;
    u8 graph_pos = 77;
    GUI_SetupLogicalView(VIEW_ID, 0, 0, graph_pos - ARROW_WIDTH -3, LCD_HEIGHT - view_origin_absoluteY ,
        view_origin_absoluteX, view_origin_absoluteY);

    mp->max_scroll = 0;
    u8 w1 = 30;
    u8 w2 = 36;
    u8 y = 0;
    mp->itemObj[0] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, 0) ,  GUI_MapToLogicalView(VIEW_ID, y),
            60, ITEM_HEIGHT, &DEFAULT_FONT, NULL, set_type_cb, (void *)NULL);
    mp->max_scroll ++;

    y += ITEM_SPACE;
    GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0) ,  GUI_MapToLogicalView(VIEW_ID, y),
            0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, SIMPLEMIX_ModeName(PITTHROMODE_NORMAL));
    y += ITEM_SPACE;
    GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, 0) ,  GUI_MapToLogicalView(VIEW_ID, y),
            w1, ITEM_HEIGHT, &TINY_FONT, NULL, set_dr_cb, (void *)(long)PITTHROMODE_NORMAL);
    GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, w1+1) ,  GUI_MapToLogicalView(VIEW_ID, y),
            w2, ITEM_HEIGHT, &TINY_FONT, NULL, set_exp_cb, (void *)(long)PITTHROMODE_NORMAL);
    mp->max_scroll += 2;

    y += ITEM_SPACE;
    GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0) ,  GUI_MapToLogicalView(VIEW_ID, y),
            0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, SIMPLEMIX_ModeName(PITTHROMODE_IDLE1));
    y += ITEM_SPACE;
    GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, 0) ,  GUI_MapToLogicalView(VIEW_ID, y),
            w1, ITEM_HEIGHT, &TINY_FONT, NULL, set_dr_cb, (void *)(long)PITTHROMODE_IDLE1);
    GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, w1+1) ,  GUI_MapToLogicalView(VIEW_ID, y),
            w2, ITEM_HEIGHT, &TINY_FONT, NULL, set_exp_cb, (void *)(long)PITTHROMODE_IDLE1);
    mp->max_scroll += 2;

    y += ITEM_SPACE;
    GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0) ,  GUI_MapToLogicalView(VIEW_ID, y),
            0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, SIMPLEMIX_ModeName(PITTHROMODE_IDLE2));
    y += ITEM_SPACE;
    GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, 0) ,  GUI_MapToLogicalView(VIEW_ID, y),
            w1, ITEM_HEIGHT, &TINY_FONT, NULL, set_dr_cb, (void *)(long)PITTHROMODE_IDLE2);
    GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, w1+1) ,  GUI_MapToLogicalView(VIEW_ID, y),
            w2, ITEM_HEIGHT, &TINY_FONT, NULL, set_exp_cb, (void *)(long)PITTHROMODE_IDLE2);
    mp->max_scroll += 2;

    GUI_SetupLogicalView(RIGHT_VIEW_ID, 0, 0, 50, RIGHT_VIEW_HEIGHT, graph_pos, 2);
    y = 0;
    u16 ymax = CHAN_MAX_VALUE/100 * MAX_SCALAR;
    s16 ymin = -ymax;
    for (u8 i = 0; i < 3; i++) {
        mp->graphs[i] = GUI_CreateXYGraph(GUI_MapToLogicalView(RIGHT_VIEW_ID, 0) ,
            GUI_MapToLogicalView(RIGHT_VIEW_ID, y), 50, RIGHT_VIEW_HEIGHT,
            CHAN_MIN_VALUE, ymin, CHAN_MAX_VALUE, ymax,
            0, 0, show_curve_cb, curpos_cb, NULL, (void *)(long)(PITTHROMODE_NORMAL + i));
        y += RIGHT_VIEW_HEIGHT;
    }

    current_xygraph = 0;
    GUI_Select1stSelectableObj();
    mp->scroll_bar = GUI_CreateScrollbar(graph_pos - ARROW_WIDTH -2, 0, LCD_HEIGHT, mp->max_scroll, NULL, NULL, NULL);
    // set the focus item back to previous selection in this page
    if (current_selected > 0) {
        u8 temp = current_selected;
        current_selected = 0;
        navigate_items(temp);
    }
}

static void _refresh_page()
{
    GUI_SetRelativeOrigin(VIEW_ID, 0, 0);
    GUI_SetRelativeOrigin(RIGHT_VIEW_ID, 0, 0);
}

static void navigate_items(s8 direction)
{
    guiObject_t *obj;
    for (u8 i = 0; i < (direction >0 ?direction:-direction); i++) {
        obj = GUI_GetSelected();
    if (direction > 0) {
        GUI_SetSelected((guiObject_t *)GUI_GetNextSelectable(obj));
    } else {
        GUI_SetSelected((guiObject_t *)GUI_GetPrevSelectable(obj));
    }
    }
    obj = GUI_GetSelected();
    current_selected += direction;
    current_selected %= mp->max_scroll;
    if (current_selected < 0)
        current_selected = mp->max_scroll - 1;
    if (!GUI_IsObjectInsideCurrentView(VIEW_ID, obj)) {
        // selected item is out of the view, scroll the view
        if (obj == mp->itemObj[0])
            GUI_SetRelativeOrigin(VIEW_ID, 0, 0);
        else
            GUI_ScrollLogicalViewToObject(VIEW_ID, obj, direction);
    }
    if (current_selected >= 0 && current_selected < 3) {
        if (current_xygraph != 0) {
            current_xygraph = 0;
            GUI_SetRelativeOrigin(RIGHT_VIEW_ID, 0, 0);
        }
    } else if (current_selected >= 5) {
        if (current_xygraph != 2) {
            current_xygraph = 2;
            GUI_SetRelativeOrigin(RIGHT_VIEW_ID, 0, RIGHT_VIEW_HEIGHT + RIGHT_VIEW_HEIGHT);
        }
    }  else if (current_xygraph != 1) {
        GUI_SetRelativeOrigin(RIGHT_VIEW_ID, 0, RIGHT_VIEW_HEIGHT);
        current_xygraph = 1;
    }
    GUI_SetScrollbar(mp->scroll_bar, current_selected);
}

static u8 _action_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    //u8 total_items = 2;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            PAGE_ChangeByID(PAGEID_MENU, PREVIOUS_ITEM);
        }  else if (CHAN_ButtonIsPressed(button, BUT_UP)) {
            navigate_items(-1);
        }  else if (CHAN_ButtonIsPressed(button, BUT_DOWN)) {
            navigate_items(1);
        }
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}
