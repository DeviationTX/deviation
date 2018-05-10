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
#include "music.h"
#include "target.h"
#include <stdlib.h>

#define MIXER_CYC1 (NUM_TX_INPUTS + 1)
#define MIXER_CYC2 (NUM_TX_INPUTS + 2)
#define MIXER_CYC3 (NUM_TX_INPUTS + 3)

extern volatile u8 ppmSync;
extern volatile s32 ppmChannels[MAX_PPM_IN_CHANNELS];
extern volatile u8 ppmin_num_channels;

// Channels should be volatile:
// This array is written from the main event loop
// and read from an interrupt service routine.
// volatile makes sure, each access to the array
// will be an actual access to the memory location
// an element is stored in.
// If it is omitted, the optimizer might create a
// 'short cut', removing seemingly unneccessary memory accesses,
// and thereby preventing the propagation of an update from
// the main loop to the interrupt routine (since the optimizer
// has no clue about interrupts)

volatile s32 Channels[NUM_OUT_CHANNELS];

struct Transmitter Transmitter;

static volatile s32 raw[NUM_SOURCES + 1];
static buttonAction_t button_action;
static unsigned switch_is_on(unsigned sw, volatile s32 *raw);
static s32 get_trim(unsigned src);

static s32 MIXER_CreateCyclicOutput(volatile s32 *raw, unsigned cycnum);

struct Mixer *MIXER_GetAllMixers()
{
    return Model.mixers;
}

struct Trim *MIXER_GetAllTrims()
{
    return Model.trims;
}

void MIXER_EvalMixers(volatile s32 *raw)
{
    int i;
    s32 orig_value[NUM_CHANNELS];
    //3rd step: apply mixers
    for (i = 0; i < NUM_CHANNELS; i++) {
        orig_value[i] = raw[i + NUM_INPUTS + 1];
    }
    for (i = 0; i < NUM_MIXERS; i++) {
        struct Mixer *mixer = &Model.mixers[i];
        // Linkers are pre-ordred such that we can process them in order
        if (MIXER_SRC(mixer->src) == 0) {
            // Mixer is not defined so we are done
            break;
        }
        //apply_mixer updates mixed[mixer->dest]
        MIXER_ApplyMixer(mixer, raw, &orig_value[mixer->dest]);
    }

}

unsigned MIXER_MapChannel(unsigned channel)
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
       case INP_RUDDER:   return INP_AILERON;
       default: return channel;
       }
       break;
    case MODE_4:
       switch(channel) {
       case INP_AILERON:  return INP_RUDDER;
       case INP_THROTTLE: return INP_ELEVATOR;
       case INP_ELEVATOR: return INP_THROTTLE;
       case INP_RUDDER:   return INP_AILERON;
       default: return channel;
       }
       break;
    }
    return channel;
}

static int map_ppm_channels(int idx)
{
    for(int i = 0; i < MAX_PPM_IN_CHANNELS; i++) {
        if(Model.ppm_map[i] == idx) {
            return i;
        }
    }
    return -1;
}
static void MIXER_UpdateRawInputs()
{
    int i;
    //1st step: read input data (sticks, switches, etc) and calibrate
    for (i = 1; i <= NUM_TX_INPUTS; i++) {
        unsigned mapped_channel = MIXER_MapChannel(i);
        if (PPMin_Mode() == PPM_IN_TRAIN2 && Model.train_sw && raw[Model.train_sw] > 0) {
            int ppm_channel_map = map_ppm_channels(i);
            if (ppm_channel_map >= 0) {
                if (ppmSync) {
                    raw[i] = ppmChannels[ppm_channel_map];
                }
                continue;
            }
        }
        raw[i] = CHAN_ReadInput(mapped_channel);
    }
    if (PPMin_Mode() == PPM_IN_SOURCE && ppmSync) {
        for (i = 0; i < (Model.num_ppmin & 0x3f); i++) {
            raw[1 + NUM_INPUTS + NUM_OUT_CHANNELS + NUM_VIRT_CHANNELS + i] = ppmChannels[i];
        }
    }
}

int MIXER_GetCachedInputs(s32 *cache, unsigned threshold)
{
    int changed = 0;
    int i;
    for (i = 1; i <= NUM_TX_INPUTS; i++) {
        if ((unsigned)abs(raw[i] - cache[i]) > threshold) {
            changed = 1;
            cache[i] = raw[i];
        }
    }
    return changed;
}

void MIXER_CalcChannels()
{
    //We retain this array so that we can refer to the prevous values in the next iteration
    int i;
    //1st step: Read Tx inputs
    MIXER_UpdateRawInputs();
    //3rd steps
    MIXER_EvalMixers(raw);

    //4th step: apply auto-templates
    for (i = 0; i < NUM_OUT_CHANNELS; i++) {
        switch(Model.templates[i]) {
            case MIXERTEMPLATE_CYC1:
            case MIXERTEMPLATE_CYC2:
            case MIXERTEMPLATE_CYC3:
                raw[NUM_INPUTS+i+1] = MIXER_CreateCyclicOutput(raw,
                                             Model.templates[i] - MIXERTEMPLATE_CYC1 + 1);
                break;
        }
    }
    //5th step: apply limits
    for (i = 0; i < NUM_OUT_CHANNELS; i++) {
        Channels[i] = MIXER_GetChannel(i, APPLY_ALL);
    }
}

volatile s32 *MIXER_GetInputs()
{
    return raw;
}

s32 MIXER_GetChannel(unsigned channel, enum LimitMask flags)
{
    if (PPMin_Mode() == PPM_IN_TRAIN1 && Model.train_sw && raw[Model.train_sw] > 0) {
        int ppm_channel_map = map_ppm_channels(channel);
        if (ppm_channel_map >= 0) {
            if (ppmSync) {
                return ppmChannels[ppm_channel_map];
            }
            return Channels[channel];   // last value obtained from synchronized ppm set by CalcChannels
        }
    }
    return MIXER_ApplyLimits(channel, &Model.limits[channel], raw, Channels, flags);
}

s16 MIXER_GetChannelDisplayScale(unsigned channel)
{
    if (channel < NUM_OUT_CHANNELS)
        return Model.limits[channel].displayscale;
    else
        return DEFAULT_DISPLAY_SCALE;
}

char* MIXER_GetChannelDisplayFormat(unsigned channel)
{
    if (channel < NUM_OUT_CHANNELS)
        return Model.limits[channel].displayformat;
    else
        return DEFAULT_DISPLAY_FORMAT;
}

#define REZ_SWASH_X(x)  ((x) - (x)/8 - (x)/128 - (x)/512)   //  1024*sin(60) ~= 886
#define REZ_SWASH_Y(x)  (1*(x))   //  1024 => 1024
s32 MIXER_CreateCyclicOutput(volatile s32 *raw, unsigned cycnum)
{
    s32 cyc[3];
    if (! Model.swash_type) {
        return raw[NUM_INPUTS + NUM_OUT_CHANNELS + cycnum];
    }
    s32 aileron    = raw[NUM_INPUTS + NUM_OUT_CHANNELS + 1];
    s32 elevator   = raw[NUM_INPUTS + NUM_OUT_CHANNELS + 2];
    s32 collective = raw[NUM_INPUTS + NUM_OUT_CHANNELS + 3];
    const int normalize = 100;

    if (Model.swash_invert & SWASH_INV_ELEVATOR_MASK)   elevator   = -elevator;
    if (Model.swash_invert & SWASH_INV_AILERON_MASK)    aileron    = -aileron;
    if (Model.swash_invert & SWASH_INV_COLLECTIVE_MASK) collective = -collective;

    switch(Model.swash_type) {
    case SWASH_TYPE_NONE:
    case SWASH_TYPE_LAST:
        cyc[0] = aileron;
        cyc[1] = elevator;
        cyc[2] = collective;
        break;
    case SWASH_TYPE_120:
        aileron  = Model.swashmix[0] * aileron / normalize;
        elevator = Model.swashmix[1] * elevator / normalize;
        collective = Model.swashmix[2] * collective / normalize;
        cyc[0] = collective - elevator;
        cyc[1] = collective + elevator/2 + aileron;
        cyc[2] = collective + elevator/2 - aileron;
        break;
    case SWASH_TYPE_120X:
        aileron  = Model.swashmix[0] * aileron / normalize;
        elevator = Model.swashmix[1] * elevator / normalize;
        collective = Model.swashmix[2] * collective / normalize;
        cyc[0] = collective - aileron;
        cyc[1] = collective + aileron/2 + elevator;
        cyc[2] = collective + aileron/2 - elevator;
        break;
    case SWASH_TYPE_140:
        aileron  = Model.swashmix[0] * aileron / normalize;
        elevator = Model.swashmix[1] * elevator / normalize;
        collective = Model.swashmix[2] * collective / normalize;
        cyc[0] = collective - elevator;
        cyc[1] = collective + elevator + aileron;
        cyc[2] = collective + elevator - aileron;
        break;
    case SWASH_TYPE_90:
        aileron  = Model.swashmix[0] * aileron / normalize;
        elevator = Model.swashmix[1] * elevator / normalize;
        collective = Model.swashmix[2] * collective / normalize;
        cyc[0] = collective - elevator;
        cyc[1] = collective + aileron;
        cyc[2] = collective - aileron;
        break;
    }

    return cyc[cycnum-1];
}

void MIXER_ApplyMixer(struct Mixer *mixer, volatile s32 *raw, s32 *orig_value)
{
    s32 value;
    if (! MIXER_SRC(mixer->src))
        return;
    if (! switch_is_on(mixer->sw, raw)) {
        // Switch is off, so this mixer is not active
        return;
    }
    //1st: Get source value with trim
    value = raw[MIXER_SRC(mixer->src)];
    //Invert if necessary
    if (MIXER_SRC_IS_INV(mixer->src))
        value = - value;

    //2nd: apply curve
    value = CURVE_Evaluate(value, &mixer->curve);

    //3rd: apply scalar and offset
    value = value * mixer->scalar / 100 + PCT_TO_RANGE(mixer->offset);

    //4th: multiplex result
    switch(MIXER_MUX(mixer)) {
    case MUX_REPLACE:
        break;
    case MUX_MULTIPLY:
        value = raw[mixer->dest + NUM_INPUTS + 1] * value / CHAN_MAX_VALUE;
        break;
    case MUX_ADD:
        value = raw[mixer->dest + NUM_INPUTS + 1] + value;
        break;
    case MUX_MAX:
        value = raw[mixer->dest + NUM_INPUTS + 1] > value
                  ? raw[mixer->dest + NUM_INPUTS + 1]
                  : value;
        break;
    case MUX_MIN:
        value = raw[mixer->dest + NUM_INPUTS + 1] < value
                  ? raw[mixer->dest + NUM_INPUTS + 1]
                  : value;
        break;
    case MUX_DELAY:
        {
            //value initially represents 20ths of seconds to cover 60-degrees
            //convert value to represent #msecs to cover 60-degrees (zero->full)
            if (value == 0 || orig_value == NULL) {
                value = raw[mixer->dest + NUM_INPUTS + 1];
                break;
            }
            value = abs(RANGE_TO_PCT(value)) * 50;
            //rate represents the maximum travel per iteration (once per MEDIUM_PRIORITY_MSEC)
            s32 rate = CHAN_MAX_VALUE * MEDIUM_PRIORITY_MSEC / value;

            value = raw[mixer->dest + NUM_INPUTS + 1];
            if (value - *orig_value > rate)
                value = *orig_value + rate;
            else if(value - *orig_value < -rate)
                value = *orig_value - rate;
        }
        break;
#if HAS_EXTENDED_AUDIO
    case MUX_BEEP:
        if (orig_value) {
            s32 new_value = raw[mixer->dest + NUM_INPUTS + 1];
            if (abs(value - new_value) > 100)
                mixer->beep_lock = 0;
            if (mixer->beep_lock == 0) {
                if ((value > *orig_value && value < new_value) ||
                    (value < *orig_value && value > new_value) ||
                    (abs(value - new_value) <= 10)) {
                    mixer->beep_lock = 1;
                    MUSIC_Play(MUSIC_SAVING);
                }
            }
        }
        value = raw[mixer->dest + NUM_INPUTS + 1];	// Use input value
        break;
    case MUX_VOICE:
        if (orig_value) {
            s32 new_value = raw[mixer->dest + NUM_INPUTS + 1];
            if (abs(value - new_value) > 100)
                mixer->voice_lock = 0;
            if (mixer->voice_lock == 0) {
                if ((value > *orig_value && value < new_value) ||
                    (value < *orig_value && value > new_value) ||
                    (abs(value - new_value) <= 10)) {
                    mixer->voice_lock = 1;
                    if (Model.voice.mixer[mixer->dest].music)
                        MUSIC_Play(Model.voice.mixer[mixer->dest].music);
                }
            }
        }
        value = raw[mixer->dest + NUM_INPUTS + 1];	// Use input value
        break;
#endif
    case MUX_LAST: break;
    }

    //5th: apply trim
    if (MIXER_APPLY_TRIM(mixer))
        value = value + (MIXER_SRC_IS_INV(mixer->src) ? -1 : 1) * get_trim(MIXER_SRC(mixer->src));

    //Ensure we don't overflow
    if (value > INT16_MAX)
        value = INT16_MAX;
    else if (value < INT16_MIN)
        value = INT16_MIN;

    raw[mixer->dest + NUM_INPUTS + 1] = value;
}

s32 MIXER_ApplyLimits(unsigned channel, struct Limit *limit, volatile s32 *_raw,
                      volatile s32 *_Channels, enum LimitMask flags)
{
    int applied_safety = 0;
    s32 value = _raw[NUM_INPUTS + 1 + channel] + get_trim(NUM_INPUTS + 1 + channel);
    if (channel >= NUM_OUT_CHANNELS)
        return value;

    if ((flags & APPLY_SAFETY) && MIXER_SRC(limit->safetysw) && switch_is_on(limit->safetysw, _raw)) {
        applied_safety = 1;
        value = PCT_TO_RANGE(Model.limits[channel].safetyval);
    }
    if (flags & APPLY_SCALAR) {
        if (value >= 0 || limit->servoscale_neg == 0)
            value = (s32)value * limit->servoscale / 100;
        else
            value = (s32)value * limit->servoscale_neg / 100;
    }
    if ((flags & APPLY_REVERSE) && (limit->flags & CH_REVERSE)) {
        value = -value;
    }
    if (flags & APPLY_SUBTRIM)
        value += PCT_TO_RANGE(limit->subtrim) / 10;
    if (! applied_safety) {
        //degrees / 100msec
        if (_Channels && (flags & APPLY_SPEED) && limit->speed) {
            s32 rate = CHAN_MAX_VALUE * limit->speed / 60 * MEDIUM_PRIORITY_MSEC / 100;
            if (value - _Channels[channel] > rate)
                value = _Channels[channel] + rate;
            else if(value - _Channels[channel] < -rate)
                value = _Channels[channel] - rate;
        }
    }
    if (flags & APPLY_LIMITS) {
        if (value > PCT_TO_RANGE(limit->max))
            value = PCT_TO_RANGE(limit->max);
        else if( value < PCT_TO_RANGE(-(int)limit->min))
            value = PCT_TO_RANGE(-(int)limit->min);
    } else {
        if (value > INT16_MAX)
            value = INT16_MAX;
        else if (value < INT16_MIN)
            value = INT16_MIN;
    }
    return value;
}

s8 *MIXER_GetTrim(unsigned i)
{
    if (Model.trims[i].sw) {
        for (int j = 0; j < 6; j++) {
            // Assume switch 0/1/2 are in order
            if(raw[Model.trims[i].sw+j] > 0) {
                return &Model.trims[i].value[j];
            }
        }
    }
    return &Model.trims[i].value[0];
}

s32 MIXER_GetTrimValue(int i)
{
    s32 value = *(MIXER_GetTrim(i));
    //0 to 100 step is 0.1
    if (Model.trims[i].step < 10)
        return PCT_TO_RANGE(value * Model.trims[i].step) / 10;
    else
        return PCT_TO_RANGE(value);
}

s32 get_trim(unsigned src)
{
    int i;
    for (i = 0; i < NUM_TRIMS; i++) {
        if (MIXER_MapChannel(Model.trims[i].src) == src) {
            return MIXER_GetTrimValue(i);
        }
    }
    return 0;
}

unsigned switch_is_on(unsigned sw, volatile s32 *raw)
{
    unsigned is_neg = MIXER_SRC_IS_INV(sw);
    sw = MIXER_SRC(sw);
    if(sw == 0) {
        // No switch selected is the same as an on switch
        return 1;
    }
    s32 value = raw[sw];
    if (is_neg)
        value = - value;
    return (value > 0);
}


void MIXER_Init()
{
    memset((void *)Channels, 0, sizeof(Channels));
    memset((void *)raw, 0, sizeof(raw));
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
    BUTTON_RegisterCallback(&button_action, mask, BUTTON_PRESS | BUTTON_LONGPRESS | BUTTON_RELEASE, MIXER_UpdateTrim, NULL);
}
enum TemplateType MIXER_GetTemplate(int ch)
{
    return Model.templates[ch];
};

void MIXER_SetTemplate(int ch, enum TemplateType value)
{
    Model.templates[ch] = value;
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
    unsigned max = NUM_MIXERS;
    unsigned i = 0;
    unsigned j;
    while(i < max) {
        unsigned src = MIXER_SRC(Model.mixers[i].src);
        if(! src
           || Model.templates[Model.mixers[i].dest] == MIXERTEMPLATE_NONE
           || Model.templates[Model.mixers[i].dest] == MIXERTEMPLATE_CYC1
           || Model.templates[Model.mixers[i].dest] == MIXERTEMPLATE_CYC2
           || Model.templates[Model.mixers[i].dest] == MIXERTEMPLATE_CYC3)
        {
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
    //Zero outunused mixers
    memset(Model.mixers + max, 0, sizeof(struct Mixer) * (NUM_MIXERS - max));
    return i;
}

unsigned find_dependencies(unsigned ch, unsigned *deps)
{
    unsigned found = 0;
    unsigned i;
    struct Mixer *mixer;
    for (i = 0; i < NUM_SOURCES; i++)
        deps[i] = 0;
    for (mixer = Model.mixers; mixer < Model.mixers + NUM_MIXERS; mixer++) {
        if (MIXER_SRC(mixer->src) && mixer->dest == ch) {
            found = 1;
            if (MIXER_SRC(mixer->src) > NUM_SOURCES && MIXER_SRC(mixer->src) != NUM_SOURCES + 1 + ch) {
                deps[MIXER_SRC(mixer->src) - NUM_SOURCES - 1] = 1;
            }
            if (MIXER_SRC(mixer->sw) > NUM_SOURCES && MIXER_SRC(mixer->sw) != NUM_SOURCES + 1 + ch) {
                deps[MIXER_SRC(mixer->sw) - NUM_SOURCES - 1] = 1;
            }
        }
    }
    return found;
}

void fix_mixer_dependencies(unsigned mixer_count)
{
    unsigned dependencies[NUM_SOURCES];
    unsigned placed[NUM_SOURCES];
    unsigned pos = 0;
    unsigned last_count = 0;
    unsigned i;
    struct Mixer mixers[NUM_MIXERS];
    memset(mixers, 0, sizeof(mixers));
    for (i = 0; i < NUM_MIXERS; i++) {
        mixers[i].src = 0;
    }
    memset(placed, 0, sizeof(placed));
    while(mixer_count || last_count != mixer_count) {
        last_count = mixer_count;
        for (i = 0; i < NUM_SOURCES; i++) {
            if (placed[i])
                continue;
            if (! find_dependencies(i, dependencies)) {
                placed[i] = 1;
                continue;
            }
            unsigned ok = 1;
            unsigned j;
            // determine if all dependencies have been placed
            for (j = 0; j < NUM_SOURCES; j++) {
                if (dependencies[i] && ! placed[i]) {
                    ok = 0;
                    break;
                }
            }
            if (ok) {
                unsigned num = MIXER_GetMixers(i, &mixers[pos], NUM_MIXERS);
                pos += num;
                mixer_count -= num;
                placed[i] = 1;
            }
        }
    }
    //mixer_count s gauranteed to be 0 when we get here
    //if (mixer_count) {
    //    printf("Could not place all mixers!\n");
    //    return;
    //}
    for (i = 0; i < NUM_MIXERS; i++)
        Model.mixers[i] = mixers[i];
}

int MIXER_SetMixers(struct Mixer *mixers, int count)
{
    int i;
    if (count) {
        int mixer_count = 0;
        unsigned dest = mixers[0].dest;
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
    unsigned pos = compact_mixers();
    for (i = 0; i < count; i++) {
        if (! MIXER_SRC(mixers[i].src) && CURVE_TYPE(&mixers[i].curve) == CURVE_FIXED) {
            mixers[i].src = mixers[i].sw || 1;
        }
        if (MIXER_SRC(mixers[i].src)) {
            Model.mixers[pos] = mixers[i];
            if(Model.templates[mixers[i].dest] != MIXERTEMPLATE_COMPLEX) {
                //Always apply the trim if the template is not 'complex'
                MIXER_SET_APPLY_TRIM(&Model.mixers[pos], 1);
            }
            pos++;
        }
    }
    fix_mixer_dependencies(pos);
    return 1;
}

struct Limit *MIXER_GetLimit(int ch)
{
    if (ch < NUM_OUT_CHANNELS)
        return &Model.limits[ch];
    else
        return NULL;
}

void MIXER_SetLimit(int ch, struct Limit *limit)
{
    if (ch < NUM_OUT_CHANNELS)
        Model.limits[ch] = *limit;
}

void MIXER_InitMixer(struct Mixer *mixer, unsigned ch)
{
    int i;
    mixer->src = ch + 1;
    mixer->dest = ch;
    mixer->scalar = 100;
    mixer->offset = 0;
    mixer->sw = 0;
    CURVE_SET_TYPE(&mixer->curve, CURVE_EXPO);
    for (i = 0; i < MAX_POINTS; i++)
        mixer->curve.points[i] = 0;
}

#if HAS_EXTENDED_AUDIO
static void _trim_music_play(int trim_idx, int is_neg, int on_state)
{
    int button_idx;

    if (is_neg)
        button_idx = Model.trims[trim_idx].neg - 1;
    else
        button_idx = Model.trims[trim_idx].pos - 1;
    if (on_state) {
        if (Model.voice.buttons[button_idx].on)
            MUSIC_Play(Model.voice.buttons[button_idx].on);
    } else {
        if (Model.voice.buttons[button_idx].off)
            MUSIC_Play(Model.voice.buttons[button_idx].off);
    }
}
#endif

static void _trim_as_switch(unsigned flags, int i, int is_neg)
{
    s8 *value = MIXER_GetTrim(i);
    if(Model.trims[i].step == TRIM_MOMENTARY) {
        //Momentarty
        if (flags & BUTTON_PRESS) {
            *value = 100;
#if HAS_EXTENDED_AUDIO
            _trim_music_play(i, is_neg, 1);
#endif
        } else if (flags & BUTTON_RELEASE) {
            *value = -100;
#if HAS_EXTENDED_AUDIO
            _trim_music_play(i, is_neg, 0);
#endif
        }
    } else if (Model.trims[i].step == TRIM_3POS) {
        if (flags & BUTTON_PRESS) {
            *value = is_neg ? -100 : 100;
#if HAS_EXTENDED_AUDIO
            _trim_music_play(i, is_neg, 1);
#endif
        } else if (flags & BUTTON_RELEASE) {
            *value = 0;
#if HAS_EXTENDED_AUDIO
            _trim_music_play(i, is_neg, 0);
#endif
        }
    } else if (flags & BUTTON_PRESS) {
        if (Model.trims[i].step == TRIM_ONOFF) {
            //On/Off
            *value = is_neg ? -100 : 100;
#if HAS_EXTENDED_AUDIO
            _trim_music_play(i, is_neg, 1);
#endif
        } else {
            //Toggle
            *value = *value == -100 ? 100 : -100;
#if HAS_EXTENDED_AUDIO
            _trim_music_play(i, is_neg, *value == -100 ? 0 : 1);
#endif
        }
    }
}

static u32 last_trim_music_time = 0;
unsigned MIXER_UpdateTrim(u32 buttons, unsigned flags, void *data)
{
#define TRIM_MUSIC_DURATION 100
#define START_TONE 1000
    (void)data;
    int i;
    int orig_step_size = 1;
    int tmp;
    unsigned reach_end = 0; // reach either 100 , 0, or -100
    if (flags & BUTTON_LONGPRESS) {
        if (orig_step_size == 1)
            orig_step_size = 9;
        else if (orig_step_size == 9)
            orig_step_size = 10;
    }
    if (! orig_step_size)
        return 1;
    unsigned volume = 10 * Transmitter.volume;
    for (i = 0; i < NUM_TRIMS; i++) {
        int step_size = orig_step_size;
        reach_end = 0;
        int neg_button = CHAN_ButtonIsPressed(buttons, Model.trims[i].neg);
        if (neg_button || CHAN_ButtonIsPressed(buttons, Model.trims[i].pos)) {
            if (Model.trims[i].step > 190) {
                _trim_as_switch(flags, i, neg_button);
                continue;
            }
            if (flags & BUTTON_RELEASE)
                continue;
            int max = 100;
            if (Model.trims[i].step >= 100) {
                step_size = Model.trims[i].step - 90;
            } else if (Model.trims[i].step > 10) {
                step_size = step_size * Model.trims[i].step / 10;
            }
            if (neg_button)
                step_size = -step_size;
            s8 *value = MIXER_GetTrim(i);
            tmp = (int)(*value) + step_size;
            //print_buttons(buttons);
            if ((int)*value > 0 && tmp <= 0) {
                *value = 0;
                reach_end = 1;
            } else if ((int)*value < 0 && tmp >= 0) {
                *value = 0;
                reach_end = 1;
            } else if (tmp > max) {
                *value = 100;
                reach_end = 1;
            } else if (tmp < -max) {
                *value = -100;
                reach_end = 1;
            } else {
                *value = tmp;
            }

            if (reach_end && (flags & BUTTON_LONGPRESS))
                BUTTON_InterruptLongPress();

            if (Model.trims[i].value[0] == 0) {
                SOUND_SetFrequency(3951, volume);
                SOUND_StartWithoutVibrating(TRIM_MUSIC_DURATION, NULL);
            }
            else if (CLOCK_getms()- last_trim_music_time > TRIM_MUSIC_DURATION + 50) {  // Do not beep too frequently
                last_trim_music_time = CLOCK_getms();
                tmp = (*value >= 0) ? *value : -*value;
                if (step_size >=9 || step_size <= -9)
                    SOUND_SetFrequency(START_TONE + tmp * 10, volume); // start from "c2" tone, frequence = 1000
                else  {  //  for small step change: generate 2 different tone for closing to/away from mid-point
                    if (*value < 0)
                        step_size = -step_size;
                    u16 tone = START_TONE + 300;
                    if (step_size < 0)
                        tone = START_TONE;
                    SOUND_SetFrequency(tone, volume);
                    //printf("tone: %d\n\n", tone);
                }

                SOUND_StartWithoutVibrating(TRIM_MUSIC_DURATION, NULL);
            }
        }
    }
    return 1;
}

unsigned MIXER_SourceHasTrim(unsigned src)
{
    int i;
    for (i = 0; i < NUM_TRIMS; i++)
        if (MIXER_MapChannel(Model.trims[i].src) == src)
            return 1;
    return 0;
}

const char *MIXER_TemplateName(enum TemplateType template)
{
    switch(template) {
    case MIXERTEMPLATE_NONE :   return _tr("None");
    case MIXERTEMPLATE_SIMPLE:  return _tr("Simple");
    case MIXERTEMPLATE_EXPO_DR: return _tr("Expo&DR");
    case MIXERTEMPLATE_COMPLEX: return _tr("Complex");
    case MIXERTEMPLATE_CYC1:    return _tr("Cyclic1");
    case MIXERTEMPLATE_CYC2:    return _tr("Cyclic2");
    case MIXERTEMPLATE_CYC3:    return _tr("Cyclic3");
    default:                    return _tr("Unknown");
    }
}

const char *MIXER_SwashType(enum SwashType swash_type)
{
    switch(swash_type) {
        case SWASH_TYPE_NONE: return _tr("None");
        case SWASH_TYPE_120:  return "120";
        case SWASH_TYPE_120X: return "120X";
        case SWASH_TYPE_140:  return "140";
        case SWASH_TYPE_90:   return "90";
        case SWASH_TYPE_LAST: break;
    }
    return "";
}

void MIXER_SetDefaultLimit(struct Limit *limit)
{
    limit->max = DEFAULT_SERVO_LIMIT;
    limit->min = DEFAULT_SERVO_LIMIT;
    limit->servoscale = 100;
    limit->servoscale_neg = 0;  //match servoscale
    limit->displayscale = DEFAULT_DISPLAY_SCALE;
    strcpy(limit->displayformat, DEFAULT_DISPLAY_FORMAT);
}

int MIXER_GetSourceVal(int idx, u32 opts)
{
    s32 val;
    if (idx <= NUM_INPUTS || idx > NUM_INPUTS + NUM_CHANNELS /*PPM*/) {
        volatile s32 *raw = MIXER_GetInputs();
        val = raw[idx];
    } else {
        val = MIXER_GetChannel(idx - NUM_INPUTS - 1, opts);
    }
    return val;
}

int MIXER_SourceAsBoolean(unsigned src)
{
    if(! src)
        return 0;
    s32 val = MIXER_GetSourceVal(MIXER_SRC(src), APPLY_SAFETY);
    if (MIXER_SRC_IS_INV(src))
        val = -val;
    return (val - CHAN_MIN_VALUE > (CHAN_MAX_VALUE - CHAN_MIN_VALUE) / 20) ? 1 : 0;
}
