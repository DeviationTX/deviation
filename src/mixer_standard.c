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

 Most of this code is based on the mixer from er9x developed by
 Erez Raviv <erezraviv@gmail.com>
 http://code.google.com/p/er9x/
 and the th9x project
 http://code.google.com/p/th9x/
 */

#include "common.h"
#include "mixer.h"
#include "buttons.h"
#include "config/model.h"
#include "config/tx.h"
#include <stdlib.h>
#include "mixer_standard.h"

MappedSimpleChannels mapped_std_channels;
extern const u8 EATRG0[PROTO_MAP_LEN];

void STDMIXER_Preset()
{
    if(Model.mixer_mode == MIXER_ADVANCED)
        return;
    mapped_std_channels.pitch = NUM_OUT_CHANNELS + 2; // Virt 3 /pit
    mapped_std_channels.rudd = 3;
    mapped_std_channels.gear = GYROOUTPUT_GEAR;
    mapped_std_channels.aux2 = GYROOUTPUT_AUX2;
    mapped_std_channels.switches[SWITCHFUNC_HOLD] = INP_GEAR0;
    mapped_std_channels.switches[SWITCHFUNC_GYROSENSE] = INP_MIX0;
    mapped_std_channels.switches[SWITCHFUNC_FLYMODE] = INP_FMOD0;
    mapped_std_channels.switches[SWITCHFUNC_DREXP_AIL] = INP_AIL_DR0;
    mapped_std_channels.switches[SWITCHFUNC_DREXP_ELE] = INP_ELE_DR0;
    mapped_std_channels.switches[SWITCHFUNC_DREXP_RUD] = INP_FMOD0;

    const u8 *ch_map = CurrentProtocolChannelMap;
    if (! ch_map) {
        // for none protocol, assign any channel to thr is fine
        ch_map = EATRG0;
    }
    for (unsigned ch = 0; ch < 3; ch++) {  // only the first 3 channels need to check
        if (ch_map[ch] == INP_THROTTLE) {
            mapped_std_channels.throttle = ch;
            break;
        }
    }
    mapped_std_channels.aile = NUM_OUT_CHANNELS; // virt 1
    mapped_std_channels.elev = NUM_OUT_CHANNELS +1; // virt 2
    STDMIXER_InitSwitches();
    STDMIXER_SaveSwitches();
}

void STDMIXER_SetChannelOrderByProtocol()
{
    const u8 *ch_map = CurrentProtocolChannelMap;
    if (! ch_map) {
        // for none protocol, assign any channel to thr is fine
        ch_map = EATRG0;
    }
    CLOCK_ResetWatchdog();// this function might be invoked after loading from template/model file, so feeding the dog in the middle
    unsigned safetysw = 0;
    s8 safetyval = 0;
    for (unsigned ch = 0; ch < 3; ch++) {  // only the first 3 channels need to check
        if (Model.limits[ch].safetysw) {
            safetysw = Model.limits[ch].safetysw;
            safetyval = Model.limits[ch].safetyval;
        }
        if (ch_map[ch] == INP_THROTTLE)
            mapped_std_channels.throttle = ch;
        else if (ch_map[ch] == INP_AILERON)
            mapped_std_channels.actual_aile = mapped_std_channels.aile = ch;
        else if (ch_map[ch] == INP_ELEVATOR)
            mapped_std_channels.actual_elev = mapped_std_channels.elev = ch;
    }
    Model.limits[mapped_std_channels.throttle].safetysw = safetysw;
    Model.limits[mapped_std_channels.throttle].safetyval = safetyval;
    Model.limits[mapped_std_channels.aile].safetysw = 0;
    Model.limits[mapped_std_channels.elev].safetysw = 0;
    Model.limits[mapped_std_channels.aile].safetyval = 0;
    Model.limits[mapped_std_channels.elev].safetyval = 0;

    //printf("thro: %d, aile: %d, elev: %d\n\n", mapped_std_channels.throttle, mapped_std_channels.aile, mapped_std_channels.elev);
    MIXER_SetTemplate(mapped_std_channels.throttle, MIXERTEMPLATE_COMPLEX);
    MIXER_SetTemplate(mapped_std_channels.aile, MIXERTEMPLATE_CYC1);
    MIXER_SetTemplate(mapped_std_channels.elev, MIXERTEMPLATE_CYC2);

    struct Mixer *mix = MIXER_GetAllMixers();
    for (unsigned idx = 0; idx < NUM_MIXERS; idx++) {
        if (mix[idx].src ==0)
            continue;
        if (mix[idx].dest == NUM_OUT_CHANNELS + 9)
           mix[idx].src = 0; // remove all mixers pointing to Virt10, because the Virt10 is reserved in Standard mode
        else if (MIXER_MUX(&mix[idx]) == MUX_REPLACE && mix[idx].src== INP_THROTTLE && mix[idx].dest < NUM_OUT_CHANNELS) { // src=THR && dest = virt should be pitch's mixer
            mix[idx].dest = mapped_std_channels.throttle;
        }
    }
    MIXER_SetTemplate(NUM_OUT_CHANNELS + 9, MIXERTEMPLATE_NONE);// remove all mixers pointing to Virt10 as the Virt10 is reserved in Standard mode
    mapped_std_channels.aile = NUM_OUT_CHANNELS; // virt 1
    mapped_std_channels.elev = NUM_OUT_CHANNELS +1; // virt 2

    // Simplfied timer sw, only throttle channel output is possible to be selected
    for (unsigned i = 0; i < NUM_TIMERS; i++) {
        if (Model.timer[i].src)
            Model.timer[i].src = mapped_std_channels.throttle + NUM_INPUTS +1;
        TIMER_Reset(i);
    }
    CLOCK_ResetWatchdog();
}

// Roughly verify if current model is a valid simple model
unsigned STDMIXER_ValidateTraditionModel()
{
    struct Mixer *mix = MIXER_GetAllMixers();
    unsigned thro_mixer_count = 0;
    unsigned pit_mixer_count = 0;
    unsigned drexp_mixer_count = 0;
    unsigned gryo_mixer_count = 0;
    for (unsigned idx = 0; idx < NUM_MIXERS; idx++) {
        if (mix[idx].src == 0  || MIXER_MUX(&mix[idx]) != MUX_REPLACE)  // all none replace mux will be considered as program mix in the Standard mode
            continue;
        if (mix[idx].dest == NUM_OUT_CHANNELS + 9) //mixers pointing to Virt10 as the Virt10 is reserved in Standard mode
            return 0;
        if (mix[idx].dest == mapped_std_channels.pitch)
            pit_mixer_count++;
        else if (mix[idx].dest == mapped_std_channels.throttle)
            thro_mixer_count++;
        else if (mix[idx].dest == mapped_std_channels.gear
                || mix[idx].dest == mapped_std_channels.aux2)
            gryo_mixer_count++;
        else if (mix[idx].dest == mapped_std_channels.aile ||
                mix[idx].dest == mapped_std_channels.elev ||
                mix[idx].dest == mapped_std_channels.rudd)
            drexp_mixer_count++;
    }
    if (thro_mixer_count != THROTTLEMIXER_COUNT ||
            (pit_mixer_count != PITCHMIXER_COUNT && pit_mixer_count != PITCHMIXER_COUNT -1) ||
            gryo_mixer_count < GYROMIXER_COUNT || drexp_mixer_count != DREXPMIXER_COUNT *3)
        return 0;

    unsigned cyc_template_count = 0;
    for (unsigned ch = 0; ch < NUM_OUT_CHANNELS; ch++) {
        switch(Model.templates[ch]) {
        case MIXERTEMPLATE_CYC1:
        case MIXERTEMPLATE_CYC2:
        case MIXERTEMPLATE_CYC3:
            cyc_template_count++;
            break;
        }
    }
    if (cyc_template_count != 3)
        return 0;
    return 1;
}

const char *STDMIXER_ModeName(int mode)
{
    return mode == MIXER_ADVANCED ? _tr_noop("Advanced") : _tr_noop("Standard");
}

void STDMIXER_InitSwitches()
{
    struct Mixer *mix = MIXER_GetAllMixers();

    if (Model.limits[mapped_std_channels.throttle].safetysw)
        mapped_std_channels.switches[SWITCHFUNC_HOLD] =  MIXER_SRC(Model.limits[mapped_std_channels.throttle].safetysw);
    unsigned found_gyro_switch = 0;
    unsigned found_flymode_switch = 0;
    unsigned found_drexp_rud_switch = 0;
    unsigned found_drexp_ail_switch = 0;
    unsigned found_drexp_ele_switch = 0;
    for (unsigned idx = 0; idx < NUM_MIXERS; idx++) {
        if (!MIXER_SRC(mix[idx].src) || MIXER_MUX(&mix[idx]) != MUX_REPLACE)  // all none replace mux will be considered as program mix in the Standard mode
            continue;
        if (!found_gyro_switch && mix[idx].sw != 0 && (mix[idx].dest == mapped_std_channels.gear || mix[idx].dest == mapped_std_channels.aux2)) {
            found_gyro_switch = 1;
            mapped_std_channels.switches[SWITCHFUNC_GYROSENSE] = mix[idx].sw;
        } else if (!found_drexp_rud_switch && mix[idx].dest == mapped_std_channels.rudd && mix[idx].sw != 0) {
            found_drexp_rud_switch = 1;
            mapped_std_channels.switches[SWITCHFUNC_DREXP_RUD] = mix[idx].sw;
        } else if (!found_drexp_ail_switch && mix[idx].dest == mapped_std_channels.aile && mix[idx].sw != 0) {
            found_drexp_ail_switch = 1;
            mapped_std_channels.switches[SWITCHFUNC_DREXP_AIL] = mix[idx].sw;
        } else if (!found_drexp_ele_switch && mix[idx].dest == mapped_std_channels.elev && mix[idx].sw != 0) {
            found_drexp_ele_switch = 1;
            mapped_std_channels.switches[SWITCHFUNC_DREXP_ELE] = mix[idx].sw;
        } else if (!found_flymode_switch && (mix[idx].dest == NUM_OUT_CHANNELS + 2) && mix[idx].sw != 0) { //virt3
            found_flymode_switch = 1;
            mapped_std_channels.switches[SWITCHFUNC_FLYMODE] = mix[idx].sw;
        }
        if (found_flymode_switch && found_gyro_switch && found_drexp_rud_switch && found_drexp_ail_switch && found_drexp_ele_switch)
            break;  // don't need to check the rest
    }
}

void save_switch(int dest, FunctionSwitch switch_type, int thold_sw)
{
    struct Mixer mix[4];
    struct Mixer thold;
    int use_thold = 0;
    int i;
    int sw = INPUT_GetFirstSwitch(mapped_std_channels.switches[switch_type]);
    int count = MIXER_GetMixers(dest, mix, 4);
    if(! count)
        return;
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

void STDMIXER_SaveSwitches()
{
    save_switch(mapped_std_channels.gear, SWITCHFUNC_GYROSENSE, 0);
    save_switch(mapped_std_channels.aux2, SWITCHFUNC_GYROSENSE, 0);
    save_switch(mapped_std_channels.aile, SWITCHFUNC_DREXP_AIL, 0);
    save_switch(mapped_std_channels.elev, SWITCHFUNC_DREXP_ELE, 0);
    save_switch(mapped_std_channels.rudd, SWITCHFUNC_DREXP_RUD, 0);
    save_switch(mapped_std_channels.throttle, SWITCHFUNC_FLYMODE, 0);
    int hold_sw = INPUT_GetFirstSwitch(mapped_std_channels.switches[SWITCHFUNC_HOLD]);
    save_switch(mapped_std_channels.pitch, SWITCHFUNC_FLYMODE, hold_sw);
}
