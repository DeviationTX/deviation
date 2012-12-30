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

#define IMPOSSIBL_SWITCH 254  // assign a big number so that it won't be on
typedef struct {
    u8 switches[3]; // maximum is 3(such as INP_MIX, INP_FMOD), minimun is 2(such as INP_RUD_DR, INP_ELE_DR, INP_AIL_DR, INP_GEAR)
    char name[15];
} SwitchTable;
static SwitchTable switch_table[] = {
    {{INP_RUD_DR, INP_RUD_DR|0x80, IMPOSSIBL_SWITCH}, "RUD DR 0/1"},  // no translation for switch labels
    {{INP_ELE_DR, INP_ELE_DR|0x80, IMPOSSIBL_SWITCH}, "ELE DR 0/1"},
    {{INP_AIL_DR, INP_AIL_DR|0x80, IMPOSSIBL_SWITCH}, "AIL DR 0/1"},
    {{INP_GEAR,   INP_GEAR|0x80,   IMPOSSIBL_SWITCH}, "GEAR 0/1"},
    {{INP_MIX0,   INP_MIX1,        INP_MIX2},         "MIX 0/1/2"},
    {{INP_FMOD0,  INP_FMOD1,       INP_FMOD2},        "FMOD 0/1/2"},
};
static u8 switch_idx[3];

static u8 get_switch_idx(FunctionSwitch switch_type) {
    for (u8 i = 0; i < 6; i++) {
        for (u8 j = 0; j < 3; j++) {
            if (mapped_simple_channels.swicthes[switch_type] == switch_table[i].switches[j])
                return i;
        }
    }
    //printf("something is wrong in get_switch_idx()\n\n");
    return 0; // this is impossible
}

static void refresh_switches()
{
    struct Mixer *mix = MIXER_GetAllMixers();

    if (Model.limits[mapped_simple_channels.throttle].safetysw)
        mapped_simple_channels.swicthes[SWITCHFUNC_HOLD] =  Model.limits[mapped_simple_channels.throttle].safetysw;
    u8 found_gyro_switch = 0;
    u8 found_flymode_switch = 0;
    for (u8 idx = 0; idx < NUM_MIXERS; idx++) {
        if (!mix[idx].src)
            continue;
        if (!found_gyro_switch && mix[idx].sw != 0 && (mix[idx].dest == mapped_simple_channels.gear || mix[idx].dest == mapped_simple_channels.aux2)) {
            found_gyro_switch = 1;
            mapped_simple_channels.swicthes[SWITCHFUNC_GYROSENSE] = mix[idx].sw;
        } else if (!found_flymode_switch && mix[idx].dest == NUM_OUT_CHANNELS && mix[idx].sw != 0) { //virt1
            found_flymode_switch = 1;
            mapped_simple_channels.swicthes[SWITCHFUNC_FLYMODE] = mix[idx].sw;
        }
        if (found_flymode_switch && found_gyro_switch)
            break;  // don't need to check the rest
    }

    // initialize
    for (FunctionSwitch switch_type = SWITCHFUNC_FLYMODE; switch_type <= SWITCHFUNC_GYROSENSE; switch_type++)
        switch_idx[switch_type] = get_switch_idx(switch_type);
}

// the only reason not to save changes in live is I don't want to change mixers' switch in the middle of changing options
void save_changes()
{
    MUSIC_Play(MUSIC_SAVING);
    for (FunctionSwitch switch_type = SWITCHFUNC_FLYMODE; switch_type <= SWITCHFUNC_GYROSENSE; switch_type++)
        mapped_simple_channels.swicthes[switch_type] = switch_table[switch_idx[switch_type]].switches[0];

    if (Model.limits[mapped_simple_channels.throttle].safetysw)
       Model.limits[mapped_simple_channels.throttle].safetysw = mapped_simple_channels.swicthes[SWITCHFUNC_HOLD];

    u8 gyro_count = 0;
    u8 drexp_aile_count = 0;
    u8 drexp_elev_count = 0;
    u8 drexp_rudd_count = 0;
    u8 thro_count = 0;
    u8 pit_count = 0;
    struct Mixer *mix = MIXER_GetAllMixers();
    for (u8 i = 0; i < NUM_MIXERS; i++) {
        if (!mix[i].src)
            continue;

        if (mix[i].dest == mapped_simple_channels.gear || mix[i].dest == mapped_simple_channels.aux2) {
            if (gyro_count < 3)
                mix[i].sw = switch_table[switch_idx[SWITCHFUNC_GYROSENSE]].switches[gyro_count++];
        } else if (mix[i].dest == mapped_simple_channels.aile) {
            if (drexp_aile_count < 3)
                mix[i].sw = switch_table[switch_idx[SWITCHFUNC_FLYMODE]].switches[drexp_aile_count++];
        } else if (mix[i].dest == mapped_simple_channels.elev) {
            if (drexp_elev_count < 3)
                mix[i].sw = switch_table[switch_idx[SWITCHFUNC_FLYMODE]].switches[drexp_elev_count++];
        } else if (mix[i].dest == mapped_simple_channels.rudd) {
            if (drexp_rudd_count < 3)
                mix[i].sw = switch_table[switch_idx[SWITCHFUNC_FLYMODE]].switches[drexp_rudd_count++];
        } else if (mix[i].dest == mapped_simple_channels.pitch) {
            if (pit_count < 3)
                mix[i].sw = switch_table[switch_idx[SWITCHFUNC_FLYMODE]].switches[pit_count++];
            else if (pit_count == 3)
                mix[i].sw = switch_table[switch_idx[SWITCHFUNC_HOLD]].switches[0];  // hold curve
        } else if (mix[i].dest == mapped_simple_channels.throttle) {
            if (thro_count < 3)
                mix[i].sw = switch_table[switch_idx[SWITCHFUNC_FLYMODE]].switches[thro_count++];
        }
    }
}

static const char *switch_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    FunctionSwitch switch_type = (long)data;
    switch_idx[switch_type] = GUI_TextSelectHelper(switch_idx[switch_type], 0, 5, dir, 1, 1, NULL);
    strcpy(mp->tmpstr, switch_table[switch_idx[switch_type]].name);
    return mp->tmpstr;
}


