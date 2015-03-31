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

s32 bind_time = 0;
#define PROTODEF(proto, module, map, cmd, name) name,
const char * const ProtocolNames[PROTOCOL_COUNT] = {
    "None",
    #include "../protocol/protocol.h"
};
#undef PROTODEF

static int get_module(int idx)
{
    int m = TX_MODULE_LAST;
    #define PROTODEF(proto, module, map, cmd, name) case proto: m = module; break;
    switch(idx) {
        #include "../protocol/protocol.h"
    }
    #undef PROTODEF
    return m;
}

const void *PROTO_Cmds(enum ProtoCmds arg)
{
    if (Model.protocol == PROTOCOL_NONE)
        return NULL;
    #define PROTODEF(proto, module, map, cmd, name) case proto: return cmd(arg);
    switch (Model.protocol) {
        #include "../protocol/protocol.h"
    }
    #undef PROTODEF
    return NULL;
}
const char **PROTOCOL_GetOptions()
{
    return (const char **)PROTO_Cmds(PROTOCMD_GETOPTIONS);
}

void PROTOCOL_SetOptions()
{
    PROTO_Cmds(PROTOCMD_SETOPTIONS);
}

int PROTOCOL_SetSwitch(int module) {
    return PACTL_SetSwitch(module);
}

int PROTOCOL_DefaultNumChannels() {
    int num_channels = MAX_PPM_IN_CHANNELS;
    if(Model.protocol != PROTOCOL_NONE)
        num_channels = (unsigned long)PROTO_Cmds(PROTOCMD_DEFAULT_NUMCHAN);
    if (num_channels > MAX_PPM_IN_CHANNELS)
        num_channels = MAX_PPM_IN_CHANNELS;
    return num_channels;
}

int PROTOCOL_NumChannels() {
    int num_channels = MAX_PPM_IN_CHANNELS;
    if(Model.protocol != PROTOCOL_NONE)
        num_channels = (unsigned long)PROTO_Cmds(PROTOCMD_NUMCHAN);
    if (num_channels > MAX_PPM_IN_CHANNELS)
        num_channels = MAX_PPM_IN_CHANNELS;
    return num_channels;
}

u8 PROTOCOL_AutoBindEnabled() {
    u8 binding = 0;
    if(Model.protocol == PROTOCOL_NONE)
        binding = 1;
    else
        binding = (unsigned long)PROTO_Cmds(PROTOCMD_CHECK_AUTOBIND);
    return binding;
}

void PROTOCOL_SetBindState(u32 msec) {
    if (msec == 0xFFFFFFFF) {
        bind_time = 1;
    } else {
        bind_time = msec / 1000;
    }
}
int PROTOCOL_SticksMoved(int init) {
    //FIXME;
    return 0;
}

void PROTOCOL_Bind()
{
    PROTO_Cmds(PROTOCMD_BIND);
}

void PROTOCOL_Deinit()
{
    CLOCK_StopTimer();
    PACTL_SetTxRxMode(TXRX_OFF);
    PROTO_Cmds(PROTOCMD_DEINIT);
}

void PROTOCOL_Init(u8 force)
{
    (void)force;
    PACTL_SetSwitch(get_module(Model.protocol));
    PROTO_Cmds(PROTOCMD_INIT);
}
