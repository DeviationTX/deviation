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
#include "datalog.h"
#include <stdlib.h>
extern const u8 const EATRG[PROTO_MAP_LEN];

static void _map_inp(unsigned *chmap, u8 *val, int offset)
{
    int i;
    for(i = 0; i < PROTO_MAP_LEN; i++) {
        if (*val == i + offset) {
            *val = chmap[i] + offset;
            return;
        }
    }
}
static void map_inp(unsigned *chmap, u8 *val)
{
    _map_inp(chmap, val, NUM_INPUTS + 1);
}
void RemapChannelsForProtocol(const u8 *oldmap)
{
    int i, j;
    const u8 *map = ProtocolChannelMap[Model.protocol];
    unsigned chmap[PROTO_MAP_LEN];
    //Automap assumes input is EATRG
    if(! oldmap)
        oldmap = EATRG;
    if(! map || map == oldmap)
        return;
    for(i = 0; i < PROTO_MAP_LEN; i++) {
        for (j= 0; j < PROTO_MAP_LEN; j++) {
            if (oldmap[i] == map[j]) {
                chmap[i] = j;
                break;
            }
        }
    }
    //Map mixers
    for(i = 0; i < NUM_MIXERS; i++) {
        if (! Model.mixers[i].src)
            break;
        for(j = 0; j < PROTO_MAP_LEN; j++) {
            if(Model.mixers[i].dest == j) {
                Model.mixers[i].dest = chmap[j];
                break;
            }
        }
    }
    u8 template[NUM_CHANNELS];
    u8 safety[NUM_SOURCES];
    struct Limit limit[NUM_OUT_CHANNELS];
    memcpy(template, Model.templates, sizeof(template));
    memcpy(limit, Model.limits, sizeof(limit));
    memcpy(safety, Model.safety, sizeof(safety));
#if HAS_DATALOG
    struct datalog datalog;
    memcpy(&datalog, &Model.datalog, sizeof(datalog));
#endif
    for(j = 0; j < PROTO_MAP_LEN; j++) {
        //Map mixer-templates
        Model.templates[j] = template[chmap[j]];
        //Map limits
        Model.limits[j]    = limit[chmap[j]];
        //Map safety
        Model.safety[j + NUM_INPUTS + 1] = safety[chmap[j] + NUM_INPUTS+1];
#if HAS_DATALOG
        //Map datalog
        DATALOG_ApplyMask(j + DLOG_CHANNELS,
                          datalog.source[DATALOG_BYTE(chmap[j] + DLOG_CHANNELS)] & (1  << DATALOG_POS(chmap[j] + DLOG_CHANNELS)));
#endif
    }
    //Map trims
    for(i = 0; i < NUM_TRIMS; i++) {
        map_inp(chmap, &Model.trims[i].src);
        map_inp(chmap, &Model.trims[i].sw);
    }
    //Map timers
    for(i = 0; i < NUM_TIMERS; i++) {
        map_inp(chmap, &Model.timer[i].src);
        map_inp(chmap, &Model.timer[i].resetsrc);
    }
    //Map display
    for(i = 0; i < NUM_ELEMS; i++) {
        switch(ELEM_TYPE(Model.pagecfg2.elem[i])) {
            case ELEM_SMALLBOX:
            case ELEM_BIGBOX:
                _map_inp(chmap, &Model.pagecfg2.elem[i].src, NUM_TELEM + NUM_TIMERS + NUM_RTC+1);
                break;
            case ELEM_BAR:
                _map_inp(chmap, &Model.pagecfg2.elem[i].src, 1);
                break;
            case ELEM_TOGGLE:
                map_inp(chmap, &Model.pagecfg2.elem[i].src);
                break;
        }
    }
    MIXER_SetMixers(NULL, 0);
}
