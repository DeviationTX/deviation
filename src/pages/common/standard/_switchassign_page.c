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

static struct mixer_page * const mp = &pagemem.u.mixer_page;
#define gui (&gui_objs.u.stdswitch)

static u8 switch_idx[SWITCHFUNC_LAST];

static u8 get_switch_idx(FunctionSwitch switch_type) {
    return mapped_simple_channels.switches[switch_type] - INPUT_SwitchPos(mapped_simple_channels.switches[switch_type]);
}

static void refresh_switches()
{
    struct Mixer *mix = MIXER_GetAllMixers();

    if (Model.limits[mapped_simple_channels.throttle].safetysw)
        mapped_simple_channels.switches[SWITCHFUNC_HOLD] =  MIXER_SRC(Model.limits[mapped_simple_channels.throttle].safetysw);
    u8 found_gyro_switch = 0;
    u8 found_flymode_switch = 0;
    u8 found_drexp_rud_switch = 0;
    u8 found_drexp_ail_switch = 0;
    u8 found_drexp_ele_switch = 0;
    for (u8 idx = 0; idx < NUM_MIXERS; idx++) {
        if (!MIXER_SRC(mix[idx].src) || mix[idx].mux != MUX_REPLACE)  // all none replace mux will be considered as program mix in the Standard mode
            continue;
        if (!found_gyro_switch && mix[idx].sw != 0 && (mix[idx].dest == mapped_simple_channels.gear || mix[idx].dest == mapped_simple_channels.aux2)) {
            found_gyro_switch = 1;
            mapped_simple_channels.switches[SWITCHFUNC_GYROSENSE] = mix[idx].sw;
        } else if (!found_drexp_rud_switch && mix[idx].dest == mapped_simple_channels.rudd && mix[idx].sw != 0) {
            found_drexp_rud_switch = 1;
            mapped_simple_channels.switches[SWITCHFUNC_DREXP_RUD] = mix[idx].sw;
        } else if (!found_drexp_ail_switch && mix[idx].dest == mapped_simple_channels.aile && mix[idx].sw != 0) {
            found_drexp_ail_switch = 1;
            mapped_simple_channels.switches[SWITCHFUNC_DREXP_AIL] = mix[idx].sw;
        } else if (!found_drexp_ele_switch && mix[idx].dest == mapped_simple_channels.elev && mix[idx].sw != 0) {
            found_drexp_ele_switch = 1;
            mapped_simple_channels.switches[SWITCHFUNC_DREXP_ELE] = mix[idx].sw;
        } else if (!found_flymode_switch && (mix[idx].dest == NUM_OUT_CHANNELS + 2) && mix[idx].sw != 0) { //virt3
            found_flymode_switch = 1;
            mapped_simple_channels.switches[SWITCHFUNC_FLYMODE] = mix[idx].sw;
        }
        if (found_flymode_switch && found_gyro_switch && found_drexp_rud_switch && found_drexp_ail_switch && found_drexp_ele_switch)
            break;  // don't need to check the rest
    }

    // initialize
    for (FunctionSwitch switch_type = SWITCHFUNC_FLYMODE; switch_type < SWITCHFUNC_LAST; switch_type++)
        switch_idx[switch_type] = get_switch_idx(switch_type);
}

// the only reason not to save changes in live is I don't want to change mixers' switch in the middle of changing options
void save_changes()
{
    MUSIC_Play(MUSIC_SAVING);
    for (FunctionSwitch switch_type = SWITCHFUNC_FLYMODE; switch_type < SWITCHFUNC_LAST; switch_type++)
        mapped_simple_channels.switches[switch_type] = switch_idx[switch_type];

    if (Model.limits[mapped_simple_channels.throttle].safetysw)
       Model.limits[mapped_simple_channels.throttle].safetysw =
             mapped_simple_channels.switches[SWITCHFUNC_HOLD]
             ? 0x80 | mapped_simple_channels.switches[SWITCHFUNC_HOLD] // inverse of '0'
             : 0;

    u8 gyro_gear_count = 0;
    u8 gyro_aux2_count = 0;
    u8 drexp_aile_count = 0;
    u8 drexp_elev_count = 0;
    u8 drexp_rudd_count = 0;
    u8 thro_count = 0;
    u8 pit_count = 0;
    struct Mixer *mix = MIXER_GetAllMixers();
    for (u8 i = 0; i < NUM_MIXERS; i++) {
        u8 *count = NULL;
        int sw;
        if (!MIXER_SRC(mix[i].src) || mix[i].mux != MUX_REPLACE)  // all non-replace mux will be considered as program mix in the Standard mode
            continue;
        if (mix[i].dest == mapped_simple_channels.gear) {
            count = &gyro_gear_count;
            sw = SWITCHFUNC_GYROSENSE;
        } else if (mix[i].dest == mapped_simple_channels.aux2) {
            count = &gyro_aux2_count;
            sw = SWITCHFUNC_GYROSENSE;
        } else if (mix[i].dest == mapped_simple_channels.aile) {
            count = &drexp_aile_count;
            sw = SWITCHFUNC_DREXP_AIL;
        } else if (mix[i].dest == mapped_simple_channels.elev) {
            count = &drexp_elev_count;
            sw = SWITCHFUNC_DREXP_ELE;
        } else if (mix[i].dest == mapped_simple_channels.rudd) {
            count = &drexp_rudd_count;
            sw = SWITCHFUNC_DREXP_RUD;
        } else if (mix[i].dest == mapped_simple_channels.pitch) {
            if (pit_count < 3) {
                count = &pit_count;
                sw = SWITCHFUNC_FLYMODE;
            } else if (pit_count == 3 && mix[i].sw != ALWAYSOFF_SWITCH) {
                mix[i].sw = 0x80 | mapped_simple_channels.switches[SWITCHFUNC_HOLD];  // hold curve
                continue;
            }
        } else if (mix[i].dest == mapped_simple_channels.throttle) {
            count = &thro_count;
            sw = SWITCHFUNC_FLYMODE;
        }
        if (count) {
            // bug fix: a) must not assign switch for the 1st mix; b) must assign a none-used switch(virt10 here) for 2-way switch. Otherwise, it won't work properly
            if (*count == 0)
                mix[i].sw = 0;
            else if (*count < INPUT_NumSwitchPos(switch_idx[sw]))
                mix[i].sw = switch_idx[sw] + *count;
            else
                mix[i].sw = ALWAYSOFF_SWITCH;
            *count = (*count) + 1;
        }
    }
}


//From common/_main_config.c
extern int fix_abbrev_src(int origval, int newval, int dir);
static const char *switch_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    FunctionSwitch switch_type = (long)data;
    int first = INP_HAS_CALIBRATION+1;
    int last = INP_LAST-1;
    int newval = GUI_TextSelectHelper(switch_idx[switch_type], first, last, dir, 1, 1, NULL);
    switch_idx[switch_type] = fix_abbrev_src(switch_idx[switch_type], newval, dir);
    return INPUT_SourceNameAbbrevSwitch(mp->tmpstr, switch_idx[switch_type]);
}


