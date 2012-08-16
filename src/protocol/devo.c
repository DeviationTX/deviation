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

//For Debug
//#define NO_SCRAMBLE

#define PKTS_PER_CHANNEL 4

enum PktState {
    DEVO_BIND,
    DEVO_BIND_SENDCH,
    DEVO_BOUND,
    DEVO_BOUND_1,
    DEVO_BOUND_2,
    DEVO_BOUND_3,
    DEVO_BOUND_4,
    DEVO_BOUND_5,
    DEVO_BOUND_6,
    DEVO_BOUND_7,
    DEVO_BOUND_8,
    DEVO_BOUND_9,
    DEVO_BOUND_10,
};

static const u8 sopcodes[][8] = {
    /* Note these are in order transmitted (LSB 1st) */
    /* 0 */ {0x3C,0x37,0xCC,0x91,0xE2,0xF8,0xCC,0x91}, //0x91CCF8E291CC373C
    /* 1 */ {0x9B,0xC5,0xA1,0x0F,0xAD,0x39,0xA2,0x0F}, //0x0FA239AD0FA1C59B
    /* 2 */ {0xEF,0x64,0xB0,0x2A,0xD2,0x8F,0xB1,0x2A}, //0x2AB18FD22AB064EF
    /* 3 */ {0x66,0xCD,0x7C,0x50,0xDD,0x26,0x7C,0x50}, //0x507C26DD507CCD66
    /* 4 */ {0x5C,0xE1,0xF6,0x44,0xAD,0x16,0xF6,0x44}, //0x44F616AD44F6E15C
    /* 5 */ {0x5A,0xCC,0xAE,0x46,0xB6,0x31,0xAE,0x46}, //0x46AE31B646AECC5A
    /* 6 */ {0xA1,0x78,0xDC,0x3C,0x9E,0x82,0xDC,0x3C}, //0x3CDC829E3CDC78A1
    /* 7 */ {0xB9,0x8E,0x19,0x74,0x6F,0x65,0x18,0x74}, //0x7418656F74198EB9
    /* 8 */ {0xDF,0xB1,0xC0,0x49,0x62,0xDF,0xC1,0x49}, //0x49C1DF6249C0B1DF
    /* 9 */ {0x97,0xE5,0x14,0x72,0x7F,0x1A,0x14,0x72}, //0x72141A7F7214E597
};


static s16 bind_counter;
static enum PktState state;
static u8 txState;
static u8 packet[16];
static u32 fixed_id;
static u8 radio_ch[5];
static u8 *radio_ch_ptr;
static u8 pkt_num;
static u8 cyrfmfg_id[6];
static u8 num_channels;
static u8 ch_idx;

static void scramble_pkt()
{
#ifdef NO_SCRAMBLE
    return;
#else
    u8 i;
    for(i = 0; i < 15; i++) {
        packet[i + 1] ^= cyrfmfg_id[i % 4];
    }
#endif
}

static void add_pkt_suffix()
{
    //FIXME: upper nibble of byte 9 can be 0x00, 0x80, 0xc0, but which one?
    packet[10] = 0x00 | (PKTS_PER_CHANNEL - pkt_num - 1);
    packet[11] = *(radio_ch_ptr + 1);
    packet[12] = *(radio_ch_ptr + 2);
    packet[13] = fixed_id  & 0xff;
    packet[14] = (fixed_id >> 8) & 0xff;
    packet[15] = (fixed_id >> 16) & 0xff;
}

static void build_beacon_pkt()
{
    packet[0] = (num_channels << 4) | 0x07;
    memset(packet + 1, 0, 8);
    packet[9] = 0;
    add_pkt_suffix();
}

static void build_bind_pkt()
{
    packet[0] = (num_channels << 4) | 0x0a;
    packet[1] = bind_counter & 0xff;
    packet[2] = (bind_counter >> 8);
    packet[3] = *radio_ch_ptr;
    packet[4] = *(radio_ch_ptr + 1);
    packet[5] = *(radio_ch_ptr + 2);
    packet[6] = cyrfmfg_id[0];
    packet[7] = cyrfmfg_id[1];
    packet[8] = cyrfmfg_id[2];
    packet[9] = cyrfmfg_id[3];
    add_pkt_suffix();
    //The fixed-id portion is scrambled in the bind packet
    //I assume it is ignored
    packet[13] ^= cyrfmfg_id[0];
    packet[14] ^= cyrfmfg_id[1];
    packet[15] ^= cyrfmfg_id[2];
}

static void build_data_pkt()
{
    u8 i;
    packet[0] = (num_channels << 4) | (0x0b + ch_idx);
    u8 sign = 0x0b;
    for (i = 0; i < 4; i++) {
        s32 value = (s32)Channels[ch_idx * 4 + i] * 0x640 / CHAN_MAX_VALUE;
        if(value < 0) {
            value = -value;
            sign |= 1 << (7 - i);
        }
        packet[2 * i + 1] = value & 0xff;
        packet[2 * i + 2] = (value >> 8) & 0xff;
    }
    packet[9] = sign;
    ch_idx = ch_idx + 1;
    if (ch_idx * 4 >= num_channels)
        ch_idx = 0;
    add_pkt_suffix();
}

static void cyrf_init()
{
    /* Initialise CYRF chip */
    CYRF_WriteRegister(CYRF_1D_MODE_OVERRIDE, 0x39);
    CYRF_WriteRegister(CYRF_03_TX_CFG, 0x0B);
    CYRF_WriteRegister(CYRF_06_RX_CFG, 0x4A);
    CYRF_WriteRegister(CYRF_0B_PWR_CTRL, 0x00);
    CYRF_WriteRegister(CYRF_0D_IO_CFG, 0x04);
    CYRF_WriteRegister(CYRF_0E_GPIO_CTRL, 0x20);
    CYRF_WriteRegister(CYRF_10_FRAMING_CFG, 0xA4);
    CYRF_WriteRegister(CYRF_11_DATA32_THOLD, 0x05);
    CYRF_WriteRegister(CYRF_12_DATA64_THOLD, 0x0E);
    CYRF_WriteRegister(CYRF_1B_TX_OFFSET_LSB, 0x55);
    CYRF_WriteRegister(CYRF_1C_TX_OFFSET_MSB, 0x05);
    CYRF_WriteRegister(CYRF_32_AUTO_CAL_TIME, 0x3C);
    CYRF_WriteRegister(CYRF_35_AUTOCAL_OFFSET, 0x14);
    CYRF_WriteRegister(CYRF_39_ANALOG_CTRL, 0x01);
    CYRF_WriteRegister(CYRF_1E_RX_OVERRIDE, 0x10);
    CYRF_WriteRegister(CYRF_1F_TX_OVERRIDE, 0x00);
    CYRF_WriteRegister(CYRF_01_TX_LENGTH, 0x10);
    CYRF_WriteRegister(CYRF_0C_XTAL_CTRL, 0xC0);
    CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x10);
    CYRF_WriteRegister(CYRF_27_CLK_OVERRIDE, 0x02);
    CYRF_WriteRegister(CYRF_28_CLK_EN, 0x02);
    CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x28);
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
    //Makes code a little easier to duplicate these here
    radio_ch[3] = radio_ch[0];
    radio_ch[4] = radio_ch[1];
}

void DEVO_BuildPacket()
{
    switch(state) {
        case DEVO_BIND:
            bind_counter--;
            build_bind_pkt();
            state = DEVO_BIND_SENDCH;
            break;
        case DEVO_BIND_SENDCH:
            bind_counter--;
            build_data_pkt();
            scramble_pkt();
            state = (bind_counter <= 0) ? DEVO_BOUND : DEVO_BIND;
            break;
        case DEVO_BOUND:
        case DEVO_BOUND_1:
        case DEVO_BOUND_2:
        case DEVO_BOUND_3:
        case DEVO_BOUND_4:
        case DEVO_BOUND_5:
        case DEVO_BOUND_6:
        case DEVO_BOUND_7:
        case DEVO_BOUND_8:
        case DEVO_BOUND_9:
            build_data_pkt();
            scramble_pkt();
            state++;
            break;
        case DEVO_BOUND_10:
            build_beacon_pkt();
            scramble_pkt();
            state = DEVO_BOUND_1;
            break;
    }
    pkt_num++;
    if(pkt_num == PKTS_PER_CHANNEL)
        pkt_num = 0;
}

static u16 devo_cb()
{
    if (txState == 0) {
        txState = 1;
        DEVO_BuildPacket();
        CYRF_WriteDataPacket(packet);
        return 1200;
    }
    txState = 0;
    while(! (CYRF_ReadRegister(0x04) & 0x02))
        ;
    if (state == DEVO_BOUND) {
        /* exit binding state */
        state = DEVO_BOUND_3;
        /* crc == 0 isn't allowed, so use 1 if the math results in 0 */
        u8 crc = (cyrfmfg_id[0] + (cyrfmfg_id[1] >> 6) + cyrfmfg_id[2]);
        if(! crc)
            crc = 1;
        u8 sopidx = (0xff &((cyrfmfg_id[0] << 2) + cyrfmfg_id[1] + cyrfmfg_id[2])) % 10;
        CYRF_ConfigRxTx(1);
        CYRF_ConfigCRCSeed((crc << 8) + crc);
        CYRF_ConfigSOPCode(sopcodes[sopidx]);
        CYRF_WriteRegister(CYRF_03_TX_CFG, 0x0D);
    }   
    if(pkt_num == 0) {
        radio_ch_ptr = radio_ch_ptr == &radio_ch[2] ? radio_ch : radio_ch_ptr + 1;
        CYRF_ConfigRFChannel(*radio_ch_ptr);
    }
    return 1200;
}

void DEVO_Initialize()
{
    CLOCK_StopTimer();
    CYRF_Reset();
    cyrf_init();
    CYRF_GetMfgData(cyrfmfg_id);
    CYRF_ConfigRxTx(1);
    CYRF_ConfigCRCSeed(0x0000);
    CYRF_ConfigSOPCode(sopcodes[0]);
    set_radio_channels();
    radio_ch_ptr = radio_ch;
    CYRF_ConfigRFChannel(*radio_ch_ptr);
    //FIXME: Properly setnumber of channels;
    num_channels = 8;
    pkt_num = 0;
    ch_idx = 0;
    txState = 0;

    if(! Model.fixed_id) {
        fixed_id = ((u32)(radio_ch[0] ^ cyrfmfg_id[0] ^ cyrfmfg_id[3]) << 16)
                 | ((u32)(radio_ch[1] ^ cyrfmfg_id[1] ^ cyrfmfg_id[4]) << 8)
                 | ((u32)(radio_ch[2] ^ cyrfmfg_id[2] ^ cyrfmfg_id[5]) << 0);
        bind_counter = 0x1388;
        state = DEVO_BIND;
    } else {
        fixed_id = Model.fixed_id;
        state = DEVO_BOUND_1;
        bind_counter = 0;
    }
    CLOCK_StartTimer(2400, devo_cb);
}

#endif
