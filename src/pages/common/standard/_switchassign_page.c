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

static struct switchassign_page * const mp  = &pagemem.u.switchassign_page;
static struct stdswitch_obj * const gui = &gui_objs.u.stdswitch;

static int get_switch_idx(FunctionSwitch switch_type)
{
    return INPUT_GetFirstSwitch(mapped_std_channels.switches[switch_type]);
}

static void refresh_switches()
{
    STDMIXER_InitSwitches();
    // initialize
    for (FunctionSwitch switch_type = SWITCHFUNC_FLYMODE; switch_type < SWITCHFUNC_LAST; switch_type++)
        mp->switch_idx[switch_type] = get_switch_idx(switch_type);
}

// the only reason not to save changes in live is I don't want to change mixers' switch in the middle of changing options
void save_changes()
{
    for (FunctionSwitch switch_type = SWITCHFUNC_FLYMODE; switch_type < SWITCHFUNC_LAST; switch_type++)
        mapped_std_channels.switches[switch_type] = mp->switch_idx[switch_type];

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
    int sw = INPUT_SelectAbbrevSource(mp->switch_idx[switch_type], dir);
    if (sw < INP_HAS_CALIBRATION+1)
        sw = INP_HAS_CALIBRATION+1;
    mp->switch_idx[switch_type] = sw;
    return INPUT_SourceNameAbbrevSwitch(tempstring, mp->switch_idx[switch_type]);
}

