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
#include "config/tx.h"

extern struct FAT FontFAT; //defined in screen/lcd_string.c

//Not static because we need it in mixer.c
const u8 const EATRG[PROTO_MAP_LEN] =
    { INP_ELEVATOR, INP_AILERON, INP_THROTTLE, INP_RUDDER, INP_GEAR1 };
static const u8 const TAERG[PROTO_MAP_LEN] = 
    { INP_THROTTLE, INP_AILERON, INP_ELEVATOR, INP_RUDDER, INP_GEAR1 };
static const u8 const AETRG[PROTO_MAP_LEN] = 
    { INP_AILERON, INP_ELEVATOR, INP_THROTTLE, INP_RUDDER, INP_GEAR1 };

static u8 proto_state;
static u32 bind_time;
#define PROTO_DEINIT  0x00
#define PROTO_INIT    0x01
#define PROTO_READY   0x02
#define PROTO_BINDING 0x04
#define PROTO_BINDDLG 0x08

#define PROTODEF(proto, module, map, cmd, name) map,
const u8 *ProtocolChannelMap[PROTOCOL_COUNT] = {
    NULL,
    #include "protocol.h"
};
#undef PROTODEF
#ifdef MODULAR
unsigned long * const loaded_protocol = (unsigned long *)MODULAR;
void * (* const PROTO_Cmds)(enum ProtoCmds) = (void *)(MODULAR +sizeof(long)+1);
#define PROTOCOL_LOADED (*loaded_protocol == Model.protocol)
#else
const void * (*PROTO_Cmds)(enum ProtoCmds) = NULL;
#define PROTOCOL_LOADED PROTO_Cmds
#endif

#define PROTODEF(proto, module, map, cmd, name) name,
const char * const ProtocolNames[PROTOCOL_COUNT] = {
    "None",
    #include "protocol.h"
};
#undef PROTODEF

void PROTOCOL_Init(u8 force)
{
    PROTOCOL_DeInit();
    PROTOCOL_Load(0);
    proto_state = PROTO_INIT;
    if (! force && PROTOCOL_CheckSafe()) {
        return;
    }
    proto_state |= PROTO_READY;

    if(Model.protocol == PROTOCOL_NONE || ! PROTOCOL_LOADED)
        CLOCK_StopTimer();
    else
        PROTO_Cmds(PROTOCMD_INIT);
}

void PROTOCOL_DeInit()
{
    CLOCK_StopTimer();
    if(Model.protocol != PROTOCOL_NONE && PROTOCOL_LOADED)
        PROTO_Cmds(PROTOCMD_DEINIT);
    proto_state = PROTO_DEINIT;
}

/*This symbol is exported bythe linker*/
extern unsigned _data_loadaddr;
void PROTOCOL_Load(int no_dlg)
{
    (void)no_dlg;
#ifdef MODULAR
    if(! PROTOCOL_HasModule(Model.protocol)) {
        *loaded_protocol = 0;
        return;
    }
    if(*loaded_protocol == Model.protocol)
        return;
    char file[25];
    strcpy(file, "protocol/");
    #define PROTODEF(proto, module, map, cmd, name) case proto: strcat(file,name); break;
    switch(Model.protocol) {
        #include "protocol.h"
        default: *loaded_protocol = 0; return;
    }
    #undef PROTODEF
    file[17] = '\0'; //truncate filename to 8 characters
    strcat(file, ".mod");
    FILE *fh;
    //We close the current font because on the dveo8 we reuse
    //the font filehandle to read the protocol.
    //Thatis necessary because we need to be able to load the
    //protocol while an ini file is open, and we don't want to
    //waste the RAM for an extra filehandle
    u8 old_font = LCD_SetFont(0);
    finit(&FontFAT, ""); //In case no fonts are loaded yet
    fh = fopen2(&FontFAT, file, "r");
    //printf("Loading %s: %08lx\n", file, fh);
    if(! fh) {
        if(! no_dlg)
            PAGE_ShowInvalidModule();
        LCD_SetFont(old_font);
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
    LCD_SetFont(old_font);
    if ((unsigned long)&_data_loadaddr != *loaded_protocol) {
        if(! no_dlg)
            PAGE_ShowInvalidModule();
        *loaded_protocol = 0;
        return;
    }
    //printf("Updated %d (%d) bytes: Data: %08lx %08lx %08lx\n", size, len, *loaded_protocol, *(loaded_protocol+1), *(loaded_protocol+2));
    //We use the same file for multiple protocols, so we need to manually set this here
    *loaded_protocol = Model.protocol;
#else
    if(! PROTOCOL_HasModule(Model.protocol)) {
        PROTO_Cmds = NULL;
        printf("Module is not defined!\n");
        return;
    }
    #define PROTODEF(proto, module, map, cmd, name) case proto: PROTO_Cmds = cmd; break;
    switch(Model.protocol) {
        #include "protocol.h"
        default: PROTO_Cmds = NULL;
    }
    #undef PROTODEF
#endif
}
 
u8 PROTOCOL_WaitingForSafe()
{
    return ((proto_state & (PROTO_INIT | PROTO_READY)) == PROTO_INIT) ? 1 : 0;
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
    if(Model.protocol == PROTOCOL_NONE || ! PROTOCOL_LOADED)
        binding = 1;
    else
        binding = (unsigned long)PROTO_Cmds(PROTOCMD_CHECK_AUTOBIND);
    return binding;
}

void PROTOCOL_Bind()
{
    if(! (proto_state & PROTO_INIT)) {
        PROTOCOL_Init(0);
    }
    if (! (proto_state & PROTO_READY)) {
        return;
    }
    if(Model.protocol != PROTOCOL_NONE && PROTOCOL_LOADED)
        PROTO_Cmds(PROTOCMD_BIND);
}

int PROTOCOL_NumChannels()
{
    int num_channels = NUM_OUT_CHANNELS;
    if(Model.protocol != PROTOCOL_NONE && PROTOCOL_LOADED)
        num_channels = (unsigned long)PROTO_Cmds(PROTOCMD_NUMCHAN);
    if (num_channels > NUM_OUT_CHANNELS)
        num_channels = NUM_OUT_CHANNELS;
    return num_channels;
}

int PROTOCOL_DefaultNumChannels()
{
    int num_channels = NUM_OUT_CHANNELS;
    if(Model.protocol != PROTOCOL_NONE && PROTOCOL_LOADED)
        num_channels = (unsigned long)PROTO_Cmds(PROTOCMD_DEFAULT_NUMCHAN);
    if (num_channels > NUM_OUT_CHANNELS)
        num_channels = NUM_OUT_CHANNELS;
    return num_channels;
}
u32 PROTOCOL_CurrentID()
{
    u32 id = 0;
    if(Model.protocol != PROTOCOL_NONE && PROTOCOL_LOADED)
        id = (unsigned long)PROTO_Cmds(PROTOCMD_CURRENT_ID);
    return id;
}

const char **PROTOCOL_GetOptions()
{
    const char **data = NULL;
    if(Model.protocol != PROTOCOL_NONE && PROTOCOL_LOADED)
        data = (const char **)PROTO_Cmds(PROTOCMD_GETOPTIONS);
    return data;
}

void PROTOCOL_SetOptions()
{
    if(Model.protocol != PROTOCOL_NONE && PROTOCOL_LOADED)
        PROTO_Cmds(PROTOCMD_SETOPTIONS);
}

s8 PROTOCOL_GetTelemetryState()
{
    s8 telem_state=  -1;  // -1 means not support
    if(Model.protocol != PROTOCOL_NONE && PROTOCOL_LOADED)
        telem_state = (long)PROTO_Cmds(PROTOCMD_TELEMETRYSTATE);
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
static int get_module(int idx)
{
    int m = TX_MODULE_LAST;
    #define PROTODEF(proto, module, map, cmd, name) case proto: m = module; break;
    switch(idx) {
        #include "protocol.h"
    }
    #undef PROTODEF
    return m;
}

int PROTOCOL_HasModule(int idx)
{
    int m = get_module(idx);
    if(m == TX_MODULE_LAST || Transmitter.module_enable[m].port != 0)
        return 1;
    return 0;
}

int PROTOCOL_HasPowerAmp(int idx)
{
    int m = get_module(idx);
    if(m != TX_MODULE_LAST && Transmitter.module_poweramp & (1 << m))
        return 1;
    return 0;
}
