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

#ifdef PROTO_HAS_DEVO

#define PKTS_PER_CHANNEL 4
#define use_fixedid 0
enum PktState {
    DEVO_BIND,
    DEVO_BIND_SENDCH,
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

static s16 bind_counter;
static enum PktState state;
static u8 packet[16];
static u32 fixed_id;
static u8 radio_ch[5];
static u8 *radio_ch_ptr;
static u8 pkt_num;
static u8 cyrfmfg_id[6];
static u8 num_channels;
static u8 ch_idx;

void scramble_pkt()
{
    u8 i;
    for(i = 0; i < 15; i++) {
        packet[i + 1] ^= cyrfmfg_id[i % 4];
    }
}

void add_pkt_suffix()
{
    //FIXME: upper nibble of byte 9 can be 0x00, 0x80, 0xc0, but which one?
    packet[10] = 0x00 | (PKTS_PER_CHANNEL - pkt_num - 1);
    packet[11] = *(radio_ch_ptr + 1);
    packet[12] = *(radio_ch_ptr + 2);
    packet[13] = fixed_id  & 0xff;
    packet[14] = (fixed_id >> 8) & 0xff;
    packet[15] = (fixed_id >> 16) & 0xff;
}

void build_unk_pkt()
{
    packet[0] = (num_channels << 4) | 0x07;
    memset(packet + 1, 0, 8);
    packet[9] = 0;
    add_pkt_suffix();
}

void build_bind_pkt()
{
    packet[0] = (num_channels << 4) | 0x0a;
    packet[1] = bind_counter & 0xff;
    packet[2] = (bind_counter >> 8);
    packet[3] = radio_ch[0];
    packet[4] = radio_ch[1];
    packet[5] = radio_ch[2];
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

void build_data_pkt()
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

void set_radio_channels()
{
    //FIXME: Query free channels
    radio_ch[0] = 0x08;
    radio_ch[1] = 0x0c;
    radio_ch[2] = 0x04;
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
            //scramble_pkt();
            state = (bind_counter <= 0) ? DEVO_BOUND_1 : DEVO_BIND;
            break;
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
            build_unk_pkt();
            scramble_pkt();
            state = DEVO_BOUND_1;
            break;
    }
    pkt_num++;
    if(pkt_num == PKTS_PER_CHANNEL)
        pkt_num = 0;
}

void devo_cb()
{
    int i;
    DEVO_BuildPacket();
    for(i = 0; i < 16; i++) {
        printf("%02x ", packet[i]);
    }
    printf("\n");
}

void DEVO_Initialize()
{
    set_radio_channels();
    radio_ch_ptr = radio_ch;
    CYRF_GetMfgData(cyrfmfg_id);
    //FIXME: Read cyrfmfg_id here
    //FIXME: Properly setnumber of channels;
    num_channels = 8;
    pkt_num = 0;
    ch_idx = 0;
    fixed_id = 0x094228;

    if(! use_fixedid) {
        bind_counter = 0x1388;
        state = DEVO_BIND;
    } else {
        state = DEVO_BOUND_1;
        bind_counter = 0;
    }
    CLOCK_StartTimer(2400, devo_cb);
}

#endif
