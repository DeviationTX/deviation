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
#include "mixer.h"
#include "config/model.h"

#ifdef PROTO_HAS_CYRF6936

#define PKTS_PER_CHANNEL 4
#define use_fixedid 0

//Fewer bind packets in the emulator so we can get right to the important bits
#ifdef EMULATOR
#define BIND_COUNT 3
#else
#define BIND_COUNT 2980
#endif

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
static u8 chan_dir;
static u8 last_beacon;

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
    chan_dir = 0;
    u8 offset = 0;
    for (i = 0; i < 8; i++) {
        if (i == 4)
            offset = 1;
        u16 value = (i & 0x01) ? 0x200 : get_channel(i >> 1, 0x200, 0x200, 0x1FF);
        packet[i+offset] = value & 0xff;
        msb = (msb << 2) | ((value >> 8) & 0x03);
    }
    packet[4] = msb >> 8; //Ele/Ail MSB
    packet[9] = msb & 0xff; //Thr/Rud MSB
    packet[10]  = 0xe0 | ((fixed_id >> 0)  & 0x0e);
    packet[11] = (fixed_id >> 8)  & 0xff;
    packet[12] = ((fixed_id >> 12) & 0xf0) | pkt_num;
    packet[13] = 0xf0; //FIXME - What is this?
    add_pkt_crc(0x00);
}

static void build_data_pkt_2601()
{
    u8 i;
    u8 msb = 0;
    chan_dir = 0;
    for (i = 0; i < 4; i++) {
        s16 value = get_channel(i, 0x190, 0, 0x1FF);
        u16 mag = value < 0 ? -value : value;
        packet[i] = mag & 0xff;
        msb = (msb << 2) | ((mag >> 8) & 0x01) | (value < 0 ? 0x02 : 0x00);
    }
    u16 gyro = get_channel(3, 400, 500, 400);
    packet[4] = msb;
    packet[5] = 0;
    packet[6] = 0;
    packet[7] = 0;
    packet[8] = (get_channel(4, 0x190, 0, 0x1FF) > 0 ? 1 : 0)
                | (get_channel(5, 0x190, 0, 0x1FF) > 0 ? 2 : 0);
    packet[9] = (pkt_num % 3 == 0 ? 0x00 : 0xa0)
                |((gyro >> 6) & 0x0c) | (pkt_num % 3);
    packet[10]  = (fixed_id >> 0)  & 0xff;
    packet[11] = (fixed_id >> 8)  & 0xff;
    packet[12] = ((fixed_id >> 12) & 0xf0) | pkt_num;
    packet[13] = gyro & 0xff;

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
    packet[9] = Model.fixed_id ? 0x1b : 0x99; //FIXME: Handle fixed-id programming (0xe4)
    packet[10]  = (fixed_id >> 0)  & 0xff;
    packet[11] = (fixed_id >> 8)  & 0xff;
    packet[12] = ((fixed_id >> 12) & 0xf0) | pkt_num;
    packet[13] = 0x00; //Does this matter?  in the docs it is the same as the data packet
    add_pkt_crc(0x1C);
}

static void cyrf_init()
{
    /* Initialise CYRF chip */
    CYRF_WriteRegister(CYRF_03_TX_CFG, 0x2B);
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
    //FIXME: Query free channels
    radio_ch[0] = 0x08;
    radio_ch[1] = 0x0c;
    radio_ch[2] = 0x04;
}

void WK_BuildPacket_2801()
{
    switch(state) {
        case WK_BIND:
            build_bind_pkt(init_2801);
            if (--bind_counter == 0)
                state = WK_BOUND_1;
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
            break;
    }
    pkt_num = (pkt_num + 1) % 12;
}

void WK_BuildPacket_2601()
{
    if (bind_counter) {
        bind_counter--;
        build_bind_pkt(init_2601);
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
    } else {
        build_data_pkt_2401();
    }
    pkt_num = (pkt_num + 1) % 12;
}

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
    while(! (CYRF_ReadRegister(0x04) & 0x02))
        ;
    if((pkt_num & 0x03) == 0) {
        radio_ch_ptr = radio_ch_ptr == &radio_ch[2] ? radio_ch : radio_ch_ptr + 1;
        CYRF_ConfigRFChannel(*radio_ch_ptr);
    }
    return 1200;
}

void WK2x01_Initialize()
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
    chan_dir = 0;
    if (! Model.fixed_id) {
        u8 cyrfmfg_id[6];
        CYRF_GetMfgData(cyrfmfg_id);
        fixed_id = ((u32)cyrfmfg_id[0] << 16) |
                   ((u32)cyrfmfg_id[1] << 8) |
                   ((u32)cyrfmfg_id[2]);
    } else {
        fixed_id = ((Model.fixed_id << 2)  & 0x0ffc00) |
               ((Model.fixed_id >> 10) & 0x000300) |
               ((Model.fixed_id)       & 0x0000ff);
    }
    if (Model.protocol == PROTOCOL_WK2401)
        fixed_id |= 0x01;  //Fixed ID must be odd for 2401
    if(! use_fixedid) {
        bind_counter = BIND_COUNT;
        state = WK_BIND;
    } else {
        state = WK_BOUND_1;
        bind_counter = 0;
    }
    CYRF_ConfigRFChannel(*radio_ch_ptr);
    CLOCK_StartTimer(2800, wk_cb);
}

#endif
