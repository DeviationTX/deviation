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

static struct main_page * const mp = &pagemem.u.main_page;
const char *show_box_cb(guiObject_t *obj, const void *data);
const char *voltage_cb(guiObject_t *obj, const void *data);
static s16 trim_cb(void * data);
static s16 bar_cb(void * data);
void press_icon_cb(guiObject_t *obj, s8 press_type, const void *data);
void press_icon2_cb(guiObject_t *obj, const void *data);
void press_box_cb(guiObject_t *obj, s8 press_type, const void *data);
static u8 _action_cb(u32 button, u8 flags, void *data);
static s32 get_boxval(u8 idx);
static void _check_voltage();

struct LabelDesc *get_box_font(u8 idx, u8 neg)
{
    if(neg) {
        return idx & 0x02 ? &SMALLBOXNEG_FONT : &BIGBOXNEG_FONT;
    } else {
        return idx & 0x02 ? &SMALLBOX_FONT : &BIGBOX_FONT;
    }
}

s32 get_boxval(u8 idx)
{
    if (idx <= NUM_TIMERS)
        return TIMER_GetValue(idx - 1);
    if(idx - NUM_TIMERS <= NUM_TELEM)
        return TELEMETRY_GetValue(idx - NUM_TIMERS);
    return RANGE_TO_PCT(MIXER_GetChannel(idx - (NUM_TIMERS + NUM_TELEM + 1), APPLY_SAFETY));
}

const char *show_box_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 idx = (long)data;
    if (idx <= NUM_TIMERS) {
        TIMER_SetString(mp->tmpstr, TIMER_GetValue(idx - 1));
    } else if(idx - NUM_TIMERS <= NUM_TELEM) {
        TELEMETRY_GetValueStr(mp->tmpstr, idx - NUM_TIMERS);
    } else {
        sprintf(mp->tmpstr, "%3d%%", RANGE_TO_PCT(MIXER_GetChannel(idx - (NUM_TIMERS + NUM_TELEM + 1), APPLY_SAFETY)));
    }
    return mp->tmpstr;
}

const char *voltage_cb(guiObject_t *obj, const void *data) {
    (void)obj;
    (void)data;
    sprintf(mp->tmpstr, "%2d.%02dV", mp->battery / 1000, (mp->battery % 1000) / 10);
    return mp->tmpstr;
}

s16 trim_cb(void * data)
{
    s8 *trim = (s8 *)data;
    return *trim;
}

s16 bar_cb(void * data)
{
    u8 idx = (long)data;
    return MIXER_GetChannel(idx-1, APPLY_SAFETY);
}

void PAGE_MainEvent()
{
    int i;
    if (PAGE_GetModal()) {
        if(pagemem.modal_page == 2) {
            PAGE_TelemtestEvent();
        }
        return;
    }
    for(i = 0; i < 6; i++) {
        if (! mp->trimObj[i])
            continue;
        if (mp->trims[i] != Model.trims[i].value) {
            mp->trims[i] = Model.trims[i].value;
            GUI_Redraw(mp->trimObj[i]);
        }
    }
    for(i = 0; i < 8; i++) {
        if (! mp->boxObj[i])
            continue;
        s32 val = get_boxval(Model.pagecfg.box[i]);
        if (Model.pagecfg.box[i] <= NUM_TIMERS) {
            if ((val >= 0 && mp->boxval[i] < 0) || (val < 0 && mp->boxval[i] >= 0)) {
                //Timer
                GUI_SetLabelDesc(mp->boxObj[i], get_box_font(i, val < 0));
                mp->boxval[i] = val;
                GUI_Redraw(mp->boxObj[i]);
            } else if (mp->boxval[i] / 1000 != val /1000) {
                mp->boxval[i] = val;
                GUI_Redraw(mp->boxObj[i]);
            }
        } else if (Model.pagecfg.box[i] - NUM_TIMERS <= NUM_TELEM) {
            u32 time = CLOCK_getms();
            if (Telemetry.time[0] && time - Telemetry.time[0] > TELEM_ERROR_TIME) {
                Telemetry.time[0] = 0;
                GUI_SetLabelDesc(mp->boxObj[i], get_box_font(i, 1));
            } else if(Telemetry.time[0] && mp->boxval[i] != val) {
                GUI_SetLabelDesc(mp->boxObj[i], get_box_font(i, 0));
                mp->boxval[i] = val;
                GUI_Redraw(mp->boxObj[i]);
            }
        } else if (mp->boxval[i] != val) {
            mp->boxval[i] = val;
            GUI_Redraw(mp->boxObj[i]);
        }
    }
    s16 *raw = MIXER_GetInputs();
    for(i = 0; i < 8; i++) {
        if (! mp->barObj[i])
            continue;
        s16 chan = MIXER_GetChannel(Model.pagecfg.bar[i]-1, APPLY_SAFETY);
        if (mp->barval[i] != chan) {
            mp->barval[i] = chan;
            GUI_Redraw(mp->barObj[i]);
        }
    }
    for(i = 0; i < 4; i++) {
        if (! mp->toggleObj[i])
            continue;
        u8 src = MIXER_SRC(Model.pagecfg.toggle[i]);
        s16 val = raw[src];
        GUI_SetHidden(mp->toggleObj[i], MIXER_SRC_IS_INV(Model.pagecfg.toggle[i]) ? val > 0 : val < 0);
    }
    _check_voltage();
}
