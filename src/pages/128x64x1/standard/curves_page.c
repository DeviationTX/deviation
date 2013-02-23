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
#include "../../common/standard/_curves_page.c"

static u8 _action_cb(u32 button, u8 flags, void *data);
static void show_page(CurvesMode curves_mode, int page);

void PAGE_ThroCurvesInit(int page)
{
    show_page(CURVESMODE_THROTTLE, page);
}

void PAGE_PitCurvesInit(int page)
{
    show_page(CURVESMODE_PITCH, page);
}

static void update_textsel_state()
{
    int state = pit_mode == PITTHROMODE_HOLD && pit_hold_state == 0;
    for (u8 i = 0; i < 9; i++) {
        GUI_SetHidden((guiObject_t *)&gui->vallbl[i], state);
        GUI_SetHidden((guiObject_t *)&gui->val[i], state);
        if(! state && i > 0 && i < 8) {
            u8 selectable_bitmap = selectable_bitmaps[curve_mode * 4 + pit_mode];
            GUI_TextSelectEnablePress(&gui->val[i], 1);
            if (selectable_bitmap >> (i-1) & 0x01) {
                GUI_TextSelectEnable(&gui->val[i], 1);
            } else {
                GUI_TextSelectEnable(&gui->val[i], 0);
            }
        }
    }
    GUI_SetHidden((guiObject_t *)&gui->graph, state);
}

static void press_cb(guiObject_t *obj, void *data)
{
    u8 point_num = (long)data;
    u8 *selectable_bitmap = &selectable_bitmaps[curve_mode * 4 + pit_mode];
    if (*selectable_bitmap >> (point_num-1) & 0x01) {
        GUI_TextSelectEnable((guiTextSelect_t *)obj, 0);
        *selectable_bitmap &= ~(1 << (point_num-1));
        auto_generate_cb(NULL, NULL);
    } else {
        GUI_TextSelectEnable((guiTextSelect_t *)obj, 1);
        *selectable_bitmap |= 1 << (point_num-1);
    }
}


static void show_page(CurvesMode _curve_mode, int page)
{
    (void)page;
    PAGE_SetActionCB(_action_cb);
    PAGE_SetModal(0);
    PAGE_RemoveAllObjects();
    memset(mp, 0, sizeof(*mp));
    curve_mode = _curve_mode;
    int count;
    int expected;
    if (curve_mode == CURVESMODE_PITCH) {
        count = STDMIX_GetMixers(mp->mixer_ptr, mapped_std_channels.pitch, 4);
        get_hold_state();
    } else {
        count = STDMIX_GetMixers(mp->mixer_ptr, mapped_std_channels.throttle, 4);
        pit_hold_state = 0;
    }
    expected = INPUT_NumSwitchPos(mapped_std_channels.switches[SWITCHFUNC_FLYMODE]) + pit_hold_state;
    if (count != expected) {
        GUI_CreateLabelBox(&gui->msg, 0, 10, 0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, "Invalid model ini!");// must be invalid model ini
        return;
    }

    set_cur_mixer();
    PAGE_ShowHeader(_tr("Mode"));
    GUI_CreateTextSelectPlate(&gui->mode, 35, 0, 55, ITEM_HEIGHT, &DEFAULT_FONT, NULL, set_mode_cb, (void *)(long)curve_mode);
    GUI_CreateTextSelectPlate(&gui->hold, 92, 0, 36, ITEM_HEIGHT, &DEFAULT_FONT, NULL, set_holdstate_cb, NULL);
    if (pit_mode != PITTHROMODE_HOLD)
        GUI_SetHidden((guiObject_t *)&gui->hold, 1);

    STANDARD_DrawCurvePoints(gui->vallbl, gui->val,
        selectable_bitmaps[curve_mode * 4 + pit_mode], press_cb, set_pointval_cb);

    GUI_CreateXYGraph(&gui->graph, 77, ITEM_SPACE, 50, 50,
                      CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                      CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                      0, 0, //CHAN_MAX_VALUE / 4, CHAN_MAX_VALUE / 4,
                      show_curve_cb, curpos_cb, NULL, NULL);

    update_textsel_state();
    GUI_Select1stSelectableObj();
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
