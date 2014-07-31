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

static struct mixer_page   * const mp  = &pagemem.u.mixer_page;
static struct stdcurve_obj * const gui = &gui_objs.u.stdcurve;

static PitThroMode pit_mode = PITTHROMODE_NORMAL;
static CurvesMode curve_mode;
static u8 pit_hold_state = 0;
static u8 selectable_bitmaps[7] = {0, 0, 0 ,0 ,0, 0, 0};

static void update_textsel_state();

void set_cur_mixer()
{
    if(pit_mode == PITTHROMODE_HOLD) {
        mp->cur_mixer = mp->mixer_ptr[INPUT_NumSwitchPos(mp->mixer_ptr[1]->sw)];
    } else {
        mp->cur_mixer = mp->mixer_ptr[pit_mode];
    }
}

static const char *set_mode_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    curve_mode = (long)data;
    int max_sw = INPUT_NumSwitchPos(mapped_std_channels.switches[SWITCHFUNC_FLYMODE])-1;
    u8 max = curve_mode == CURVESMODE_PITCH? PITTHROMODE_HOLD : PITTHROMODE_NORMAL + max_sw;
    u8 changed;
    pit_mode = GUI_TextSelectHelper(pit_mode, PITTHROMODE_NORMAL, max, dir, 1, 1, &changed);
    u8 i;
    if (pit_mode > (u32)(PITTHROMODE_NORMAL + max_sw) && pit_mode != PITTHROMODE_HOLD) {
        pit_mode = (dir > 0) ? PITTHROMODE_HOLD : PITTHROMODE_NORMAL + max_sw;
        changed = 1;
    }
    if (changed) {
        set_cur_mixer();
        if (pit_mode == PITTHROMODE_HOLD)
            GUI_SetHidden((guiObject_t *)&gui->hold, 0);
        else
            GUI_SetHidden((guiObject_t *)&gui->hold,  1);
        update_textsel_state();
        GUI_Redraw(&gui->graph);
        for ( i = 0; i < 9; i++)
            GUI_Redraw(&gui->val[i]);
    }
    tempstring_cpy((const char *)STDMIX_ModeName(pit_mode));
    return tempstring;
}

static void get_hold_state()
{
    pit_hold_state = 0;
    if(mp->mixer_ptr[3]) {
        if (MIXER_SRC(mp->mixer_ptr[3]->sw) == mapped_std_channels.switches[SWITCHFUNC_HOLD])
            pit_hold_state = 1;
    } else if(mp->mixer_ptr[2] && MIXER_SRC(mp->mixer_ptr[2]->sw) == mapped_std_channels.switches[SWITCHFUNC_HOLD]) {
        pit_hold_state = 1;
    }
}

static void set_hold_state(u8 state) {
    struct Mixer mix[4];
    int i;
    for(i = 0; i < 4 && mp->mixer_ptr[i]; i++)
        mix[i] = *mp->mixer_ptr[i];
    int num_pos = INPUT_NumSwitchPos(mix[1].sw);
    if (state != 0) {
        if (i == num_pos) {
            mix[num_pos] = mix[0];
        }
        mix[num_pos].sw = 0x80 | INPUT_GetFirstSwitch(mapped_std_channels.switches[SWITCHFUNC_HOLD]);
        num_pos++;
    }
    MIXER_SetMixers(mix, num_pos);
    STDMIX_GetMixers(mp->mixer_ptr, mix[1].dest, 4);
    set_cur_mixer();
    GUI_Redraw(&gui->graph);
    for (u8 i = 0; i < 9; i++)
        GUI_Redraw(&gui->val[i]);
}

static const char *set_holdstate_cb(guiObject_t *obj, int dir, void *data)
{
    (void)dir;
    (void)data;
    (void)obj;
    u8 changed;
    pit_hold_state = GUI_TextSelectHelper(pit_hold_state, 0, 1, dir, 1, 1, &changed);
    if (changed) {
        set_hold_state(pit_hold_state);
        update_textsel_state();
    }
    if (pit_mode != PITTHROMODE_HOLD)
        tempstring_cpy("");
    else if (pit_hold_state)
        tempstring_cpy(_tr("On"));
    else
        tempstring_cpy(_tr("Off"));
    return tempstring;
}


static void auto_generate_cb(guiObject_t *obj, const void *data)
{
    (void)data;
    (void)obj;
    if (! mp->cur_mixer)
        return;
    struct Curve *curve = &(mp->cur_mixer->curve);
    s16 y_diff = 0;
    s16 x_start = 0;
    s16 x_end = 8;
    u8 selectable_bitmap = selectable_bitmaps[curve_mode * 4 + pit_mode];
    u8 j;
    for (u8 i = 1; i < 9; i++) {
        if (i == 8 || selectable_bitmap >> (i-1) & 0x01) {
            x_end = i;
            if (x_end - x_start > 1) {
                y_diff = curve->points[x_end] - curve->points[x_start];
                u16 x_diff =  x_end - x_start;
                for (j = x_start + 1; j < x_end; j++) {
                    curve->points[j] = y_diff * (j - x_start)/x_diff + curve->points[x_start];
                    GUI_Redraw(&gui->val[j]);
                }
            }
            x_start = x_end; // no need to calculate
        }
    }
    GUI_Redraw(&gui->graph);
}

static const char *set_pointval_cb(guiObject_t *obj, int dir, void *data)
{
    if (! mp->cur_mixer)
        return "";
    u8 point_num = (long)data;
    struct Curve *curve = &(mp->cur_mixer->curve);
    if (GUI_IsTextSelectEnabled(obj) == 1) {
        u8 changed = 1;
        curve->points[point_num] = GUI_TextSelectHelper(curve->points[point_num], -100, 100, dir, 1, LONG_PRESS_STEP, &changed);
        if (changed)
            auto_generate_cb(NULL, NULL);
    }
    sprintf(tempstring, "%d", curve->points[point_num]);
    return tempstring;
}

static u8 curpos_cb(s16 *x, s16 *y, u8 pos, void *data)
{
    (void)data;
    if (! mp->cur_mixer)
        return 0;
    if (pos != 0)
        return 0;
    *x = mp->raw[MIXER_SRC(mp->cur_mixer->src)];
    if (*x > CHAN_MAX_VALUE)
        *x = CHAN_MAX_VALUE;
    else if (*x  < CHAN_MIN_VALUE)
        *x = CHAN_MIN_VALUE;
    *y = STDMIX_EvalMixerCb(*x, mp->cur_mixer, CHAN_MAX_VALUE, CHAN_MIN_VALUE);
    return 1;
}

static s16 show_curve_cb(s16 xval, void *data)
{
    (void)data;
    if (! mp->cur_mixer)
        return 0;
    s16 yval = CURVE_Evaluate(xval, &(mp->cur_mixer->curve));
    return yval;
}

static u8 event_interval = 0;
void PAGE_CurvesEvent()
{
    //if (event_interval++ <EVENT_REFRESH_INTERVAL) // reduce the refresh frequency
    //    return;
    event_interval = 0;
    if (OBJ_IS_USED(&gui->graph)) {
        if(MIXER_GetCachedInputs(mp->raw, CHAN_MAX_VALUE / 100)) { // +/-1%
            GUI_Redraw(&gui->graph);
/*
            if (mp->graphs[1])
                GUI_Redraw(mp->graphs[1]);
            if (mp->graphs[2])
                GUI_Redraw(mp->graphs[2]);
*/
        }
    }
}

