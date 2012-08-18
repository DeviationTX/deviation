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

#include "target.h"
#include "mixer.h"
#include "buttons.h"
#include "config/model.h"
#include "config/tx.h"
#include <stdlib.h>

#define SWASH_INV_ELEVATOR_MASK   1
#define SWASH_INV_AILERON_MASK    2
#define SWASH_INV_COLLECTIVE_MASK 4

#define MIXER_CYC1 (NUM_TX_INPUTS + 1)
#define MIXER_CYC2 (NUM_TX_INPUTS + 2)
#define MIXER_CYC3 (NUM_TX_INPUTS + 3)

// Channels should be volatile:
// This array is written from the main event loop
// and read from an interrupt service routine.
// volatile makes sure, each access to the array
// will be an actual access to the memory location
// an element is stored in.
// If it is omitted, the optimizer might create a 
// "short cut", removing seemingly unneccessary memory accesses,
// and thereby preventing the propagation of an update from
// the main loop to the interrupt routine (since the optimizer
// has no clue about interrupts)

volatile s16 Channels[NUM_CHANNELS];

struct Transmitter Transmitter;

static s16 raw[NUM_INPUTS + NUM_CHANNELS + 1];
static buttonAction_t button_action;
static u8 switch_is_on(u8 sw, s16 *raw);
static s16 get_trim(u8 src);
static u8 update_trim(u32 buttons, u8 flags, void *data);

struct Mixer *MIXER_GetAllMixers()
{
    return Model.mixers;
}

struct Trim *MIXER_GetAllTrims()
{
    return Model.trims;
}

void MIXER_EvalMixers(s16 *raw)
{
    int i;
    //3rd step: apply mixers
    for (i = 0; i < NUM_MIXERS; i++) {
        struct Mixer *mixer = &Model.mixers[i];
        // Linkers are pre-ordred such that we can process them in order
        if (MIXER_SRC(mixer->src) == 0) {
            // Mixer is not defined so we are done
            break;
        }
        //apply_mixer updates mixed[mirer->dest]
        MIXER_ApplyMixer(mixer, raw);
    }

}

u8 MIXER_MapChannel(u8 channel)
{
    switch(Transmitter.mode) {
    case MODE_1:
       return channel;
    case MODE_2:
       switch(channel) {
       case INP_THROTTLE: return INP_ELEVATOR;
       case INP_ELEVATOR: return INP_THROTTLE;
       default: return channel;
       }
       break;
    case MODE_3:
       switch(channel) {
       case INP_AILERON:  return INP_RUDDER;
       case INP_THROTTLE: return INP_ELEVATOR;
       case INP_ELEVATOR: return INP_THROTTLE;
       case INP_RUDDER:   return INP_AILERON;
       default: return channel;
       }
       break;
    case MODE_4:
       switch(channel) {
       case INP_AILERON:  return INP_RUDDER;
       case INP_RUDDER:   return INP_AILERON;
       default: return channel;
       }
       break;
    }
    return channel;
}

u8 MIXER_ReadInputs(s16 *raw, u8 threshold)
{
    u8 changed = 0;
    u8 i;
    //1st step: read input data (sticks, switches, etc) and calibrate
    for (i = 1; i <= NUM_TX_INPUTS; i++) {
        u8 mapped_channel = MIXER_MapChannel(i);
        s16 value = CHAN_ReadInput(mapped_channel);
        if (abs(value - raw[i]) > threshold) {
            changed = 1;
            raw[i] = value;
        }
    }
    return changed;
}

void MIXER_CalcChannels()
{
    //We retain this array so that we can refer to the prevous values in the next iteration
    int i;
    //1st step: Read Tx inputs
    MIXER_ReadInputs(raw, 0);
    //2nd step: calculate virtual channels (CCPM, etc)
    MIXER_CreateCyclicInputs(raw);
    //3rd steps
    MIXER_EvalMixers(raw);

    //4th step: apply limits
    for (i = 0; i < NUM_CHANNELS; i++) {
        Channels[i] = MIXER_ApplyLimits(i, &Model.limits[i], raw, APPLY_ALL);
    }
}
s16 *MIXER_GetInputs()
{
    return raw;
}

s16 MIXER_GetChannel(u8 channel, enum LimitMask flags)
{
    return MIXER_ApplyLimits(channel, &Model.limits[channel], raw, flags);
}

#define REZ_SWASH_X(x)  ((x) - (x)/8 - (x)/128 - (x)/512)   //  1024*sin(60) ~= 886
#define REZ_SWASH_Y(x)  (1*(x))   //  1024 => 1024
void MIXER_CreateCyclicInputs(s16 *raw)
{
    if (! Model.swash_type)
        return;
    s8 ele_trim = get_trim(MIXER_MapChannel(INP_ELEVATOR));
    s8 ail_trim = get_trim(MIXER_MapChannel(INP_AILERON));
    s16 elevator   = raw[INP_ELEVATOR] + ele_trim;
    s16 aileron    = raw[INP_AILERON]  + ail_trim;
    s16 collective = raw[Model.collective_source];

    if (Model.swash_invert & SWASH_INV_ELEVATOR_MASK)   elevator   = -elevator;
    if (Model.swash_invert & SWASH_INV_AILERON_MASK)    aileron    = -aileron;
    if (Model.swash_invert & SWASH_INV_COLLECTIVE_MASK) collective = -collective;

    switch(Model.swash_type) {
    case SWASH_TYPE_NONE:
        break;
    case SWASH_TYPE_120:
        elevator = REZ_SWASH_Y(elevator);
        aileron  = REZ_SWASH_X(aileron);
        raw[MIXER_CYC1] = collective - elevator;
        raw[MIXER_CYC2] = collective + elevator/2 + aileron;
        raw[MIXER_CYC3] = collective + elevator/2 - aileron;
        break;
    case SWASH_TYPE_120X:
        elevator = REZ_SWASH_X(elevator);
        aileron = REZ_SWASH_Y(aileron);
        raw[MIXER_CYC1] = collective - aileron;
        raw[MIXER_CYC2] = collective + aileron/2 + elevator;
        raw[MIXER_CYC3] = collective + aileron/2 - elevator;
        break;
    case SWASH_TYPE_140:
        elevator = REZ_SWASH_Y(elevator);
        aileron = REZ_SWASH_Y(aileron);
        raw[MIXER_CYC1] = collective - elevator;
        raw[MIXER_CYC2] = collective + elevator + aileron;
        raw[MIXER_CYC3] = collective + elevator - aileron;
        break;
    case SWASH_TYPE_90:
        elevator = REZ_SWASH_Y(elevator);
        aileron = REZ_SWASH_Y(aileron);
        raw[MIXER_CYC1] = collective - elevator;
        raw[MIXER_CYC2] = collective + aileron;
        raw[MIXER_CYC3] = collective - aileron;
        break;
    }
}

void MIXER_ApplyMixer(struct Mixer *mixer, s16 *raw)
{
    s16 value;
    if (! MIXER_SRC(mixer->src))
        return;
    if (! switch_is_on(mixer->sw, raw)) {
        // Switch is off, so this mixer is not active
        return;
    }
    //1st: Get source value with trim
    value = raw[MIXER_SRC(mixer->src)] + get_trim(MIXER_SRC(mixer->src));
    //Invert if necessary
    if (MIXER_SRC_IS_INV(mixer->src))
        value = - value;

    //2nd: apply curve
    value = CURVE_Evaluate(value, &mixer->curve);

    //3rd: apply scalar and offset
    value = value * mixer->scalar / 100 + PCT_TO_RANGE(mixer->offset);

    //4th: multiplex result
    switch(mixer->mux) {
    case MUX_REPLACE:
        raw[mixer->dest + NUM_INPUTS + 1] = value;
        break;
    case MUX_MULTIPLY:
        raw[mixer->dest + NUM_INPUTS + 1] *= (s32)value / CHAN_MAX_VALUE;
        break;
    case MUX_ADD:
        raw[mixer->dest + NUM_INPUTS + 1] += value;
        break;
    }
}

s16 MIXER_ApplyLimits(u8 channel, struct Limit *limit, s16 *raw, enum LimitMask flags)
{
    s16 value = raw[NUM_INPUTS + 1 + channel];
    if (flags & APPLY_SUBTRIM)
        value += PCT_TO_RANGE(limit->subtrim) / 10;
    if ((flags & APPLY_REVERSE) && (limit->flags & CH_REVERSE))
        value = -value;
    if ((flags & APPLY_SAFETY) && MIXER_SRC(limit->safetysw) && switch_is_on(limit->safetysw, raw)) {
        value = PCT_TO_RANGE(Model.limits[channel].safetyval);
    } else if (flags & APPLY_LIMITS) {
        if (value > PCT_TO_RANGE(limit->max))
            value = PCT_TO_RANGE(limit->max);
        else if( value < PCT_TO_RANGE(limit->min))
            value = PCT_TO_RANGE(limit->min);
    }
    return value;
}

s16 get_trim(u8 src)
{
    int i;
    for (i = 0; i < NUM_TRIMS; i++) {
        if (MIXER_MapChannel(Model.trims[i].src) == src) {
            return PCT_TO_RANGE(Model.trims[i].value * Model.trims[i].step / 10);
        }
    }
    return 0;
}
  
u8 switch_is_on(u8 sw, s16 *raw)
{
    u8 is_neg = MIXER_SRC_IS_INV(sw);
    sw = MIXER_SRC(sw);
    if(sw == 0) {
        // No switch selected is the same as an on switch
        return 1;
    }
    s16 value = raw[sw];
    if (is_neg)
        value = - value;
    return (value > 0);
}


void MIXER_Init()
{
    memset((void *)Channels, 0, sizeof(Channels));
    //memset(&Model, 0, sizeof(Model));
}

void MIXER_RegisterTrimButtons()
{
    int i;
    BUTTON_UnregisterCallback(&button_action);
    u32 mask = 0;
    for (i = 0; i < NUM_TRIMS; i++) {
        mask |= CHAN_ButtonMask(Model.trims[i].neg);
        mask |= CHAN_ButtonMask(Model.trims[i].pos);
    }
    BUTTON_RegisterCallback(&button_action, mask, BUTTON_PRESS | BUTTON_LONGPRESS, update_trim, NULL);
}
enum TemplateType MIXER_GetTemplate(int ch)
{
    return Model.template[ch];
};

void MIXER_SetTemplate(int ch, enum TemplateType value)
{
    Model.template[ch] = value;
};

int MIXER_GetMixers(int ch, struct Mixer *mixers, int count)
{
    int idx = 0;
    int i;
    for(i = 0; i < NUM_MIXERS; i++) {
        if (MIXER_SRC(Model.mixers[i].src) && Model.mixers[i].dest == ch) {
            mixers[idx++] = Model.mixers[i];
            if(idx == count)
                return count;
        }
    }
    return idx;
}

int compact_mixers() {
    u8 max = NUM_MIXERS;
    u8 i = 0;
    u8 j;
    while(i < max) {
        if(! MIXER_SRC(Model.mixers[i].src)) {
            //Found an empty space so move all following mixers down 1 and decrease max
            for (j = i + 1; j < max; j++) {
                Model.mixers[j - 1] = Model.mixers[j];
            }
            max--;
        } else {
            //Found a used mixer so go to the next one
            i++;
        }
    }
    return i;
}

u8 find_dependencies(u8 ch, u8 *deps)
{
    u8 found = 0;
    u8 i;
    struct Mixer *mixer;
    for (i = 0; i < NUM_CHANNELS; i++)
        deps[i] = 0;
    for (mixer = Model.mixers; mixer < Model.mixers + NUM_MIXERS; mixer++) {
        if (MIXER_SRC(mixer->src) && mixer->dest == ch) {
            found = 1;
            if (MIXER_SRC(mixer->src) > NUM_INPUTS && MIXER_SRC(mixer->src) != NUM_INPUTS + 1 + ch) {
                deps[MIXER_SRC(mixer->src) - NUM_INPUTS - 1] = 1;
            } 
            if (MIXER_SRC(mixer->sw) > NUM_INPUTS) {
                deps[MIXER_SRC(mixer->sw) - NUM_INPUTS - 1] = 1;
            }
        }
    }
    return found;
}

void fix_mixer_dependencies(u8 mixer_count)
{
    u8 dependencies[NUM_CHANNELS];
    u8 placed[NUM_CHANNELS];
    u8 pos = 0;
    u8 last_count = 0;
    u8 i;
    struct Mixer mixers[NUM_MIXERS];
    for (i = 0; i < NUM_MIXERS; i++) {
        mixers[i].src = 0;
    }
    for (i = 0; i < NUM_CHANNELS; i++) {
        placed[i] = 0;
    }
    while(mixer_count || last_count != mixer_count) {
        last_count = mixer_count;
        for (i = 0; i < NUM_CHANNELS; i++) {
            if (placed[i])
                continue;
            if (! find_dependencies(i, dependencies)) {
                placed[i] = 1;
                continue;
            }
            u8 ok = 1;
            u8 j;
            // determine if all dependencies have been placed
            for (j = 0; j < NUM_CHANNELS; j++) {
                if (dependencies[i] && ! placed[i]) {
                    ok = 0;
                    break;
                }
            }
            if (ok) {
                u8 num = MIXER_GetMixers(i, &mixers[pos], NUM_MIXERS);
                pos += num;
                mixer_count -= num;
                placed[i] = 1;
            }
        }
    }
    if (mixer_count) {
        printf("Could not place all mixers!\n");
        return;
    }
    for (i = 0; i < NUM_MIXERS; i++)
        Model.mixers[i] = mixers[i];
}

int MIXER_SetMixers(struct Mixer *mixers, int count)
{
    int i;
    if (count) {
        int mixer_count = 0;
        u8 dest = mixers[0].dest;
        //Determine if we have enough free mixers
        for (i = 0; i < NUM_MIXERS; i++) {
            if (MIXER_SRC(Model.mixers[i].src) && Model.mixers[i].dest != dest)
                mixer_count++;
        }
        if (mixer_count + count > NUM_MIXERS) {
            printf("Need %d free mixers, but only %d are available\n", count, NUM_MIXERS - mixer_count);
            return 0;
        }
        //Remove all mixers for this channel
        for (i = 0; i < NUM_MIXERS; i++) {
            if (MIXER_SRC(Model.mixers[i].src) && Model.mixers[i].dest == dest)
                Model.mixers[i].src = 0;
        }
    }
    u8 pos = compact_mixers();
    for (i = 0; i < count; i++) {
        if (MIXER_SRC(mixers[i].src))
            Model.mixers[pos++] = mixers[i];
    }
    fix_mixer_dependencies(pos);
    return 1;
}

void MIXER_GetLimit(int ch, struct Limit *limit)
{
    *limit = Model.limits[ch];
}

void MIXER_SetLimit(int ch, struct Limit *limit)
{
    Model.limits[ch] = *limit;
}

void MIXER_InitMixer(struct Mixer *mixer, u8 ch)
{
    int i;
    mixer->src = ch + 1;
    mixer->dest = ch;
    mixer->scalar = 100;
    mixer->offset = 0;
    mixer->sw = 0;
    mixer->curve.type = CURVE_EXPO;
    for (i = 0; i < MAX_POINTS; i++)
        mixer->curve.points[i] = 0;
}

u8 update_trim(u32 buttons, u8 flags, void *data)
{
    (void)data;
    (void)flags;
    int i;
    static u8 step_size = 1;
    if (flags & BUTTON_PRESS)
        step_size = 1;
    if (flags & BUTTON_LONGPRESS) {
        if (step_size == 1)
            step_size = 9;
        else if (step_size == 9)
            step_size = 10;
    }
    if (! step_size)
        return 1;
    for (i = 0; i < NUM_TRIMS; i++) {
        if (CHAN_ButtonIsPressed(buttons, Model.trims[i].neg)) {
            int tmp = (int)(Model.trims[i].value) - step_size;
            if ((int)(Model.trims[i].value) > 0 && tmp <= 0) {
                Model.trims[i].value = 0;
                step_size = 0;
            } else {
                Model.trims[i].value = tmp < -100 ? -100 : tmp;
            }
            break;
        }
        if (CHAN_ButtonIsPressed(buttons, Model.trims[i].pos)) {
            int tmp = (int)(Model.trims[i].value) + step_size;
            if ((int)(Model.trims[i].value) < 0 && tmp >= 0) {
                Model.trims[i].value = 0;
                step_size = 0;
            } else {
                Model.trims[i].value = tmp > 100 ? 100 : tmp;
            }
            break;
        }
    }
    return 1;
}

const char *MIXER_SourceName(char *str, u8 src)
{
    u8 is_neg = MIXER_SRC_IS_INV(src);
    src = MIXER_SRC(src);

    if(! src) {
        sprintf(str, "None");
    } else if(src <= NUM_TX_INPUTS) {
        sprintf(str, "%s%s", is_neg ? "!" : "", tx_input_str[src - 1]);
    } else if(src <= NUM_INPUTS) {
        sprintf(str, "%sCYC%d", is_neg ? "!" : "", src - NUM_TX_INPUTS);
    } else {
        sprintf(str, "%sCh%d", is_neg ? "!" : "", src - NUM_INPUTS);
    }
    return str;
}
const char *MIXER_TemplateName(enum TemplateType template)
{
    switch(template) {
    case MIXERTEMPLATE_NONE :   return "None";
    case MIXERTEMPLATE_SIMPLE:  return "Simple";
    case MIXERTEMPLATE_EXPO_DR: return "Expo&DR";
    case MIXERTEMPLATE_COMPLEX: return "Complex";
    default:                    return "Unknown";
    }
}

const char *MIXER_ButtonName(u8 button)
{
    if (! button) {
        return "None";
    }
    return tx_button_str[button - 1];
}

const char *MIXER_SwashType(enum SwashType swash_type)
{
    switch(swash_type) {
        case SWASH_TYPE_NONE: return "None";
        case SWASH_TYPE_120:  return "120";
        case SWASH_TYPE_120X: return "120X";
        case SWASH_TYPE_140:  return "140";
        case SWASH_TYPE_90:   return "90";
    }
    return "";
}

void MIXER_AdjustForProtocol()
{
    const u8 *map = ProtocolChannelMap[Model.protocol];
    if(! map)
        return;
    int i, ch;
    u8 template[NUM_CHANNELS];
    memcpy(template, Model.template, sizeof(template));
    for(i = 0; i < NUM_MIXERS; i++) {
        if (! Model.mixers[i].src)
            break;
        for(ch = 0; ch < PROTO_MAP_LEN; ch++)
            if (map[ch] == Model.mixers[i].src) {
                template[ch] = Model.template[Model.mixers[i].dest];
                Model.mixers[i].dest = ch;
                break;
            }
    }
    memcpy(Model.template, template, sizeof(template));
    MIXER_SetMixers(NULL, 0);
}
