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

#define SWASH_INV_ELEVATOR_MASK   1
#define SWASH_INV_AILERON_MASK    2
#define SWASH_INV_COLLECTIVE_MASK 4

#define NUM_TRIMS 4
#define MAX_POINTS 13
#define NUM_MIXERS 16
#define NUM_TX_INPUTS 4
#define NUM_INPUTS (NUM_TX_INPUTS + 3)
#define NUM_CHANNELS 12
#define MIX_CYC1 (NUM_TX_INPUTS + 0)
#define MIX_CYC2 (NUM_TX_INPUTS + 1)
#define MIX_CYC3 (NUM_TX_INPUTS + 2)

#define CURVE_TYPE(x) x.num_points
#define NO_CURVE   0x80
#define MIN_MAX    0x81
#define ZERO_MAX   0x82
#define GT_ZERO    0x83
#define LT_ZERO    0x84
#define ABSVAL     0x85

//MAX = 32700
//MIN = -32700
#define PCT_TO_RANGE(x) ((s16)x * 327)
#define RANGE_TO_PCT(x) ((s16)x / 327)
#define MAX_VALUE (327 * 100)
#define MIN_VALUE (327 * 100)

struct Calibration {
    u8 value;
};
struct Transmitter {
    struct Calibration calibration[NUM_INPUTS];
} Transmitter;

struct Curve {
    u8 num_points;
    s8 points[MAX_POINTS];
};

enum MuxType {
    MUX_REPLACE,
    MUX_MULTIPLY,
    MUX_ADD,
};

enum SwashType {
    SWASH_TYPE_NONE,
    SWASH_TYPE_120,
    SWASH_TYPE_120X,
    SWASH_TYPE_140,
    SWASH_TYPE_90,
};
struct Mixer {
    u8 src;
    u8 dest;
    u8 sw;
    struct Curve curve;
    s8 scaler;
    s8 offset;
    enum MuxType multiplex;
};

struct Limit {
    s8 max;
    s8 min;
};

struct Model {
    enum SwashType swash_type;
    u8 swash_invert;
    u8 Elevator_Stick;
    u8 Aileron_Stick;
    u8 Collective_Stick;
    s8 trim[NUM_TRIMS];
    struct Mixer mixers[NUM_MIXERS];
    struct Limit limits[NUM_CHANNELS];
};
struct Model Model;
s16 Channels[NUM_CHANNELS];

static void apply_mixer(struct Mixer *mixer, s16 *raw, s16 *mixed);
static void create_cyclic_inputs (s16 *raw);
static s16 apply_limits(s16 value, int channel);

static s16 read_input(u8 input, struct Calibration *calibration);
static u8 switch_is_on(u8 sw);
static s16 apply_curve(struct Curve *curve, s16 value);

void MIX_CalcChannels()
{
    s16 raw[NUM_INPUTS];
    s16 mixed[NUM_CHANNELS];
    int i;
    //1st step: read input data (sticks, switches, etc) and calibrate
    for (i = 0; i < NUM_TX_INPUTS; i++) {
        raw[i] = read_input(i, &Transmitter.calibration[i]);
    }

    //2nd step: calculate virtual channels (CCPM, etc)
    create_cyclic_inputs(raw);

    //3rd step: apply mixers
    for (i = 0; i < NUM_MIXERS; i++) {
        struct Mixer *mixer = &Model.mixers[i];
        // Linkers are pre-ordred such that we can process them in order
        if (mixer->src == NUM_CHANNELS + NUM_INPUTS) {
            // Mixer is not defined so we are done
            break;
        }
        if (! switch_is_on(mixer->sw)) {
            // Switch is off, so this mixer is not active
            continue;
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
    //1st: Get source value
    if (mixer->src < NUM_INPUTS) {
        // Mixer is using raw input as its source
        value = raw[mixer->src];
    } else if (mixer->src == NUM_INPUTS + mixer->dest) {
        // Mixer is using itself (retain)
        value = Channels[mixer->dest];
    } else {
        // Mixer is using another output-channel as its source
        value = mixed[mixer->src - NUM_INPUTS];
    }

    //2nd: apply curve
    switch (CURVE_TYPE(mixer->curve)) {
        case NO_CURVE:  break;
        case MIN_MAX:  value = (value < 0) ? MIN_VALUE : MAX_VALUE;
        case ZERO_MAX: value = (value < 0) ? 0 : MAX_VALUE;
        case GT_ZERO:  value = (value < 0) ? 0 : value;
        case LT_ZERO:  value = (value > 0) ? 0 : value;
        case ABSVAL:   value = (value < 0) ? -value : value;
        default:       value = apply_curve(&mixer->curve, value);
    }

    //3rd: apply scaler and offset
    value = value * mixer->scaler / 100 + PCT_TO_RANGE(mixer->offset);

    //4th: multiplex result
    switch(mixer->multiplex) {
    case MUX_REPLACE:
        mixed[mixer->dest] = value;
        break;
    case MUX_MULTIPLY:
        mixed[mixer->dest] *= (s32)value / MAX_VALUE;
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
