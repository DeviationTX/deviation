/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Foobar is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Foobar.  If not, see <http://www.gnu.org/licenses/>.

 Most of this code is based on the mixer from er9x developed by
 Erez Raviv <erezraviv@gmail.com>
 http://code.google.com/p/er9x/
 and the th9x project
 http://code.google.com/p/th9x/
 */

#include "target.h"
#include "mixer.h"

#define SWASH_INV_ELEVATOR_MASK   1
#define SWASH_INV_AILERON_MASK    2
#define SWASH_INV_COLLECTIVE_MASK 4

#define MIX_CYC1 (NUM_TX_INPUTS + 1)
#define MIX_CYC2 (NUM_TX_INPUTS + 2)
#define MIX_CYC3 (NUM_TX_INPUTS + 3)


struct Model {
    enum SwashType swash_type;
    u8 swash_invert;
    u8 Elevator_Stick;
    u8 Aileron_Stick;
    u8 Collective_Stick;
    s8 trim[NUM_TRIMS];
    struct Mixer mixers[NUM_MIXERS];
    struct Limit limits[NUM_CHANNELS];
    u8 template[NUM_CHANNELS];
};

struct Model Model;
s16 Channels[NUM_CHANNELS];
struct Transmitter Transmitter;
static void apply_mixer(struct Mixer *mixer, s16 *raw, s16 *mixed);
static void create_cyclic_inputs (s16 *raw);
static s16 apply_limits(s16 value, int channel);
static u8 switch_is_on(u8 sw, s16 *raw, s16 *mixed);

void MIX_CalcChannels()
{
    s16 raw[NUM_INPUTS + 1];
    s16 mixed[NUM_CHANNELS];
    int i;
    //1st step: read input data (sticks, switches, etc) and calibrate
    for (i = 1; i <= NUM_TX_INPUTS; i++) {
        raw[i] = CHAN_ReadInput(i);
    }

    //2nd step: calculate virtual channels (CCPM, etc)
    create_cyclic_inputs(raw);

    //3rd step: apply mixers
    for (i = 0; i < NUM_MIXERS; i++) {
        struct Mixer *mixer = &Model.mixers[i];
        // Linkers are pre-ordred such that we can process them in order
        if (mixer->src == 0) {
            // Mixer is not defined so we are done
            break;
        }
        //apply_mixer updates mixed[mirer->dest]
        apply_mixer(mixer, raw, mixed);
    }

    //4th step: apply limits
    for (i = 0; i < NUM_CHANNELS; i++) {
        Channels[i] = apply_limits(mixed[i], i);
    }
}

#define REZ_SWASH_X(x)  ((x) - (x)/8 - (x)/128 - (x)/512)   //  1024*sin(60) ~= 886
#define REZ_SWASH_Y(x)  ((x))   //  1024 => 1024
void create_cyclic_inputs(s16 *raw)
{
    if (! Model.swash_type)
        return;

    s16 elevator   = raw[Model.Elevator_Stick] + Model.trim[Model.Elevator_Stick];
    s16 aileron    = raw[Model.Aileron_Stick]  + Model.trim[Model.Aileron_Stick];
    s16 collective = raw[Model.Collective_Stick];

    if (Model.swash_invert & SWASH_INV_ELEVATOR_MASK)   elevator   = -elevator;
    if (Model.swash_invert & SWASH_INV_AILERON_MASK)    aileron    = -aileron;
    if (Model.swash_invert & SWASH_INV_COLLECTIVE_MASK) collective = -collective;

    switch(Model.swash_type) {
    case SWASH_TYPE_NONE:
        break;
    case SWASH_TYPE_120:
        elevator = REZ_SWASH_Y(elevator);
        aileron  = REZ_SWASH_X(aileron);
        raw[MIX_CYC1] = collective - elevator;
        raw[MIX_CYC2] = collective + elevator/2 + aileron;
        raw[MIX_CYC3] = collective + elevator/2 - aileron;
        break;
    case SWASH_TYPE_120X:
        elevator = REZ_SWASH_X(elevator);
        aileron = REZ_SWASH_Y(aileron);
        raw[MIX_CYC1] = collective - aileron;
        raw[MIX_CYC2] = collective + aileron/2 + elevator;
        raw[MIX_CYC3] = collective + aileron/2 - elevator;
        break;
    case SWASH_TYPE_140:
        elevator = REZ_SWASH_Y(elevator);
        aileron = REZ_SWASH_Y(aileron);
        raw[MIX_CYC1] = collective - elevator;
        raw[MIX_CYC2] = collective + elevator + aileron;
        raw[MIX_CYC3] = collective + elevator - aileron;
        break;
    case SWASH_TYPE_90:
        elevator = REZ_SWASH_Y(elevator);
        aileron = REZ_SWASH_Y(aileron);
        raw[MIX_CYC1] = collective - elevator;
        raw[MIX_CYC2] = collective + aileron;
        raw[MIX_CYC3] = collective - aileron;
        break;
    }
}

void apply_mixer(struct Mixer *mixer, s16 *raw, s16 *mixed)
{
    s16 value;
    if (! switch_is_on(mixer->sw, raw, mixed)) {
        // Switch is off, so this mixer is not active
        return;
    }
    //1st: Get source value
    if (mixer->src < NUM_INPUTS + 1) {
        // Mixer is using raw input as its source
        value = raw[mixer->src];
    } else if (mixer->src == NUM_INPUTS + 1 + mixer->dest) {
        // Mixer is using itself (retain)
        value = Channels[mixer->dest];
    } else {
        // Mixer is using another output-channel as its source
        value = mixed[mixer->src - NUM_INPUTS - 1];
    }

    //2nd: apply curve
    value = CURVE_Evaluate(value, &mixer->curve);

    //3rd: apply scaler and offset
    value = value * mixer->scaler / 100 + PCT_TO_RANGE(mixer->offset);

    //4th: multiplex result
    switch(mixer->mux) {
    case MUX_REPLACE:
        mixed[mixer->dest] = value;
        break;
    case MUX_MULTIPLY:
        mixed[mixer->dest] *= (s32)value / CHAN_MAX_VALUE;
        break;
    case MUX_ADD:
        mixed[mixer->dest] += value;
        break;
    }
}

s16 apply_limits(s16 value, int channel)
{
    if(value > PCT_TO_RANGE(Model.limits[channel].max))
        value = PCT_TO_RANGE(Model.limits[channel].max);
    else if( value < PCT_TO_RANGE(Model.limits[channel].min))
        value = PCT_TO_RANGE(Model.limits[channel].min);
    return value;
}

u8 switch_is_on(u8 sw, s16 *raw, s16 *mixed)
{
    if(sw == 0) {
        // No switch selected is the same as an on switch
        return 1;
    }
    if(sw <= NUM_INPUTS) {
        return (raw[sw] > 0);
    }
    return (mixed[sw - NUM_INPUTS] > 0);
}


void TEST_init_mixer()
{
    int i;
    memset(Channels, 0, sizeof(Channels));
    memset(&Model, 0, sizeof(Model));
    Model.swash_type = SWASH_TYPE_120;
    Model.Elevator_Stick   = INP_ELEVATOR;
    Model.Aileron_Stick    = INP_AILERON;
    Model.Collective_Stick = INP_THROTTLE;

    Model.mixers[0].src = 1;
    Model.mixers[0].dest = 0;

    for (i = 0; i < 4; i++) {
        Model.mixers[i].src = i + 1;
        Model.mixers[i].dest = i;
        Model.mixers[i].scaler = 100;
    }

    //Test simple scaler
    Model.mixers[0].scaler = -50;
    Model.mixers[0].offset = +50;
    Model.mixers[0].dest  =  10;

    //Test curves
    Model.mixers[1].curve.type = CURVE_3POINT;
    Model.mixers[1].curve.points[0] = 75;
    Model.mixers[1].curve.points[1] = 10;
    Model.mixers[1].curve.points[2] = 75;

    Model.mixers[4].src = NUM_INPUTS + 10 + 1;
    Model.mixers[4].dest = 0;
    
    Model.mixers[5].src  = NUM_INPUTS + 10 + 1;
    Model.mixers[5].dest = 0;
    Model.mixers[5].mux  = MUX_ADD;

    for(i = 0; i < NUM_CHANNELS; i++) {
        Model.limits[i].max = 100;
        Model.limits[i].min = -100;
        Model.template[i] = MIXERTEMPLATE_NONE;
    }
}


enum TemplateType MIX_GetTemplate(int ch)
{
    return Model.template[ch];
};

void MIX_SetTemplate(int ch, enum TemplateType value)
{
    Model.template[ch] = value;
};

int MIX_GetMixers(int ch, struct Mixer *mixers, int count)
{
    int idx = 0;
    int i;
    for(i = 0; i < NUM_MIXERS; i++) {
        if (Model.mixers[i].src && Model.mixers[i].dest == ch) {
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
        if(! Model.mixers[i].src) {
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
        if (mixer->src && mixer->dest == ch) {
            found = 1;
            if (mixer->src > NUM_INPUTS && mixer->src != NUM_INPUTS + 1 + ch) {
                deps[mixer->src - NUM_INPUTS - 1] = 1;
            } 
            if (mixer->sw > NUM_INPUTS) {
                deps[mixer->sw - NUM_INPUTS - 1] = 1;
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
                u8 num = MIX_GetMixers(i, &mixers[pos], NUM_MIXERS);
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

int MIX_SetMixers(struct Mixer *mixers, int count)
{
    int i;
    u8 dest = mixers[0].dest;
    //Remove all mixers for this channel
    for (i = 0; i < NUM_MIXERS; i++) {
        if (Model.mixers[i].src && Model.mixers[i].dest == dest)
            Model.mixers[i].src = 0;
    }
    u8 pos = compact_mixers();
    if (pos + count > NUM_MIXERS) {
        printf("Need %d free mixers, but only %d are available\n", count, NUM_MIXERS - pos);
        return 0;
    }
    for (i = 0; i < count; i++) {
        if (mixers[i].src)
            Model.mixers[pos++] = mixers[i];
    }
    fix_mixer_dependencies(pos);
    return 0;
}

void MIX_GetLimit(int ch, struct Limit *limit)
{
    *limit = Model.limits[ch];
}

void MIX_SetLimit(int ch, struct Limit *limit)
{
    Model.limits[ch] = *limit;
}

void MIX_InitMixer(struct Mixer *mixer, u8 ch)
{
    int i;
    mixer->src = ch + 1;
    mixer->dest = ch;
    mixer->scaler = 100;
    mixer->offset = 0;
    mixer->curve.type = CURVE_NONE;
    for (i = 0; i < MAX_POINTS; i++)
        mixer->curve.points[i] = 0;
}
