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

#include "common.h"
#include "config/model.h"
#include "protocol/interface.h"

#ifdef BUILDTYPE_DEV
const void * (*PROTO_Cmds)(enum ProtoCmds) = TESTRF_Cmds;
#else
const void * (*PROTO_Cmds)(enum ProtoCmds) = NULL;
#endif

#define PROTODEF(proto, module, map, cmd, name) name,
const char * const ProtocolNames[PROTOCOL_COUNT] = {
    "None",
    #include "../protocol/protocol.h"
};
#undef PROTODEF
const char **PROTOCOL_GetOptions()
{
    const char **data = NULL;
    if(Model.protocol != PROTOCOL_NONE)
        data = (const char **)PROTO_Cmds(PROTOCMD_GETOPTIONS);
    return data;
}

void PROTOCOL_SetOptions()
{
    if(Model.protocol != PROTOCOL_NONE)
        PROTO_Cmds(PROTOCMD_SETOPTIONS);
}

int PROTOCOL_SetSwitch(int module) {
    return PACTL_SetSwitch(module);
}
