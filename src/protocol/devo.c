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
#include "interface.h"
#include "mixer.h"
#include "telemetry.h"
#include "config/model.h"

#ifdef PROTO_HAS_CYRF6936

//For Debug
//#define NO_SCRAMBLE

#define TELEMETRY 1
#define PKTS_PER_CHANNEL 4

#ifdef EMULATOR
#define BIND_COUNT 4
#else
#define BIND_COUNT 0x1388
#endif

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
static u8 use_fixed_id;

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
    u8 bind_state;
    if (use_fixed_id) {
        if (bind_counter > 0) {
            bind_state = 0xc0;
        } else {
            bind_state = 0x80;
        }
    } else {
        bind_state = 0x00;
    }
    packet[10] = bind_state | (PKTS_PER_CHANNEL - pkt_num - 1);
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

static s32 float_to_int(u8 *ptr)
{
    s32 value = 0;
    int seen_decimal = 0;
    for(int i = 0; i < 7; i++) {
        if(ptr[i] == '.') {
            value *= 1000;
            seen_decimal = 100;
            continue;
        }
        if(ptr[i] == 0)
            break;
        if(seen_decimal) {
            value += (ptr[i] - '0') * seen_decimal;
            seen_decimal /= 10;
            if(! seen_decimal)
                break;
        } else {
            value = value * 10 + (ptr[i] - '0');
        }
    }
    return value;
}
static void parse_telemetry_packet(u8 *packet)
{
    if((packet[0] & 0xF0) != 0x30)
        return;
    scramble_pkt(); //This will unscramble the packet
    //if (packet[0] < 0x37) {
    //    memcpy(Telemetry.line[packet[0]-0x30], packet+1, 12);
    //}
    if (packet[0] == 0x30) {
        Telemetry.volt[0] = packet[1]; //In 1/10 of Volts
        Telemetry.volt[1] = packet[3]; //In 1/10 of Volts
        Telemetry.volt[2] = packet[5]; //In 1/10 of Volts
        Telemetry.rpm[0]  = packet[7] * 120; //In RPM
        Telemetry.rpm[1]  = packet[9] * 120; //In RPM
        Telemetry.time[0] = CLOCK_getms();
    }
    if (packet[0] == 0x31) {
        Telemetry.temp[0] = packet[1] == 0xff ? 0 : packet[1] - 20; //In degrees-C
        Telemetry.temp[1] = packet[2] == 0xff ? 0 : packet[2] - 20; //In degrees-C
        Telemetry.temp[2] = packet[3] == 0xff ? 0 : packet[3] - 20; //In degrees-C
        Telemetry.temp[3] = packet[3] == 0xff ? 0 : packet[4] - 20; //In degrees-C
        Telemetry.time[1] = CLOCK_getms();
    }
    /* GPS Data
       32: 30333032302e3832373045fb  = 030°20.8270E
       33: 353935342e373737364e0700  = 59°54.776N
       34: 31322e380000004d4d4e45fb  = 12.8 MMNE (altitude maybe)?
       35: 000000000000302e30300000  = 0.00 (probably speed)
       36: 313832353532313531303132  = 2012-10-15 18:25:52 (UTC)
    */
    if (packet[0] == 0x32) {
        Telemetry.time[2] = CLOCK_getms();
        Telemetry.gps.longitude = ((packet[1]-'0') * 100 + (packet[2]-'0') * 10 + (packet[3]-'0')) * 3600000
                                  + ((packet[4]-'0') * 10 + (packet[5]-'0')) * 60000
                                  + ((packet[7]-'0') * 1000 + (packet[8]-'0') * 100
                                     + (packet[9]-'0') * 10 + (packet[10]-'0')) * 6;
        if (packet[11] == 'W')
            Telemetry.gps.longitude *= -1;
    }
    if (packet[0] == 0x33) {
        Telemetry.time[2] = CLOCK_getms();
        Telemetry.gps.latitude = ((packet[1]-'0') * 10 + (packet[2]-'0')) * 3600000
                                  + ((packet[3]-'0') * 10 + (packet[4]-'0')) * 60000
                                  + ((packet[6]-'0') * 1000 + (packet[7]-'0') * 100
                                     + (packet[8]-'0') * 10 + (packet[9]-'0')) * 6;
        if (packet[10] == 'S')
            Telemetry.gps.latitude *= -1;
    }
    if (packet[0] == 0x34) {
        Telemetry.time[2] = CLOCK_getms();
        Telemetry.gps.altitude = float_to_int(packet+1);
    }
    if (packet[0] == 0x35) {
        Telemetry.time[2] = CLOCK_getms();
        Telemetry.gps.velocity = float_to_int(packet+7);
    }
    if (packet[0] == 0x36) {
        Telemetry.time[2] = CLOCK_getms();
        u8 hour  = (packet[1]-'0') * 10 + (packet[2]-'0');
        u8 min   = (packet[3]-'0') * 10 + (packet[4]-'0');
        u8 sec   = (packet[5]-'0') * 10 + (packet[6]-'0');
        u8 day   = (packet[7]-'0') * 10 + (packet[8]-'0');
        u8 month = (packet[9]-'0') * 10 + (packet[10]-'0');
        u8 year  = (packet[11]-'0') * 10 + (packet[12]-'0'); // + 2000
        Telemetry.gps.time = ((year & 0x3F) << 26)
                           | ((month & 0x0F) << 22)
                           | ((day & 0x1F) << 17)
                           | ((hour & 0x1F) << 12)
                           | ((min & 0x3F) << 6)
                           | ((sec & 0x3F) << 0);
    }
        
}

static void cyrf_set_bound_sop_code()
{
    /* crc == 0 isn't allowed, so use 1 if the math results in 0 */
    u8 crc = (cyrfmfg_id[0] + (cyrfmfg_id[1] >> 6) + cyrfmfg_id[2]);
    if(! crc)
        crc = 1;
    u8 sopidx = (0xff &((cyrfmfg_id[0] << 2) + cyrfmfg_id[1] + cyrfmfg_id[2])) % 10;
    CYRF_ConfigRxTx(1);
    CYRF_ConfigCRCSeed((crc << 8) + crc);
    CYRF_ConfigSOPCode(sopcodes[sopidx]);
    CYRF_WriteRegister(CYRF_03_TX_CFG, 0x08 | Model.tx_power);
}

static void cyrf_init()
{
    /* Initialise CYRF chip */
    CYRF_WriteRegister(CYRF_1D_MODE_OVERRIDE, 0x39);
    CYRF_WriteRegister(CYRF_03_TX_CFG, 0x08 | Model.tx_power);
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
            if (bind_counter <= 0) {
                state = DEVO_BOUND;
                PROTOCOL_SetBindState(0);
            } else {
                state = DEVO_BIND;
            }
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
            if (bind_counter > 0) {
                bind_counter--;
                if (bind_counter == 0)
                    PROTOCOL_SetBindState(0);
            }
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

static u16 devo_telemetry_cb()
{
    if (txState == 0) {
        txState = 1;
        DEVO_BuildPacket();
        CYRF_WriteDataPacket(packet);
        return 900;
    }
    int delay = 100;
    if (txState == 1) {
        while(! (CYRF_ReadRegister(0x04) & 0x02))
            ;
        if (state == DEVO_BOUND) {
            /* exit binding state */
            state = DEVO_BOUND_3;
            cyrf_set_bound_sop_code();
        }
        if(pkt_num == 0 || bind_counter > 0) {
            delay = 1500;
            txState = 15;
        } else {
            CYRF_ConfigRxTx(0); //Receive mode
            CYRF_WriteRegister(0x07, 0x80); //Prepare to receive
            CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x87); //Prepare to receive
        }
    } else {
        if(CYRF_ReadRegister(0x07) & 0x20) {
            CYRF_ReadDataPacket(packet);
            parse_telemetry_packet(packet);
            delay = 100 * (16 - txState);
            txState = 15;
        }
    }
    txState++;
    if(txState == 16) { //2.3msec have passed
        CYRF_ConfigRxTx(1); //Write mode
        if(pkt_num == 0) {
            radio_ch_ptr = radio_ch_ptr == &radio_ch[2] ? radio_ch : radio_ch_ptr + 1;
            CYRF_ConfigRFChannel(*radio_ch_ptr);
        }
        txState = 0;
    }
    return delay;
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
        cyrf_set_bound_sop_code();
    }   
    if(pkt_num == 0) {
        radio_ch_ptr = radio_ch_ptr == &radio_ch[2] ? radio_ch : radio_ch_ptr + 1;
        CYRF_ConfigRFChannel(*radio_ch_ptr);
    }
    return 1200;
}

static void bind()
{
    fixed_id = Model.fixed_id;
    bind_counter = BIND_COUNT;
    use_fixed_id = 1;
    PROTOCOL_SetBindState(0x1388 * 2400 / 1000); //msecs
}

static void initialize()
{
    CLOCK_StopTimer();
    CYRF_Reset();
    cyrf_init();
    CYRF_GetMfgData(cyrfmfg_id);
    CYRF_ConfigRxTx(1);
    CYRF_ConfigCRCSeed(0x0000);
    CYRF_ConfigSOPCode(sopcodes[0]);
    set_radio_channels();
    use_fixed_id = 0;
    radio_ch_ptr = radio_ch;
    memset(&Telemetry, 0, sizeof(Telemetry));
    /*
    parse_telemetry_packet("203020.8270E\0");
    parse_telemetry_packet("35954.776N\0\0");
    parse_telemetry_packet("412.8\0\0\0MMNE\0");
    parse_telemetry_packet("5\0\0\0\0\0\00.00\0\0");
    parse_telemetry_packet("6182552151012");
    */
    CYRF_ConfigRFChannel(*radio_ch_ptr);
    //FIXME: Properly setnumber of channels;
    num_channels = ((Model.num_channels + 3) >> 2) * 4;
    pkt_num = 0;
    ch_idx = 0;
    txState = 0;

    if(! Model.fixed_id) {
        fixed_id = ((u32)(radio_ch[0] ^ cyrfmfg_id[0] ^ cyrfmfg_id[3]) << 16)
                 | ((u32)(radio_ch[1] ^ cyrfmfg_id[1] ^ cyrfmfg_id[4]) << 8)
                 | ((u32)(radio_ch[2] ^ cyrfmfg_id[2] ^ cyrfmfg_id[5]) << 0);
        fixed_id = fixed_id % 1000000;
        bind_counter = BIND_COUNT;
        state = DEVO_BIND;
        PROTOCOL_SetBindState(0x1388 * 2400 / 1000); //msecs
    } else {
        fixed_id = Model.fixed_id;
        use_fixed_id = 0;
        state = DEVO_BOUND_1;
        bind_counter = 0;
        cyrf_set_bound_sop_code();
    }
    if (TELEMETRY) {
        CLOCK_StartTimer(2400, devo_telemetry_cb);
    } else {
        CLOCK_StartTimer(2400, devo_cb);
    }
}

const void *DEVO_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_CHECK_AUTOBIND: return Model.fixed_id ? 0 : (void *)1L;
        case PROTOCMD_BIND:  bind(); return 0;
        case PROTOCMD_NUMCHAN: return (void *)12L;
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)8L;
        case PROTOCMD_CURRENT_ID:  return (void *)((unsigned long)fixed_id);
        case PROTOCMD_SET_TXPOWER:
            CYRF_WriteRegister(CYRF_03_TX_CFG, 0x08 | Model.tx_power);
            break;
        default: break;
    }
    return 0;
}

#endif
