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
#include "target.h"
#include "template.h"
#include "config/model.h"

struct {
    struct Mixer mix[4];
    u8 template[4];
} simple =
{
  {
/* Mixer - Throttle*/
    {INP_THROTTLE, 0, 0, {0, {0}}, 100, 0, MUX_REPLACE},
/* Mixer - Rudder*/
    {INP_RUDDER,   1, 0, {0, {0}}, 100, 0, MUX_REPLACE},
/* Mixer - Elevator*/
    {INP_ELEVATOR, 2, 0, {0, {0}}, 100, 0, MUX_REPLACE},
/* Mixer - Aileron*/
    {INP_AILERON,  3, 0, {0, {0}}, 100, 0, MUX_REPLACE},
  },
  {MIXERTEMPLATE_SIMPLE, MIXERTEMPLATE_SIMPLE, MIXERTEMPLATE_SIMPLE, MIXERTEMPLATE_SIMPLE},
};

struct {
    struct Mixer mix[7];
    u8 template[4];
} _4chdr =
{
  {
/* Mixer - Throttle*/
    {INP_THROTTLE, 0, 0, {0, {0}}, 100, 0, MUX_REPLACE},
/* Mixer - Rudder*/
    {INP_RUDDER,   1, 0, {0, {0}}, 100, 0, MUX_REPLACE},
    {INP_RUDDER,   1, INP_RUD_DR, {0, {0}}, 60, 0, MUX_REPLACE},
/* Mixer - Elevator*/
    {INP_ELEVATOR, 2, 0, {0, {0}}, 100, 0, MUX_REPLACE},
    {INP_ELEVATOR, 2, INP_ELE_DR, {0, {0}}, 60, 0, MUX_REPLACE},
/* Mixer - Aileron*/
    {INP_AILERON,  3, 0, {0, {0}}, 100, 0, MUX_REPLACE},
    {INP_AILERON,  3, INP_AIL_DR, {0, {0}}, 60, 0, MUX_REPLACE},
  },
  {MIXERTEMPLATE_SIMPLE, MIXERTEMPLATE_EXPO_DR, MIXERTEMPLATE_EXPO_DR, MIXERTEMPLATE_EXPO_DR},
};

#define COLLECTIVE_CH 9
struct {
    struct Mixer mix[11];
    u8 template[10];
} heli6ch =
{
  {
/* Mixer - CYC1 */
    {NUM_TX_INPUTS+1, 0, 0, {0, {0}}, 100, 0, MUX_REPLACE},
/* Mixer - CYC2 */
    {NUM_TX_INPUTS+2, 1, 0, {0, {0}}, 100, 0, MUX_REPLACE},
/* Mixer - CYC3 */
    {NUM_TX_INPUTS+3, 2, 0, {0, {0}}, 100, 0, MUX_REPLACE},
/* Mixer - Rudder */
    {INP_RUDDER,      3, 0, {0, {0}}, 100, 0, MUX_REPLACE},
/* Mixer - Throttle */
    {INP_THROTTLE,    4, INP_FMOD0, {CURVE_5POINT, {-100, -20, 30, 70, 90}},  100, 0, MUX_REPLACE},
    {INP_THROTTLE,    4, INP_FMOD1, {CURVE_5POINT, {80, 70, 60, 70, 100}},    100, 0, MUX_REPLACE},
    {INP_THROTTLE,    4, INP_FMOD2, {CURVE_5POINT, {100, 90, 80, 90, 100}},   100, 0, MUX_REPLACE},
/* Mixer - Gyro */
    {0x80 | INP_GEAR, 5, 0, {0, {0}}, 30, 0, MUX_REPLACE},
/* Mixer - Collective */
    {INP_THROTTLE,    COLLECTIVE_CH, INP_FMOD0, {CURVE_5POINT, {-30,  -15, 0, 50, 100}},  70, 0, MUX_REPLACE},
    {INP_THROTTLE,    COLLECTIVE_CH, INP_FMOD1, {CURVE_5POINT, {-100, -50, 0, 50, 100}},  70, 0, MUX_REPLACE},
    {INP_THROTTLE,    COLLECTIVE_CH, INP_FMOD2, {CURVE_5POINT, {-100, -50, 0, 50, 100}},  70, 0, MUX_REPLACE},
  },
  {MIXERTEMPLATE_SIMPLE, MIXERTEMPLATE_SIMPLE, MIXERTEMPLATE_SIMPLE,
   MIXERTEMPLATE_SIMPLE, MIXERTEMPLATE_EXPO_DR, MIXERTEMPLATE_SIMPLE, 0, 0, 0, MIXERTEMPLATE_EXPO_DR,}
};

const struct Trim trims[] = {
    {INP_ELEVATOR, BUT_TRIM1_POS, BUT_TRIM1_NEG, 10},
    {INP_THROTTLE, BUT_TRIM2_POS, BUT_TRIM2_NEG, 10},
    {INP_RUDDER,   BUT_TRIM3_POS, BUT_TRIM3_NEG, 10},
    {INP_AILERON,  BUT_TRIM4_POS, BUT_TRIM4_NEG, 10},
};

/* Because '\0' is a string terminator, the map is indexed from 1 */
void adjust_for_protocol()
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
    MIX_SetMixers(NULL, 0);
}

void simple_template() {

    memcpy(Model.mixers, simple.mix, sizeof(simple.mix));
    memcpy(Model.template, simple.template, sizeof(simple.template));
    Model.num_channels = 4;
    adjust_for_protocol();
}
void _4chdr_template() {

    memcpy(Model.mixers, _4chdr.mix, sizeof(_4chdr.mix));
    memcpy(Model.template, _4chdr.template, sizeof(_4chdr.template));
    Model.num_channels = 4;
    adjust_for_protocol();
}

void heli_6ch_template() {
    memcpy(Model.mixers, heli6ch.mix, sizeof(heli6ch.mix));
    memcpy(Model.template, heli6ch.template, sizeof(heli6ch.template));
    Model.num_channels = 6;
    Model.collective_source = COLLECTIVE_CH + NUM_INPUTS + 1;
    Model.swash_type = SWASH_TYPE_120;
    Model.swash_invert = 0;
    Model.type = 0;
}

void TEMPLATE_Apply(enum Templates tmpl)
{
    int i;
    /* Initialize */
    memset(Model.mixers,   0, sizeof(Model.mixers));
    memset(Model.template, 0, sizeof(Model.template));
    memset(Model.limits,   0, sizeof(Model.limits));
    memset(Model.trims,    0, sizeof(Model.trims));
    memcpy(Model.trims, trims, sizeof(trims));

    for(i = 0; i < NUM_CHANNELS; i++) {
        Model.limits[i].max = 100;
        Model.limits[i].min = -100;
    }

    Model.swash_type = SWASH_TYPE_NONE;
    Model.swash_invert = 0;

    switch(tmpl) {
        case TEMPLATE_4CH_SIMPLE: simple_template(); break;
        case TEMPLATE_4CH_DR:     _4chdr_template(); break;
        case TEMPLATE_6CH_HELI:   heli_6ch_template(); break;
        default: break;
    }
    MIX_RegisterTrimButtons();
}
