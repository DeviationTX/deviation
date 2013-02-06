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
#include "music.h"
#include "interface.h"
#include "config/model.h"

//Not static because we need it in mixer.c
const u8 const EATRG[PROTO_MAP_LEN] =
    { INP_ELEVATOR, INP_AILERON, INP_THROTTLE, INP_RUDDER, INP_GEAR1 };
static const u8 const TAERG[PROTO_MAP_LEN] = 
    { INP_THROTTLE, INP_AILERON, INP_ELEVATOR, INP_RUDDER, INP_GEAR1 };
static const u8 const AETRG[PROTO_MAP_LEN] = 
    { INP_AILERON, INP_ELEVATOR, INP_THROTTLE, INP_RUDDER, INP_GEAR1 };

static u8 proto_state;
static u32 bind_time;
#define PROTO_READY   0x01
#define PROTO_BINDING 0x02
#define PROTO_BINDDLG 0x04

#define PROTODEF(proto, map, cmd, name) map,
const u8 *ProtocolChannelMap[PROTOCOL_COUNT] = {
    NULL,
    #include "protocol.h"
};
#undef PROTODEF
#ifdef MODULAR
unsigned long * const loaded_protocol = (unsigned long *)MODULAR;
void * (* const PROTO_Cmds)(enum ProtoCmds) = (void *)(MODULAR +sizeof(long)+1);
#endif

#define PROTODEF(proto, map, cmd, name) name,
const char * const ProtocolNames[PROTOCOL_COUNT] = {
    "None",
    #include "protocol.h"
};
#undef PROTODEF

static void PROTOCOL_Load();
void PROTOCOL_Init(u8 force)
{
    PROTOCOL_DeInit();
    PROTOCOL_Load(Model.protocol);
    proto_state = 0;
    if (! force && PROTOCOL_CheckSafe()) {
        return;
    }
    proto_state |= PROTO_READY;

#ifdef MODULAR
    if(Model.protocol == PROTOCOL_NONE || *loaded_protocol != Model.protocol)
        CLOCK_StopTimer();
    else
        PROTO_Cmds(PROTOCMD_INIT);
#else
    #define PROTODEF(proto, map, cmd, name) case proto: cmd(PROTOCMD_INIT); break;
    switch(Model.protocol) {
        #include "protocol.h"
        case PROTOCOL_NONE:
        default:
            CLOCK_StopTimer();
            break;
    }
    #undef PROTODEF
#endif
}

void PROTOCOL_DeInit()
{
    CLOCK_StopTimer();
#ifdef MODULAR
    if(Model.protocol != PROTOCOL_NONE && *loaded_protocol == Model.protocol)
        PROTO_Cmds(PROTOCMD_DEINIT);
#else
    #define PROTODEF(proto, map, cmd, name) case proto: cmd(PROTOCMD_DEINIT); break;
    switch(Model.protocol) {
        #include "protocol.h"
        case PROTOCOL_NONE:
        default: break;
    }
    #undef PROTODEF
#endif
    proto_state = PROTO_READY;
}

/*This symbol is exported bythe linker*/
extern unsigned _data_loadaddr;
void PROTOCOL_Load()
{
#ifdef MODULAR
    if(*loaded_protocol == Model.protocol)
        return;
    char file[25];
    #define PROTODEF(proto, map, cmd, name) case proto: sprintf(file,"protocol/%s.mod", name); break;
    switch(Model.protocol) {
        #include "protocol.h"
        default: *loaded_protocol = 0; return;
    }
    #undef PROTODEF
    FILE *fh;
    fh = fopen(file, "r");
    //printf("Loading %s: %08lx\n", file, fh);
    if(! fh) {
	PAGE_ShowInvalidModule();
        return;
    }
    setbuf(fh, 0);
    int size = 0;
    unsigned char buf[256];
    int len;
    char *ptr = (char *)loaded_protocol;
    while(size < 4096) {
        len = fread(buf, 1, 256, fh);
        if(len) {
            memcpy(ptr, buf, len);
            ptr += len;
        }
        size += len;
        if (len != 256)
            break;
    }
    fclose(fh);
    if ((unsigned long)&_data_loadaddr != *loaded_protocol) {
	PAGE_ShowInvalidModule();
        *loaded_protocol = 0;
        return;
    }
    //printf("Updated %d (%d) bytes: Data: %08lx %08lx %08lx\n", size, len, *loaded_protocol, *(loaded_protocol+1), *(loaded_protocol+2));
    //We use the same file for multiple protocols, so we need to manually set this here
    *loaded_protocol = Model.protocol;
#endif
}
 
u8 PROTOCOL_WaitingForSafe()
{
    return (proto_state & PROTO_READY) ? 0 : 1;
}

u32 PROTOCOL_Binding()
{

    if (proto_state & PROTO_BINDING) {
        if (bind_time == 0xFFFFFFFF)
            return bind_time;
        s32 tmp = bind_time - CLOCK_getms();
        return tmp > 0 ? tmp : 1;
    }
    return 0;
}

void PROTOCOL_SetBindState(u32 msec)
{
    if (msec) {
        if (msec == 0xFFFFFFFF)
            bind_time = msec;
        else
            bind_time = CLOCK_getms() + msec;
        proto_state |= PROTO_BINDING;
    } else {
        proto_state &= ~PROTO_BINDING;
    }
}

int PROTOCOL_MapChannel(int input, int default_ch)
{
    int i;
    if (ProtocolChannelMap[Model.protocol]) {
        for(i = 0; i < PROTO_MAP_LEN; i++) {
            if (ProtocolChannelMap[Model.protocol][i] == input) {
                default_ch = NUM_INPUTS + i;
                break;
            }
        }
    }
    return default_ch;
}

u64 PROTOCOL_CheckSafe()
{
    int i;
    volatile s16 *raw = MIXER_GetInputs();
    u64 unsafe = 0;
    for(i = 0; i < NUM_SOURCES + 1; i++) {
        if (! Model.safety[i])
            continue;
        int ch;
        if (i == 0) {
            //Auto mode (choose 'THR' channel (or CH3)
            ch = PROTOCOL_MapChannel(INP_THROTTLE, NUM_INPUTS + 2);
        } else {
            ch = i-1;
        }
        s16 val = RANGE_TO_PCT((ch < NUM_INPUTS)
                      ? raw[ch+1]
                      : MIXER_GetChannel(ch - (NUM_INPUTS), APPLY_SAFETY));
        if (Model.safety[i] == SAFE_MIN && val > -99)
            unsafe |= 1LL << i;
        else if (Model.safety[i] == SAFE_ZERO && (val < -1 || val > 1))
            unsafe |= 1LL << i;
        else if (Model.safety[i] == SAFE_MAX && val < 99)
            unsafe |= 1LL << i;
    }
    return unsafe;
}

u8 PROTOCOL_AutoBindEnabled()
{
    u8 binding = 0;
#ifdef MODULAR
    if(Model.protocol == PROTOCOL_NONE || *loaded_protocol != Model.protocol)
        binding = 1;
    else
        binding = (unsigned long)PROTO_Cmds(PROTOCMD_CHECK_AUTOBIND);
#else
    #define PROTODEF(proto, map, cmd, name) case proto: binding = (unsigned long)cmd(PROTOCMD_CHECK_AUTOBIND); break;
    switch(Model.protocol) {
        #include "protocol.h"
        case PROTOCOL_NONE:
        default:
            binding = 1;
    }
    #undef PROTODEF
#endif
    return binding;
}

void PROTOCOL_Bind()
{
#ifdef MODULAR
    if(Model.protocol != PROTOCOL_NONE && *loaded_protocol == Model.protocol)
        PROTO_Cmds(PROTOCMD_BIND);
#else
    #define PROTODEF(proto, map, cmd, name) case proto: cmd(PROTOCMD_BIND); break;
    switch(Model.protocol) {
        #include "protocol.h"
        case PROTOCOL_NONE:
        default: break;
    }
    #undef PROTODEF
#endif
}

int PROTOCOL_NumChannels()
{
    int num_channels = NUM_OUT_CHANNELS;
#ifdef MODULAR
    if(Model.protocol != PROTOCOL_NONE && *loaded_protocol == Model.protocol)
        num_channels = (unsigned long)PROTO_Cmds(PROTOCMD_NUMCHAN);
#else
    #define PROTODEF(proto, map, cmd, name) case proto: num_channels = (unsigned long)cmd(PROTOCMD_NUMCHAN); break;
    switch(Model.protocol) {
        #include "protocol.h"
        case PROTOCOL_NONE:
        default: break;
    }
    #undef PROTODEF
#endif
    if (num_channels > NUM_OUT_CHANNELS)
        num_channels = NUM_OUT_CHANNELS;
    return num_channels;
}

int PROTOCOL_DefaultNumChannels()
{
    int num_channels = NUM_OUT_CHANNELS;
#ifdef MODULAR
    if(Model.protocol != PROTOCOL_NONE && *loaded_protocol == Model.protocol)
        num_channels = (unsigned long)PROTO_Cmds(PROTOCMD_DEFAULT_NUMCHAN);
#else
    #define PROTODEF(proto, map, cmd, name) case proto: num_channels = (unsigned long)cmd(PROTOCMD_DEFAULT_NUMCHAN); break;
    switch(Model.protocol) {
        #include "protocol.h"
        case PROTOCOL_NONE:
        default: break;
    }
    #undef PROTODEF
#endif
    if (num_channels > NUM_OUT_CHANNELS)
        num_channels = NUM_OUT_CHANNELS;
    return num_channels;
}
u32 PROTOCOL_CurrentID()
{
    u32 id = 0;
#ifdef MODULAR
    if(Model.protocol != PROTOCOL_NONE && *loaded_protocol == Model.protocol)
        id = (unsigned long)PROTO_Cmds(PROTOCMD_CURRENT_ID);
#else
    #define PROTODEF(proto, map, cmd, name) case proto: id = (unsigned long)cmd(PROTOCMD_CURRENT_ID); break;
    switch(Model.protocol) {
        #include "protocol.h"
        case PROTOCOL_NONE:
        default:
            id = 0;
    }
    #undef PROTODEF
#endif
    return id;
}

const char **PROTOCOL_GetOptions()
{
    const char **data = NULL;
#ifdef MODULAR
    if(Model.protocol != PROTOCOL_NONE && *loaded_protocol == Model.protocol)
        data = (const char **)PROTO_Cmds(PROTOCMD_GETOPTIONS);
#else
    #define PROTODEF(proto, map, cmd, name) case proto: data = (const char **)cmd(PROTOCMD_GETOPTIONS); break;
    switch(Model.protocol) {
        #include "protocol.h"
        case PROTOCOL_NONE:
        default:
            data = NULL;
    }
    #undef PROTODEF
#endif
    return data;
}

void PROTOCOL_SetOptions()
{
#ifdef MODULAR
    if(Model.protocol != PROTOCOL_NONE && *loaded_protocol == Model.protocol)
        PROTO_Cmds(PROTOCMD_SETOPTIONS);
#else
    #define PROTODEF(proto, map, cmd, name) case proto: cmd(PROTOCMD_SETOPTIONS); break;
    switch(Model.protocol) {
        #include "protocol.h"
        case PROTOCOL_NONE:
        default: break;
    }
    #undef PROTODEF
#endif
}

s8 PROTOCOL_GetTelemetryState()
{
    s8 telem_state=  -1;  // -1 means not support
#ifdef MODULAR
    if(Model.protocol != PROTOCOL_NONE && *loaded_protocol == Model.protocol)
        telem_state = (long)PROTO_Cmds(PROTOCMD_TELEMETRYSTATE);
#else
    #define PROTODEF(proto, map, cmd, name) case proto: telem_state = (long)cmd(PROTOCMD_TELEMETRYSTATE); break;
    switch(Model.protocol) {
        #include "protocol.h"
        case PROTOCOL_NONE:
        default:
            telem_state = -1;
    }
    #undef PROTODEF
#endif
    return telem_state;
}

void PROTOCOL_CheckDialogs()
{
    if (PROTOCOL_WaitingForSafe()) {
        PAGE_ShowSafetyDialog();
    } else {
        if (PROTOCOL_Binding()) {
            PAGE_ShowBindingDialog(proto_state & PROTO_BINDDLG);
            proto_state |= PROTO_BINDDLG;
        } else if (proto_state & PROTO_BINDDLG) {
            PAGE_CloseBindingDialog();
            MUSIC_Play(MUSIC_DONE_BINDING);
            proto_state &= ~PROTO_BINDDLG;
        }
    }
}
