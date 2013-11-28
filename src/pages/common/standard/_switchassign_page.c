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

// the only reason not to save changes in live is I don't want to change mixers' switch in the middle of changing options
void save_changes()
{
    for (FunctionSwitch switch_type = SWITCHFUNC_FLYMODE; switch_type < SWITCHFUNC_LAST; switch_type++)
        mapped_std_channels.switches[switch_type] = switch_idx[switch_type];

    if (Model.limits[mapped_std_channels.throttle].safetysw)
       Model.limits[mapped_std_channels.throttle].safetysw =
             mapped_std_channels.switches[SWITCHFUNC_HOLD]
             ? 0x80 | mapped_std_channels.switches[SWITCHFUNC_HOLD] // inverse of '0'
             : 0;

    STDMIXER_SaveSwitches();
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
    switch_idx[switch_type] = INPUT_GetAbbrevSource(switch_idx[switch_type], newval, dir);
    return INPUT_SourceNameAbbrevSwitch(tempstring, switch_idx[switch_type]);
}


