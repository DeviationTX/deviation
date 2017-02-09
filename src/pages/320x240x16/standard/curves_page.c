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
#include "../../common/standard/_curves_page.c"

static const char *curvepos[] = {
  _tr_noop("L"), "2", "3", "4", _tr_noop("M"), "6", "7", "8", _tr_noop("H")
};

static const char *lockstr_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    int pos = (long)data;
    return (selectable_bitmaps[curve_mode * 4 + pit_mode] & (1 << (pos - 1))) ? _tr("Manual") : _tr("Auto");
}

static void press_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 point_num = (long)data;
    u8 *selectable_bitmap = &selectable_bitmaps[curve_mode * 4 + pit_mode];
    if (*selectable_bitmap >> (point_num-1) & 0x01) {
        GUI_TextSelectEnable(&gui->val[point_num], 0);
        *selectable_bitmap &= ~(1 << (point_num-1));
        auto_generate_cb(NULL, NULL);
    } else {
        GUI_TextSelectEnable(&gui->val[point_num], 1);
        *selectable_bitmap |= 1 << (point_num-1);
    }
}

static const char *holdsw_str_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    snprintf(tempstring, sizeof(tempstring), "%s ", _tr("Switch"));
    INPUT_SourceNameAbbrevSwitch(tempstring + strlen(tempstring), mapped_std_channels.switches[SWITCHFUNC_HOLD]);
    return tempstring;
}

static void update_textsel_state()
{
    GUI_SetHidden((guiObject_t *)&gui->holdsw, pit_mode == PITTHROMODE_HOLD ? 0 : 1);
    GUI_SetHidden((guiObject_t *)&gui->holdlbl, pit_mode == PITTHROMODE_HOLD ? 0 : 1);
    int state = pit_mode == PITTHROMODE_HOLD && pit_hold_state == 0;
    for (u8 i = 0; i < 9; i++) {
        GUI_SetHidden((guiObject_t *)&gui->vallbl[i], state);
        GUI_SetHidden((guiObject_t *)&gui->val[i], state);
        GUI_SetHidden((guiObject_t *)&gui->lock[i], state);
        if(! state && i > 0 && i < 8) {
            u8 selectable_bitmap = selectable_bitmaps[curve_mode * 4 + pit_mode];
            if (selectable_bitmap >> (i-1) & 0x01) {
                GUI_TextSelectEnable(&gui->val[i], 1);
            } else {
                GUI_TextSelectEnable(&gui->val[i], 0);
            }
        }
    }
    GUI_SetHidden((guiObject_t *)&gui->graph, state);
}

static void show_page(CurvesMode _curve_mode, int page)
{
    (void)page;
    enum {
        // LINE1      = (40 + ((LCD_HEIGHT - 240) / 2)),
        LINE1      = 36,
        LINE2      = (LINE1 + 20),
        // COL_LBL    = (92 + ((LCD_WIDTH - 320) / 2)),
        COL_LBL    = (LCD_WIDTH - 208),
        COL_TEXT   = (COL_LBL + 48),
        COL_SWITCH = (COL_LBL + 120),
        ////
        COL1 = (4 + ((LCD_WIDTH - 320) / 2)),
        COL2 = (20 + ((LCD_WIDTH - 320) / 2)),
        COL3 = (92 + ((LCD_WIDTH - 320) / 2)),
        ROWBASE = ((LCD_HEIGHT == 240 ? 60 : 67) + ((LCD_HEIGHT - 240) / 2)),
        ////
        GRAPHSIZE_320 = 150,
        GRAPHSIZE = (LCD_WIDTH == 320 ? GRAPHSIZE_320 : GRAPHSIZE_320 + 32),
    };
    curve_mode = _curve_mode;
    memset(mp, 0, sizeof(*mp));
    PAGE_ShowHeader(NULL);
    int count;
    int expected;
    if (curve_mode == CURVESMODE_PITCH) {
        PAGE_ShowHeader_SetLabel(STDMIX_TitleString, SET_TITLE_DATA(PAGEID_PITCURVES, SWITCHFUNC_FLYMODE));
        count = STDMIX_GetMixers(mp->mixer_ptr, mapped_std_channels.pitch, 4);
        get_hold_state();
    } else {
        PAGE_ShowHeader_SetLabel(STDMIX_TitleString, SET_TITLE_DATA(PAGEID_THROCURVES, SWITCHFUNC_FLYMODE));
        count = STDMIX_GetMixers(mp->mixer_ptr, mapped_std_channels.throttle, 4);
        pit_hold_state = 0;
    }
    expected = INPUT_NumSwitchPos(mapped_std_channels.switches[SWITCHFUNC_FLYMODE]) + pit_hold_state;
    if (count != expected) {
        GUI_CreateLabelBox(&gui->msg, 0, 120, 240, 16, &NARROW_FONT, NULL, NULL, "Invalid model ini!");// must be invalid model ini
        return;
    }

    set_cur_mixer();
    /* Row 1 */
    GUI_CreateLabelBox(&gui->modelbl, COL_LBL, LINE1, 0, 16, &DEFAULT_FONT, NULL, NULL, _tr("Mode"));
    GUI_CreateTextSelect(&gui->mode, COL_TEXT, LINE1-1, TEXTSELECT_128, NULL, set_mode_cb, (void *)(long)curve_mode);
    GUI_CreateLabelBox(&gui->holdlbl, COL_LBL, LINE2, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Enabled"));
    GUI_CreateTextSelect(&gui->hold, COL_TEXT, LINE2-1, TEXTSELECT_64, NULL, set_holdstate_cb, NULL);
    GUI_CreateLabelBox(&gui->holdsw, COL_SWITCH, LINE2, 0, 0, &DEFAULT_FONT, holdsw_str_cb, NULL, NULL);
    if (pit_mode != PITTHROMODE_HOLD)
        GUI_SetHidden((guiObject_t *)&gui->hold, 1);

    /* Row 2 */
    for(long i = 0; i < 9; i++) {
        const char *label = curvepos[i];
        if(label[0] > '9')
            label = _tr(label);
        GUI_CreateLabelBox(&gui->vallbl[i], COL1, ROWBASE+20*i+1, COL2-COL1, 16, &DEFAULT_FONT, NULL, NULL, label);
        GUI_CreateTextSelect(&gui->val[i], COL2, ROWBASE+20*i, TEXTSELECT_64, NULL, set_pointval_cb, (void *)i);
        if (i > 0 && i < 8)
            GUI_CreateButton(&gui->lock[i-1], COL3, ROWBASE+20*i, BUTTON_64x16, lockstr_cb, press_cb, (void *)i);
    }
    GUI_CreateXYGraph(&gui->graph, 160 + ((LCD_WIDTH - 320) / 2), 80 + ((LCD_HEIGHT - 240) / 2) - (GRAPHSIZE - GRAPHSIZE_320) / 2, GRAPHSIZE, GRAPHSIZE,
                  CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                  CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                  0, 0,
                  show_curve_cb, curpos_cb, NULL, NULL);
    update_textsel_state();
}

void PAGE_ThroCurvesInit(int page)
{
    show_page(CURVESMODE_THROTTLE, page);
}

void PAGE_PitCurvesInit(int page)
{
    show_page(CURVESMODE_PITCH, page);
}
#endif //HAS_STANDARD_GUI
