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
#include "mixer_simple.h"

MappedSimpleChannels mapped_simple_channels;

void SIMPLEMIXER_Preset()
{
    mapped_simple_channels.pitch = NUM_OUT_CHANNELS + 2; // Virt 3 /pit
    mapped_simple_channels.rudd = 3;
    mapped_simple_channels.gear = GYROOUTPUT_GEAR;
    mapped_simple_channels.aux2 = GYROOUTPUT_AUX2;
    mapped_simple_channels.switches[SWITCHFUNC_HOLD] = INP_RUD_DR;
    mapped_simple_channels.switches[SWITCHFUNC_GYROSENSE] = INP_MIX0;
    mapped_simple_channels.switches[SWITCHFUNC_FLYMODE] = INP_FMOD0;

    if (Model.protocol == 0) {  // for none protocol, assign any channel to thr is fine
        mapped_simple_channels.throttle = 0;
    } else {
        for (u8 ch = 0; ch < 3; ch++) {  // only the first 3 channels need to check
            if (ProtocolChannelMap[Model.protocol][ch] == INP_THROTTLE) {
                mapped_simple_channels.throttle = ch;
                break;
            }
        }
    }
    mapped_simple_channels.aile = NUM_OUT_CHANNELS; // virt 1
    mapped_simple_channels.elev = NUM_OUT_CHANNELS +1; // virt 2
}

void SIMPLEMIXER_SetChannelOrderByProtocol()
{
    if (Model.protocol == 0)
        return;
    CLOCK_ResetWatchdog();// this function might be invoked after loading from template/model file, so feeding the dog in the middle
    u8 safetysw = 0;
    s8 safetyval = 0;
    for (u8 ch = 0; ch < 3; ch++) {  // only the first 3 channels need to check
        if (Model.limits[ch].safetysw) {
            safetysw = Model.limits[ch].safetysw;
            safetyval = Model.limits[ch].safetyval;
        }
        if (ProtocolChannelMap[Model.protocol][ch] == INP_THROTTLE)
            mapped_simple_channels.throttle = ch;
        else if (ProtocolChannelMap[Model.protocol][ch] == INP_AILERON)
            mapped_simple_channels.aile = ch;
        else if (ProtocolChannelMap[Model.protocol][ch] == INP_ELEVATOR)
            mapped_simple_channels.elev = ch;
    }
    Model.limits[mapped_simple_channels.throttle].safetysw = safetysw;
    Model.limits[mapped_simple_channels.throttle].safetyval = safetyval;
    Model.limits[mapped_simple_channels.aile].safetysw = 0;
    Model.limits[mapped_simple_channels.elev].safetysw = 0;
    Model.limits[mapped_simple_channels.aile].safetyval = 0;
    Model.limits[mapped_simple_channels.elev].safetyval = 0;

    //printf("thro: %d, aile: %d, elev: %d\n\n", mapped_simple_channels.throttle, mapped_simple_channels.aile, mapped_simple_channels.elev);
    MIXER_SetTemplate(mapped_simple_channels.throttle, MIXERTEMPLATE_COMPLEX);
    MIXER_SetTemplate(mapped_simple_channels.aile, MIXERTEMPLATE_CYC1);
    MIXER_SetTemplate(mapped_simple_channels.elev, MIXERTEMPLATE_CYC2);

    u8 i = 0;
    struct Mixer *mix = MIXER_GetAllMixers();
    for (u8 idx = 0; idx < NUM_MIXERS; idx++) {
        if (i >= THROTTLEMIXER_COUNT)
            break;
        if (mix[idx].src== INP_THROTTLE && mix[idx].dest < NUM_OUT_CHANNELS) { // src=THR && dest = virt should be pitch's mixer
            mix[idx].dest = mapped_simple_channels.throttle;
            i++;
        }
    }
    mapped_simple_channels.aile = NUM_OUT_CHANNELS; // virt 1
    mapped_simple_channels.elev = NUM_OUT_CHANNELS +1; // virt 2

    // Simplfied timer sw, only throttle channel output is possible to be selected
    for (i = 0; i < NUM_TIMERS; i++) {
        if (Model.timer[i].src)
            Model.timer[i].src = mapped_simple_channels.throttle + NUM_INPUTS +1;
        TIMER_Reset(i);
    }
    CLOCK_ResetWatchdog();
}

// Roughly verify if current model is a valid simple model
u8 SIMPLEMIXER_ValidateTraditionModel()
{
    struct Mixer *mix = MIXER_GetAllMixers();
    u8 thro_mixer_count = 0;
    u8 pit_mixer_count = 0;
    u8 drexp_mixer_count = 0;
    u8 gryo_mixer_count = 0;
    for (u8 idx = 0; idx < NUM_MIXERS; idx++) {
        if (mix[idx].src == 0)
            continue;
        if (mix[idx].dest == mapped_simple_channels.pitch)
            pit_mixer_count++;
        else if (mix[idx].dest == mapped_simple_channels.throttle)
            thro_mixer_count++;
        else if (mix[idx].dest == mapped_simple_channels.gear
                || mix[idx].dest == mapped_simple_channels.aux2)
            gryo_mixer_count++;
        else if (mix[idx].dest == mapped_simple_channels.aile ||
                mix[idx].dest == mapped_simple_channels.elev ||
                mix[idx].dest == mapped_simple_channels.rudd)
            drexp_mixer_count++;
    }
    if (thro_mixer_count != THROTTLEMIXER_COUNT ||
            (pit_mixer_count != PITCHMIXER_COUNT && pit_mixer_count != PITCHMIXER_COUNT -1) ||
            gryo_mixer_count != GYROMIXER_COUNT || drexp_mixer_count != DREXPMIXER_COUNT *3)
        return 0;

    u8 cyc_template_count = 0;
    for (u8 ch = 0; ch < NUM_OUT_CHANNELS; ch++) {
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

const char *SIMPLEMIXER_ModeName(int mode)
{
    return mode == MIXER_ADVANCED ? _tr("Advanced") : _tr("Simple");
}
