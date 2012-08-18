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
#include "interface.h"
#include "config/model.h"

static const u8 const _eatrg[PROTO_MAP_LEN] = ORDER_EATRG;
static const u8 const _taerg[PROTO_MAP_LEN] = ORDER_EATRG;
u8 proto_ready;

const u8 *ProtocolChannelMap[PROTOCOL_COUNT] = {
    NULL,
#ifdef PROTO_HAS_CYRF6936
    _eatrg, //DEVO
    _eatrg, //2801
    _eatrg, //2601
    _eatrg, //2401
    _taerg, //DSM2
    _eatrg, //J6PRO
#endif
#ifdef PROTO_HAS_A7105
    _eatrg, //Flysky
#endif
};

void PROTOCOL_Init(u8 force)
{
    if (! force && PROTOCOL_CheckSafe()) {
        proto_ready = 0;
        return;
    }
    proto_ready = 1;
    switch(Model.protocol) {
        #ifdef PROTO_HAS_A7105
        case PROTOCOL_FLYSKY:
            FLYSKY_Initialize();
            break;
        #endif
        #ifdef PROTO_HAS_CYRF6936
        case PROTOCOL_DEVO:
            DEVO_Initialize();
            break;
        case PROTOCOL_WK2801:
        case PROTOCOL_WK2601:
        case PROTOCOL_WK2401:
            WK2x01_Initialize();
            break;
        case PROTOCOL_DSM2:
            DSM2_Initialize();
            break;
        case PROTOCOL_J6PRO:
            J6PRO_Initialize();
            break;
        #endif
        case PROTOCOL_NONE:
        default:
            CLOCK_StopTimer();
            break;
    }
}

void PROTOCOL_DeInit()
{
    CLOCK_StopTimer();
    proto_ready = 1;
}

u8 PROTOCOL_WaitingForSafe()
{
    return ! proto_ready;
}

u32 PROTOCOL_CheckSafe()
{
    int i;
    s16 *raw = MIXER_GetInputs();
    u32 unsafe = 0;
    for(i = 0; i < NUM_INPUTS + NUM_CHANNELS; i++) {
        if (! Model.safety[i])
            continue;
        s16 val = RANGE_TO_PCT((i < NUM_INPUTS)
                      ? raw[i+1]
                      : MIXER_GetChannel(i - (NUM_INPUTS), APPLY_SAFETY));
        if (Model.safety[i] == SAFE_MIN && val > -99)
            unsafe |= 1 << i;
        else if (Model.safety[i] == SAFE_ZERO && (val < -1 || val > 1))
            unsafe |= 1 << i;
        else if (Model.safety[i] == SAFE_MAX && val < 99)
            unsafe |= 1 << i;
    }
    return unsafe;
}

