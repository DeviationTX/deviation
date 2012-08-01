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

static const u8 const _eatrg[PROTO_MAP_LEN] = ORDER_EATRG;
static const u8 const _taerg[PROTO_MAP_LEN] = ORDER_EATRG;

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

void PROTOCOL_Init(enum Protocols p)
{
    switch(p) {
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
}
