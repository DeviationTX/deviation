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

#ifdef MODULAR
  //Allows the linker to properly relocate
  #define WK2x01_Cmds PROTO_Cmds
  #pragma long_calls
#endif

#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"

#ifdef MODULAR
  #pragma long_calls_off
  extern unsigned _data_loadaddr;
  const unsigned long protocol_type = (unsigned long)&_data_loadaddr;
  //Force the following functions to be builtin 
  #define abs __builtin_abs
#endif
#include <stdlib.h>

#ifdef PROTO_HAS_CYRF6936

#define PKTS_PER_CHANNEL 4

//Fewer bind packets in the emulator so we can get right to the important bits
#ifdef EMULATOR
#define BIND_COUNT 3
#else
#define BIND_COUNT 2980
#endif

#define NUM_WAIT_LOOPS (100 / 5) //each loop is ~5us.  Do not wait more than 100us

enum PktState {
    WK_BIND,
    WK_BOUND_1,
    WK_BOUND_2,
    WK_BOUND_3,
    WK_BOUND_4,
    WK_BOUND_5,
    WK_BOUND_6,
    WK_BOUND_7,
    WK_BOUND_8,
};

static const u8 sopcodes[8] = {
    /* Note these are in order transmitted (LSB 1st) */
    0xDF,0xB1,0xC0,0x49,0x62,0xDF,0xC1,0x49 //0x49C1DF6249C0B1DF
};
static const u8 fail_map[8] = {2, 1, 0, 3, 4, 5, 6, 7};

static s16 bind_counter;
static enum PktState state;
static u8 txState;
static u8 packet[16];
static u32 fixed_id;
static u8 radio_ch[3];
static u8 *radio_ch_ptr;
static u8 pkt_num;
static u8 last_beacon;

static const char * const wk2601_opts[] = {
  _tr_noop("Chan mode"), _tr_noop("5+1"), _tr_noop("Heli"), _tr_noop("6+1"), NULL,
  _tr_noop("COL Inv"), _tr_noop("Normal"), _tr_noop("Inverted"), NULL,
  _tr_noop("COL Limit"), "-100", "100", NULL,
  NULL
};
#define WK2601_OPT_CHANMODE  0
#define WK2601_OPT_PIT_INV   1
#define WK2601_OPT_PIT_LIMIT 2


static void add_pkt_crc(u8 init)
{
    u8 add = init;
    u8 xor = init;
    int i;
    for (i = 0; i < 14; i++) {
        add += packet[i];
        xor ^= packet[i];
    }
    packet[14] = xor;
    packet[15] = add & 0xff;
}
static const char init_2801[] = {0xc5, 0x34, 0x60, 0x00, 0x25};
static const char init_2601[] = {0xb9, 0x45, 0xb0, 0xf1, 0x3a};
static const char init_2401[] = {0xa5, 0x23, 0xd0, 0xf0, 0x00};
static void build_bind_pkt(const char *init)
{
    packet[0] = init[0];
    packet[1] = init[1];
    packet[2] = radio_ch[0];
    packet[3] = radio_ch[1];
    packet[4] = init[2];
    packet[5] = radio_ch[2];
    packet[6] = 0xff;
    packet[7] = 0x00;
    packet[8] = 0x00;
    packet[9] = 0x32;
    if (Model.protocol == PROTOCOL_WK2401)
        packet[10]  = 0x10 | ((fixed_id >> 0)  & 0x0e);
    else
        packet[10]  = (fixed_id >> 0) & 0xff;
    packet[11] = (fixed_id >> 8)  & 0xff;
    packet[12] = ((fixed_id >> 12) & 0xf0) | pkt_num;
    packet[13] = init[3];
    add_pkt_crc(init[4]);
}

static s16 get_channel(u8 ch, s32 scale, s32 center, s32 range)
{
    s32 value = (s32)Channels[ch] * scale / CHAN_MAX_VALUE + center;
    if (value < center - range)
        value = center - range;
    if (value > center + range)
        value = center + range;
    return value;
}

static void build_data_pkt_2401()
{
    u8 i;
    u16 msb = 0;
    u8 offset = 0;
    for (i = 0; i < 4; i++) {
        if (i == 2)
            offset = 1;
        s16 value = get_channel(i, 0x800, 0, 0xA00); //12 bits, allow value to go to 125%
        u16 base = abs(value) >> 2;  //10 bits is the base value
        u16 trim = abs(value) & 0x03; //lowest 2 bits represent trim
        if (base >= 0x200) {  //if value is > 100%, remainder goes to trim
            trim = 4 *(base - 0x200);
            base = 0x1ff;
        }
        base = (value >= 0) ? 0x200 + base : 0x200 - base;
        trim = (value >= 0) ? 0x200 + trim : 0x200 - trim;
     
        packet[2*i+offset]   = base & 0xff;
        packet[2*i+offset+1] = trim & 0xff;
        msb = (msb << 4) | ((base >> 6) & 0x0c) | ((trim >> 8) & 0x03);
    }
    packet[4] = msb >> 8; //Ele/Ail MSB
    packet[9] = msb & 0xff; //Thr/Rud MSB
    packet[10]  = 0xe0 | ((fixed_id >> 0)  & 0x0e);
    packet[11] = (fixed_id >> 8)  & 0xff;
    packet[12] = ((fixed_id >> 12) & 0xf0) | pkt_num;
    packet[13] = 0xf0; //FIXME - What is this?
    add_pkt_crc(0x00);
}

#define PCT(pct, max) (((max) * (pct) + 1L) / 1000)
#define MAXTHR 426 //Measured to provide equal value at +/-0
static void channels_6plus1_2601(int frame, int *_v1, int *_v2)
{
    s16 thr = get_channel(2, 1000, 0, 1000);
    int v1;
    int thr_rev = 0, pitch_rev = 0;
    /*Throttle is computed as follows:
        val <= -78%    : thr = (100-42.6)% * (-val-78%) / 22% + 42.6%, tcurve=100%, thr_rev=1
        -78% < val < 0 : thr = 42.6%, tcurve = (-val)/78%, thr_rev=1
        0 <= val < 78% : thr = 42.6%, tcurve = 100% - val/78%, thr_rev=0
        78% <= val     : thr = (100-42.6)% * (val-78%) / 22% + 42.6%, tcurve=0, thr_rev=0
     */
    if(thr > 0) {
        if(thr >= 780) { //78%
            v1 = 0; //thr = 60% * (x - 78%) / 22% + 40%
            thr = PCT(1000-MAXTHR,512) * (thr-PCT(780,1000)) / PCT(220,1000) + PCT(MAXTHR,512);
        } else {
            v1 = 1023 - 1023 * thr / 780;
            thr = PCT(MAXTHR, 512); //40%
        }
    } else {
        thr = -thr;
        thr_rev = 1;
        if(thr >= 780) { //78%
            v1 = 1023; //thr = 60% * (x - 78%) / 22% + 40%
            thr = PCT(1000-MAXTHR,512) * (thr-PCT(780,1000)) / PCT(220,1000) + PCT(MAXTHR,512);
            if (thr >= 512)
                thr = 511;
        } else {
            v1 = 1023 * thr / 780;
            thr = PCT(MAXTHR, 512); //40%
        }
    }
    if (thr >= 512)
        thr = 511;
    packet[2] = thr & 0xff;
    packet[4] = (packet[4] & 0xF3) | ((thr >> 6) & 0x04);

    s16 pitch= get_channel(5, 0x400, 0, 0x400);
    if (pitch < 0) {
        pitch_rev = 1;
        pitch = -pitch;
    }
    if (frame == 1) {
        //Pitch curve and range
        if (thr > PCT(MAXTHR, 512)) {
            //v2 = pit% * ( 1 - (thr% - 40) / 60 * 16%)
            *_v2 = pitch - pitch * 16 * (thr - PCT(MAXTHR, 512)) / PCT(1000 - MAXTHR, 512) / 100;
        } else {
            *_v2 = pitch;
        }
        *_v1 = 0;
    } else if (frame == 2) {
        //Throttle curve & Expo
        *_v1 = v1;
        *_v2 = 512;
    }
    packet[7] = (thr_rev << 5) | (pitch_rev << 2); //reverse bits
    packet[8] = 0;
}

static void channels_5plus1_2601(int frame, int *v1, int *v2)
{
    (void)v1;
    //Zero out pitch, provide ail, ele, thr, rud, gyr + gear
    if (frame == 1) {
        //Pitch curve and range
        *v2 = 0;
    }
    packet[7] = 0;
    packet[8] = 0;
}
static void channels_heli_2601(int frame, int *v1, int *v2)
{
    (void)frame;
    //pitch is controlled by rx
    //we can only control fmode, pit-reverse and pit/thr rate
    int pit_rev = 0;
    if (Model.proto_opts[WK2601_OPT_PIT_INV]) {
        pit_rev = 1;
    }
    s16 pit_rate = get_channel(5, 0x400, 0, 0x400);
    int fmode = 1;
    if (pit_rate < 0) {
        pit_rate = -pit_rate;
        fmode = 0;
    }
    if (frame == 1) {
        //Pitch curve and range
        *v1 = pit_rate;
        *v2 = Model.proto_opts[WK2601_OPT_PIT_LIMIT] * 0x400 / 100 + 0x400;
    }
    packet[7] = (pit_rev << 2); //reverse bits
    packet[8] = fmode ? 0x02 : 0x00;
}

static void build_data_pkt_2601()
{
    u8 i;
    u8 msb = 0;
    u8 frame = (pkt_num % 3);
    for (i = 0; i < 4; i++) {
        s16 value = get_channel(i, 0x190, 0, 0x1FF);
        u16 mag = value < 0 ? -value : value;
        packet[i] = mag & 0xff;
        msb = (msb << 2) | ((mag >> 8) & 0x01) | (value < 0 ? 0x02 : 0x00);
    }
    packet[4] = msb;
    int v1 = 0x200, v2 = 0x200;
    if (frame == 0) {
        //Gyro & Rudder mix
        v1 = get_channel(6, 0x200, 0x200, 0x200);
        v2 = 0;
    }
    if (Model.proto_opts[WK2601_OPT_CHANMODE] == 1) {
        channels_heli_2601(frame, &v1, &v2);
    } else if (Model.proto_opts[WK2601_OPT_CHANMODE] == 2) {
        channels_6plus1_2601(frame, &v1, &v2);
    } else {
        channels_5plus1_2601(frame, &v1, &v2);
    }
    if (v1 > 1023)
        v1 = 1023;
    if (v2 > 1023)
        v2 = 1023;
    packet[5] = v2 & 0xff;
    packet[6] = v1 & 0xff;
    //packet[7] handled by channel code
    packet[8] |= (get_channel(4, 0x190, 0, 0x1FF) > 0 ? 1 : 0);
    packet[9] =  ((v1 >> 4) & 0x30)
               | ((v2 >> 2) & 0xc0)
               | 0x04 | frame;
    packet[10]  = (fixed_id >> 0)  & 0xff;
    packet[11] = (fixed_id >> 8)  & 0xff;
    packet[12] = ((fixed_id >> 12) & 0xf0) | pkt_num;
    packet[13] = 0xff;

    add_pkt_crc(0x3A);
}

static void build_data_pkt_2801()
{
    u8 i;
    u16 msb = 0;
    u8 offset = 0;
    u8 sign = 0;
    for (i = 0; i < 8; i++) {
        if (i == 4)
            offset = 1;
        s16 value = get_channel(i, 0x190, 0, 0x3FF);
        u16 mag = value < 0 ? -value : value;
        packet[i+offset] = mag & 0xff;
        msb = (msb << 2) | ((mag >> 8) & 0x03);
        if (value < 0)
            sign |= 1 << i;
    }
    packet[4] = msb >> 8;
    packet[9] = msb  & 0xff;
    packet[10]  = (fixed_id >> 0)  & 0xff;
    packet[11] = (fixed_id >> 8)  & 0xff;
    packet[12] = ((fixed_id >> 12) & 0xf0) | pkt_num;
    packet[13] = sign;
    add_pkt_crc(0x25);
}

static void build_beacon_pkt_2801()
{
    last_beacon ^= 1;
    u8 i;
    u8 en = 0;
    u8 bind_state;
    if (Model.fixed_id) {
        if (bind_counter) {
            bind_state = 0xe4;
        } else {
            bind_state = 0x1b;
        }
    } else {
        bind_state = 0x99;
    }
    for (i = 0; i < 4; i++) {
        if (Model.limits[fail_map[i + last_beacon * 4]].flags & CH_FAILSAFE_EN) {
            s32 value = Model.limits[fail_map[i + last_beacon * 4]].failsafe + 128;
            if (value > 255)
                value = 255;
            if (value < 0)
                value = 0;
            packet[i+1] = value;
            en |= 1 << i;
        } else {
            packet[i+1] = 0;
        }
    }
    packet[0] = en;
    packet[5] = packet[4];
    packet[4] = last_beacon << 6;
    packet[6] = radio_ch[0];
    packet[7] = radio_ch[1];
    packet[8] = radio_ch[2];
    packet[9] = bind_state;
    packet[10]  = (fixed_id >> 0)  & 0xff;
    packet[11] = (fixed_id >> 8)  & 0xff;
    packet[12] = ((fixed_id >> 12) & 0xf0) | pkt_num;
    packet[13] = 0x00; //Does this matter?  in the docs it is the same as the data packet
    add_pkt_crc(0x1C);
}

static void cyrf_init()
{
    /* Initialise CYRF chip */
    CYRF_WriteRegister(CYRF_03_TX_CFG, 0x28 | Model.tx_power);
    CYRF_WriteRegister(CYRF_06_RX_CFG, 0x4A);
    CYRF_WriteRegister(CYRF_0B_PWR_CTRL, 0x00);
    CYRF_WriteRegister(CYRF_0C_XTAL_CTRL, 0xC0);
    CYRF_WriteRegister(CYRF_0D_IO_CFG, 0x04);
    CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x2C);
    CYRF_WriteRegister(CYRF_10_FRAMING_CFG, 0xEE);
    CYRF_WriteRegister(CYRF_1B_TX_OFFSET_LSB, 0x55);
    CYRF_WriteRegister(CYRF_1C_TX_OFFSET_MSB, 0x05);
    CYRF_WriteRegister(CYRF_1D_MODE_OVERRIDE, 0x18);
    CYRF_WriteRegister(CYRF_32_AUTO_CAL_TIME, 0x3C);
    CYRF_WriteRegister(CYRF_35_AUTOCAL_OFFSET, 0x14);
    CYRF_WriteRegister(CYRF_1E_RX_OVERRIDE, 0x90);
    CYRF_WriteRegister(CYRF_1F_TX_OVERRIDE, 0x00);
    CYRF_WriteRegister(CYRF_01_TX_LENGTH, 0x10);
    CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x2C);
    CYRF_WriteRegister(CYRF_28_CLK_EN, 0x02);
    CYRF_WriteRegister(CYRF_27_CLK_OVERRIDE, 0x02);
    CYRF_ConfigSOPCode(sopcodes);
    CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x28);
    CYRF_WriteRegister(CYRF_1E_RX_OVERRIDE, 0x10);
    CYRF_WriteRegister(CYRF_0E_GPIO_CTRL, 0x20);
    CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x2C);
}

static void set_radio_channels()
{
    int i;
    CYRF_FindBestChannels(radio_ch, 3, 4, 4, 80);
    printf("Radio Channels:");
    for (i = 0; i < 3; i++) {
        printf(" %02x", radio_ch[i]);
    }
    printf("\n");
}

void WK_BuildPacket_2801()
{
    switch(state) {
        case WK_BIND:
            build_bind_pkt(init_2801);
            if (--bind_counter == 0) {
                PROTOCOL_SetBindState(0);
                state = WK_BOUND_1;
            }
            break;
        case WK_BOUND_1:
        case WK_BOUND_2:
        case WK_BOUND_3:
        case WK_BOUND_4:
        case WK_BOUND_5:
        case WK_BOUND_6:
        case WK_BOUND_7:
            build_data_pkt_2801();
            state++;
            break;
        case WK_BOUND_8:
            build_beacon_pkt_2801();
            state = WK_BOUND_1;
            if (bind_counter) {
                bind_counter--;
                if (bind_counter == 0)
                    PROTOCOL_SetBindState(0);
            }
            break;
    }
    pkt_num = (pkt_num + 1) % 12;
}

void WK_BuildPacket_2601()
{
    if (bind_counter) {
        bind_counter--;
        build_bind_pkt(init_2601);
        if (bind_counter == 0)
            PROTOCOL_SetBindState(0);
    } else {
        build_data_pkt_2601();
    }
    pkt_num = (pkt_num + 1) % 12;
}

void WK_BuildPacket_2401()
{
    if (bind_counter) {
        bind_counter--;
        build_bind_pkt(init_2401);
        if (bind_counter == 0)
            PROTOCOL_SetBindState(0);
    } else {
        build_data_pkt_2401();
    }
    pkt_num = (pkt_num + 1) % 12;
}

MODULE_CALLTYPE
static u16 wk_cb()
{
    if (txState == 0) {
        txState = 1;
        if(Model.protocol == PROTOCOL_WK2801) 
            WK_BuildPacket_2801();
        else if(Model.protocol == PROTOCOL_WK2601)
            WK_BuildPacket_2601();
        else if(Model.protocol == PROTOCOL_WK2401)
            WK_BuildPacket_2401();

        CYRF_WriteDataPacket(packet);
        return 1600;
    }
    txState = 0;
    int i = 0;
    while (! (CYRF_ReadRegister(0x04) & 0x02)) {
        if(++i > NUM_WAIT_LOOPS)
            break;
    }
    if((pkt_num & 0x03) == 0) {
        radio_ch_ptr = radio_ch_ptr == &radio_ch[2] ? radio_ch : radio_ch_ptr + 1;
        CYRF_ConfigRFChannel(*radio_ch_ptr);
        //Keep transmit power updated
        CYRF_WriteRegister(CYRF_03_TX_CFG, 0x28 | Model.tx_power);
    }
    return 1200;
}

static void bind()
{
    if((Model.protocol != PROTOCOL_WK2801) || (! Model.fixed_id))
        return;
    fixed_id = ((Model.fixed_id << 2)  & 0x0ffc00) |
           ((Model.fixed_id >> 10) & 0x000300) |
           ((Model.fixed_id)       & 0x0000ff);
    bind_counter = BIND_COUNT / 8 + 1;
    PROTOCOL_SetBindState(2980 * 2800 / 1000);
}

static void initialize()
{
    CLOCK_StopTimer();
    CYRF_Reset();
    cyrf_init();
    CYRF_ConfigRxTx(1);
    set_radio_channels();
    radio_ch_ptr = radio_ch;
    CYRF_ConfigRFChannel(*radio_ch_ptr);

    pkt_num = 0;
    txState = 0;
    last_beacon = 0;
    if (! Model.fixed_id) {
        u8 cyrfmfg_id[6];
        CYRF_GetMfgData(cyrfmfg_id);
        fixed_id = ((radio_ch[0] ^ cyrfmfg_id[0] ^ cyrfmfg_id[3]) << 16)
                 | ((radio_ch[1] ^ cyrfmfg_id[1] ^ cyrfmfg_id[4]) << 8)
                 | ((radio_ch[2] ^ cyrfmfg_id[2] ^ cyrfmfg_id[5]) << 0);
    } else {
        fixed_id = ((Model.fixed_id << 2)  & 0x0ffc00) |
               ((Model.fixed_id >> 10) & 0x000300) |
               ((Model.fixed_id)       & 0x0000ff);
    }
    if (Model.protocol == PROTOCOL_WK2401)
        fixed_id |= 0x01;  //Fixed ID must be odd for 2401
    if(Model.protocol != PROTOCOL_WK2801 || ! Model.fixed_id) {
        bind_counter = BIND_COUNT;
        state = WK_BIND;
        PROTOCOL_SetBindState(2980 * 2800 / 1000);
    } else {
        state = WK_BOUND_1;
        bind_counter = 0;
    }
    CYRF_ConfigRFChannel(*radio_ch_ptr);
    CLOCK_StartTimer(2800, wk_cb);
}

const void *WK2x01_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT: return 0;
        case PROTOCMD_CHECK_AUTOBIND:
            return (Model.protocol == PROTOCOL_WK2801 && Model.fixed_id) ? 0 : (void *)1L;
        case PROTOCMD_BIND:  bind(); return 0;
        case PROTOCMD_DEFAULT_NUMCHAN: return (Model.protocol == PROTOCOL_WK2801)
              ? (void *)8L
              : (Model.protocol == PROTOCOL_WK2601)
                ? (void *)6L
                : (void *)4L;
        case PROTOCMD_NUMCHAN: return (Model.protocol == PROTOCOL_WK2801)
              ? (void *)8L
              : (Model.protocol == PROTOCOL_WK2601)
                ? (void *)7L
                : (void *)4L;
        case PROTOCMD_GETOPTIONS:
            if(Model.protocol == PROTOCOL_WK2601)
                return wk2601_opts;
            break;
        case PROTOCMD_TELEMETRYSTATE:
            return (void *)(long)-1;
        default: break;
    }
    return 0;
}
#endif
