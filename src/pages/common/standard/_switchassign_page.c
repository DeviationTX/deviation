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

static int get_switch_idx(FunctionSwitch switch_type)
{
    return INPUT_GetFirstSwitch(mapped_std_channels.switches[switch_type]);
}

static void refresh_switches()
{
    STDMIXER_InitSwitches();
    // initialize
    for (FunctionSwitch switch_type = SWITCHFUNC_FLYMODE; switch_type < SWITCHFUNC_LAST; switch_type++)
        switch_idx[switch_type] = get_switch_idx(switch_type);
}

void save_switch(int dest, FunctionSwitch switch_type, int thold_sw)
{
    struct Mixer mix[4];
    struct Mixer thold;
    int use_thold = 0;
    int i;
    int sw = get_switch_idx(switch_type);
    int count = MIXER_GetMixers(dest, mix, 4);
    if(thold_sw && count > 2 && mix[1].sw != mix[count-1].sw) {
        //Pitch uses thold
        thold = mix[count-1];
        use_thold = 1;
    }
    mix[0].sw = 0;
    for(i = 1; i < INPUT_NumSwitchPos(sw); i++) {
        if(i >= count)
            mix[i] = mix[i-1];
        mix[i].sw = sw + i;
    }
    if (use_thold) {
        mix[i] = thold;
        mix[i].sw = 0x80 | thold_sw;
        i++;
    }
    MIXER_SetMixers(mix, i);
}

// the only reason not to save changes in live is I don't want to change mixers' switch in the middle of changing options
void save_changes()
{
    MUSIC_Play(MUSIC_SAVING);
    for (FunctionSwitch switch_type = SWITCHFUNC_FLYMODE; switch_type < SWITCHFUNC_LAST; switch_type++)
        mapped_std_channels.switches[switch_type] = switch_idx[switch_type];

    if (Model.limits[mapped_std_channels.throttle].safetysw)
       Model.limits[mapped_std_channels.throttle].safetysw =
             mapped_std_channels.switches[SWITCHFUNC_HOLD]
             ? 0x80 | mapped_std_channels.switches[SWITCHFUNC_HOLD] // inverse of '0'
             : 0;

    save_switch(mapped_std_channels.gear, SWITCHFUNC_GYROSENSE, 0);
    save_switch(mapped_std_channels.aux2, SWITCHFUNC_GYROSENSE, 0);
    save_switch(mapped_std_channels.aile, SWITCHFUNC_DREXP_AIL, 0);
    save_switch(mapped_std_channels.elev, SWITCHFUNC_DREXP_ELE, 0);
    save_switch(mapped_std_channels.rudd, SWITCHFUNC_DREXP_RUD, 0);
    save_switch(mapped_std_channels.throttle, SWITCHFUNC_FLYMODE, 0);
    save_switch(mapped_std_channels.pitch, SWITCHFUNC_FLYMODE, get_switch_idx(SWITCHFUNC_HOLD));
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


