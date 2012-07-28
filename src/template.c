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

const struct Trim trims[] = {
    {INP_ELEVATOR, BUT_TRIM1_POS, BUT_TRIM1_NEG, 10},
    {INP_THROTTLE, BUT_TRIM2_POS, BUT_TRIM2_NEG, 10},
    {INP_RUDDER,   BUT_TRIM3_POS, BUT_TRIM3_NEG, 10},
    {INP_AILERON,  BUT_TRIM4_POS, BUT_TRIM4_NEG, 10},
};

/* Because '\0' is a string terminator, the map is indexed from 1 */
const char * const ProtocolChannelMap[] = {
    "", /* PROTOCOL_NONE */
#ifdef PROTO_HAS_CYRF6936
    /* PROTOCOL_DEVO */
    "\x01\x02\x03\x04",
    /* PROTOCOL_WK2801 */
    "\x01\x02\x03\x04",
    /* PROTOCOL_WK2601 */
    "\x01\x02\x03\x04",
    /* PROTOCOL_WK2401 */
    "\x01\x02\x03\x04",
    /* PROTOCOL_DSM2 */
    "\x01\x02\x03\x04",
    /* PROTOCOL_J6PRO */
    "\x01\x02\x03\x04",
#endif
#ifdef PROTO_HAS_A7105
    /* PROTOCOL_FLYSKY */
    "\x01\x02\x03\x04",
#endif
};

void adjust_for_protocol()
{
    const char *map = ProtocolChannelMap[Model.protocol];
    u8 len = strlen(map);
    int i;
    for(i = 0; i < NUM_MIXERS; i++) {
        if (! Model.mixers[i].src)
            return;
        u8 src = Model.mixers[i].src;
        if (src < len)
            Model.mixers[i].src = map[Model.mixers[i].src] - 1;
    }
}

void simple_template() {

    memcpy(Model.mixers, simple.mix, sizeof(simple.mix));
    memcpy(Model.template, simple.template, sizeof(simple.template));
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
        default: break;
    }
    adjust_for_protocol();
    MIX_RegisterTrimButtons();
}
