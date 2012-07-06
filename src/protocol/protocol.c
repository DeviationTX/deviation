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

void PROTOCOL_Init(enum Protocols p)
{
    switch(p) {
        case PROTOCOL_FLYSKY:
            #ifdef PROTO_HAS_A7105
            FLYSKY_Initialize();
            #endif
            break;
        case PROTOCOL_DEVO:
            #ifdef PROTO_HAS_CYRF6936
            DEVO_Initialize();
            #endif
            break;
        case PROTOCOL_DSM2:
            #ifdef PROTO_HAS_CYRF6936
            DSM2_Initialize();
            #endif
            break;
        case PROTOCOL_J6PRO:
            #ifdef PROTO_HAS_CYRF6936
            J6PRO_Initialize();
            #endif
            break;
    }
}

void PROTOCOL_DeInit()
{
    CLOCK_StopTimer();
}
