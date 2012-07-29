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
    for(i = 0; i < NUM_MIXERS; i++) {
        if (! Model.mixers[i].src)
            return;
        for(ch = 0; ch < PROTO_MAP_LEN; ch++)
            if (map[ch] == Model.mixers[i].src)
                Model.mixers[i].dest = ch;
    }
}

void simple_template() {

    memcpy(Model.mixers, simple.mix, sizeof(simple.mix));
    memcpy(Model.template, simple.template, sizeof(simple.template));
    Model.num_channels = 4;
}
void _4chdr_template() {

    memcpy(Model.mixers, _4chdr.mix, sizeof(_4chdr.mix));
    memcpy(Model.template, _4chdr.template, sizeof(_4chdr.template));
    Model.num_channels = 4;
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
        default: break;
    }
    adjust_for_protocol();
    MIX_RegisterTrimButtons();
}
