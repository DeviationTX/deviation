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
#include "protospi.h"

#include <stdlib.h>

#if !defined(HAS_4IN1_FLASH)
#define HAS_4IN1_FLASH 0
#elif defined(EMULATOR)
// Disable for emulator
#undef HAS_4IN1_FLASH
#define HAS_4IN1_FLASH 0
#endif


//Not static because we need it in mixer.c
const u8 EATRG0[PROTO_MAP_LEN] =
    { INP_ELEVATOR, INP_AILERON, INP_THROTTLE, INP_RUDDER, INP_GEAR1 };
static const u8 TAERG0[PROTO_MAP_LEN] =
    { INP_THROTTLE, INP_AILERON, INP_ELEVATOR, INP_RUDDER, INP_GEAR1 };
static const u8 AETRG0[PROTO_MAP_LEN] =
    { INP_AILERON, INP_ELEVATOR, INP_THROTTLE, INP_RUDDER, INP_GEAR1 };
const u8 * CurrentProtocolChannelMap;

static u8 proto_state;
static u32 bind_time;
#define PROTO_DEINIT    0x00
#define PROTO_INIT      0x01
#define PROTO_READY     0x02
#define PROTO_BINDING   0x04
#define PROTO_BINDDLG   0x08
#define PROTO_MODULEDLG 0x10

#ifdef ENABLE_MODULAR
unsigned long * const loaded_protocol = (unsigned long *)ENABLE_MODULAR;
uintptr_t (* const PROTO_Cmds)(enum ProtoCmds) = (void *)(ENABLE_MODULAR + sizeof(unsigned) + 1);
#define PROTOCOL_LOADED (*loaded_protocol == Model.protocol)
#else
uintptr_t (*PROTO_Cmds)(enum ProtoCmds) = NULL;
#define PROTOCOL_LOADED PROTO_Cmds
#endif

#ifdef ENABLE_MODULAR
#define PROTODEF(proto, module, map, cmd, name) {module, name},
const struct{
    int module;
    const char* name;
}Protocols[PROTOCOL_COUNT] = {
    { 0, "None" },
    #include "protocol.h"
};
#undef PROTODEF
#else
#define PROTODEF(proto, module, map, cmd, name) {module, cmd, name},
const struct{
    enum Radio module;
    uintptr_t (*cmd)(enum ProtoCmds);
    const char* name;
}Protocols[PROTOCOL_COUNT] = {
    { 0, NULL, "None"},
    #include "protocol.h"
};
#undef PROTODEF
#endif

static int get_module(u16 idx);

const char * PROTOCOL_GetName(u16 idx)
{
    if (idx > PROTOCOL_COUNT)
        return _tr("None");
    else
        return Protocols[idx].name;
}

enum Radio PROTOCOL_GetRadio(u16 idx) {
    return (enum Radio)get_module(idx);
}

const char * PROTOCOL_Name()
{
    return PROTOCOL_GetName(Model.protocol);
}

void PROTOCOL_Init(u8 force)
{
    if(! force && (proto_state & PROTO_MODULEDLG))
        return;
    PROTOCOL_DeInit();
    PROTOCOL_Load(0);
    proto_state = PROTO_INIT;
    if (! force && PROTOCOL_CheckSafe()) {
        return;
    }
    proto_state |= PROTO_READY;

    if(Model.protocol == PROTOCOL_NONE || ! PROTOCOL_LOADED)
        CLOCK_StopTimer();
    else {
        CLOCK_StartMixer(); // enable mixer updates on timer
        PROTO_Cmds(PROTOCMD_INIT);
    }
}

void PROTOCOL_DeInit()
{
    CLOCK_StopTimer();
    if(Model.protocol != PROTOCOL_NONE && PROTOCOL_LOADED)
        PROTO_Cmds(PROTOCMD_DEINIT);
    CLOCK_StartMixer(); // run mixer on timer so channels are updated for things like calibration
    proto_state = PROTO_DEINIT;
}

/*This symbol is exported bythe linker*/
extern unsigned _data_loadaddr;
void PROTOCOL_Load(int no_dlg)
{
    (void)no_dlg;
#ifdef ENABLE_MODULAR
    FATFS ModuleFAT;
    FILE *fh;

    if(! PROTOCOL_HasModule(Model.protocol)) {
        *loaded_protocol = 0;
        return;
    }
    if(*loaded_protocol == Model.protocol)
        return;
    char file[25];
    strcpy(file, "protocol/");

    if (Model.protocol > PROTOCOL_COUNT)
    {
        *loaded_protocol = 0;
        return;
    }
    else
    {
        strcat(file, Protocols[Model.protocol].name);
    }
    file[17] = '\0'; //truncate filename to 8 characters
    strcat(file, ".mod");

    memset(&ModuleFAT, 0, sizeof(ModuleFAT));
    finit(&ModuleFAT, "protocol");
    fh = fopen2(&ModuleFAT, file, "r");
    //printf("Loading %s: %08lx\n", file, fh);
    if(! fh) {
        if(! no_dlg) {
            sprintf(tempstring, "Misisng protocol:\n%s", file);
            PAGE_ShowWarning(NULL, tempstring);
        }
        return;
    }
    setbuf(fh, 0);
    fread(loaded_protocol, 1, 4 * 1024, fh);
    fclose(fh);
    if ((unsigned long)&_data_loadaddr != *loaded_protocol) {
        if(! no_dlg) {
            sprintf(tempstring, "Protocol Mismatch:\n%08x\n%08x", (unsigned long)&_data_loadaddr, *loaded_protocol);
            PAGE_ShowWarning(NULL, tempstring);
        }
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
    PROTO_Cmds = Protocols[Model.protocol].cmd;
#endif
    PROTOCOL_SetSwitch(get_module(Model.protocol));
    if (PROTOCOL_GetTelemetryState() != PROTO_TELEM_UNSUPPORTED) {
        memset(&Telemetry, 0, sizeof(Telemetry));
        TELEMETRY_SetType(PROTOCOL_GetTelemetryType());
    }

    CurrentProtocolChannelMap = PROTOCOL_GetChannelMap();
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
        PROTOCOL_SticksMoved(1);  //Initialize Stick position
    } else {
        proto_state &= ~PROTO_BINDING;
    }
}

int PROTOCOL_MapChannel(int input, int default_ch)
{
    int i;
    if (CurrentProtocolChannelMap) {
        for(i = 0; i < PROTO_MAP_LEN; i++) {
            if (CurrentProtocolChannelMap[i] == input) {
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
    volatile s32 *raw = MIXER_GetInputs();
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
        s32 val = RANGE_TO_PCT((ch < NUM_INPUTS)
                      ? raw[ch+1]
                      : MIXER_GetChannel(ch - (NUM_INPUTS), APPLY_SAFETY));
        if (Model.safety[i] == SAFE_MIN && val > -99)
            unsafe |= 1ULL << i;
        else if (Model.safety[i] == SAFE_ZERO && (val < -1 || val > 1))
            unsafe |= 1ULL << i;
        else if (Model.safety[i] == SAFE_MAX && val < 99)
            unsafe |= 1ULL << i;
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

const u8* PROTOCOL_GetChannelMap()
{
    const u8* channelmap = NULL;
    if(Model.protocol != PROTOCOL_NONE && PROTOCOL_LOADED)
    {
        int map = (int)PROTO_Cmds(PROTOCMD_CHANNELMAP);
        switch(map)
        {
            case (int)AETRG:
                channelmap = AETRG0;
                break;
            case (int)UNCHG:
                channelmap = NULL;
                break;
            case (int)TAERG:
                channelmap = TAERG0;
                break;
            case (int)EATRG:
                channelmap = EATRG0;
                break;
            default:
                printf("Unknown channemap: %d\n", map);
        }
    }
    return channelmap;
}

int PROTOCOL_GetTelemetryState()
{
    int telem_state = PROTO_TELEM_UNSUPPORTED;
    if(Model.protocol != PROTOCOL_NONE && PROTOCOL_LOADED)
        telem_state = (long)PROTO_Cmds(PROTOCMD_TELEMETRYSTATE);
    return telem_state;
}

int PROTOCOL_GetTelemetryType()
{
    int telem_type = TELEM_DEVO;
    if(Model.protocol != PROTOCOL_NONE && PROTOCOL_LOADED)
        telem_type = (long)PROTO_Cmds(PROTOCMD_TELEMETRYTYPE);
    return telem_type;
}

void PROTOCOL_ResetTelemetry()
{
    if(Model.protocol != PROTOCOL_NONE && PROTOCOL_LOADED)
        PROTO_Cmds(PROTOCMD_TELEMETRYRESET);
}

void PROTOCOL_CheckDialogs()
{
    if (proto_state & PROTO_MODULEDLG) {
        if(! PAGE_DialogVisible()) {
            //Dialog was dismissed, proceed
            proto_state &= ~PROTO_MODULEDLG;
            PROTOCOL_Init(0);
        }
        return;
    }
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

static int get_module(u16 idx)
{
    if (idx >= PROTOCOL_COUNT)
        return TX_MODULE_LAST;
    else
        return Protocols[idx].module;
}

int PROTOCOL_HasModule(int idx)
{
// If you need to enable hardware.ini [modules] to
// enable/disable individual modules for 4-in-1 flash
// replace the condition with #if 0.
#if HAS_4IN1_FLASH
    if (SPISwitch_Present()) {
        return 1;
    } else {
#endif
    int m = get_module(idx);
    if (m > MULTIMOD) return 1;  // last internal module
    if(m == TX_MODULE_LAST || Transmitter.module_enable[m].port != 0)
        return 1;
    return 0;
#if HAS_4IN1_FLASH
    }
#endif
}

int PROTOCOL_HasPowerAmp(int idx)
{
#if HAS_4IN1_FLASH
    if (SPISwitch_Present()) {
        return 1;
    } else {
#endif
    int m = get_module(idx);
    if (m != TX_MODULE_LAST && ((Transmitter.module_poweramp & (1 << m))
                             || (m > MULTIMOD)))  // all external modules
        return 1;
    return 0;
#if HAS_4IN1_FLASH
    }
#endif
}

int PROTOCOL_SetSwitch(int module)
{
    (void)module;
#if HAS_MULTIMOD_SUPPORT
    if (! Transmitter.module_enable[MULTIMOD].port)
        return 0;
    u8 csn = SPI_ProtoGetPinConfig(module, CSN_PIN);
    return SPI_ConfigSwitch(0x0f, 0x0f ^ csn);
#else
    return 0;
#endif
}

int PROTOCOL_SticksMoved(int init)
{
    const s32 STICK_MOVEMENT = 15;   // defines when the bind dialog should be interrupted (stick movement STICK_MOVEMENT %)
    static s32 ele_start, ail_start;
    s32 ele = CHAN_ReadInput(MIXER_MapChannel(INP_ELEVATOR));
    s32 ail = CHAN_ReadInput(MIXER_MapChannel(INP_AILERON));
    if(init) {
        ele_start = ele;
        ail_start = ail;
        return 0;
    }
    s32 ele_diff = abs(ele_start - ele);
    s32 ail_diff = abs(ail_start - ail);
    return ((ele_diff + ail_diff > 2 * STICK_MOVEMENT * CHAN_MAX_VALUE / 100));
}

void PROTOCOL_InitModules()
{
#if HAS_MULTIMOD_SUPPORT
    int error = 0;
    const char * missing[TX_MODULE_LAST];
    memset(missing, 0, sizeof(missing));

    if (PROTOCOL_SetSwitch(TX_MODULE_LAST) == 0) {
        //No Switch found
/*
        missing[MULTIMOD] = MODULE_NAME[MULTIMOD];
        for(int i = 0; i < MULTIMOD; i++) {
            if(Transmitter.module_enable[i].port == SWITCH_ADDRESS) {
                error = 1;
                printf("Disabling %s because switch wasn't found\n", MODULE_NAME[i]);
                missing[i] = MODULE_NAME[i];
                Transmitter.module_enable[i].port = 0;
            }
        }
*/
    }
    int orig_proto = Model.protocol;
    for(int i = 0; i < MULTIMOD; i++) {
        if(Transmitter.module_enable[i].port) {
            for(int j = 1; j < PROTOCOL_COUNT; j++) {
                if (get_module(j) == i) {
                    //Try this module
                    Model.protocol = j;
                    PROTOCOL_Load(1);
                    if (! PROTOCOL_LOADED)
                        continue;
                    int res = (long)PROTO_Cmds(PROTOCMD_RESET);
                    if (res == 0)
                        continue;
                    if (res < 0) {
                        error = 1;
                        missing[i] = MODULE_NAME[i];
                        if (! (Transmitter.extra_hardware & FORCE_MODULES))
                            Transmitter.module_enable[i].port = 0;
                    }
                    break;
                }
            }
        }
    }
    //Put this last because the switch will not respond until after it has been initialized
    if (Transmitter.module_enable[MULTIMOD].port && PROTOCOL_SetSwitch(TX_MODULE_LAST) == 0) {
        //No Switch found
	error = 1;
        missing[MULTIMOD] = MODULE_NAME[MULTIMOD];
    }
    Model.protocol = orig_proto;
    if(error) {
        proto_state |= PROTO_MODULEDLG;
        PAGE_ShowModuleDialog(missing);
    } else
#endif //HAS_MULTIMOD_SUPPORT
    {
        PROTOCOL_Init(0);
    }
}

void PROTO_CS_HI(int module)
{
#if HAS_4IN1_FLASH
    if (SPISwitch_Present()) {
        SPISwitch_CS_HI(module);
    } else {
#endif
#if HAS_MULTIMOD_SUPPORT
    if (MODULE_ENABLE[MULTIMOD].port) {
        //We need to set the multimodule CSN even if we don't use it
        //for this protocol so that it doesn't interpret commands
        PROTOSPI_pin_set(MODULE_ENABLE[MULTIMOD]);
        if(MODULE_ENABLE[module].port == SWITCH_ADDRESS) {
            for(int i = 0; i < 20; i++)
                _NOP();
            return;
        }
    }
#endif
    PROTOSPI_pin_set(MODULE_ENABLE[module]);
#if HAS_4IN1_FLASH
    }
#endif
}

void PROTO_CS_LO(int module)
{
#if HAS_4IN1_FLASH
    if (SPISwitch_Present()) {
        SPISwitch_CS_LO(module);
    } else {
#endif
#if HAS_MULTIMOD_SUPPORT
    if (MODULE_ENABLE[MULTIMOD].port) {
        //We need to set the multimodule CSN even if we don't use it
        //for this protocol so that it doesn't interpret commands
        PROTOSPI_pin_clear(MODULE_ENABLE[MULTIMOD]);
        if(MODULE_ENABLE[module].port == SWITCH_ADDRESS) {
            for(int i = 0; i < 20; i++)
                _NOP();
            return;
        }
    }
#endif
    PROTOSPI_pin_clear(MODULE_ENABLE[module]);
#if HAS_4IN1_FLASH
    }
#endif
}
