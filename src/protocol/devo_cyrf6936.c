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
  #define DEVO_Cmds PROTO_Cmds
  #pragma long_calls
#endif

#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "telemetry.h"
#include "config/model.h"

#ifdef MODULAR
  //Some versions of gcc applythis to definitions, others to calls
  //So just use long_calls everywhere
  //#pragma no_long_calls
  extern unsigned _data_loadaddr;
  const unsigned long protocol_type = (unsigned long)&_data_loadaddr;
#endif

#ifdef PROTO_HAS_CYRF6936

//For Debug
//#define NO_SCRAMBLE

#define PKTS_PER_CHANNEL 4

#ifdef EMULATOR
#include <stdlib.h>
#define BIND_COUNT 4
#else
#define BIND_COUNT 0x1388
#endif

#define TELEMETRY_ENABLE 0x30

#define NUM_WAIT_LOOPS (100 / 5) //each loop is ~5us.  Do not wait more than 100us
static const char * const devo_opts[] = {
  _tr_noop("Telemetry"),  _tr_noop("On"), _tr_noop("Off"), NULL,
  NULL
};
enum {
    PROTOOPTS_TELEMETRY = 0,
    LAST_PROTO_OPT,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

#define TELEM_ON 0
#define TELEM_OFF 1

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
static u8 failsafe_pkt;

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

static void build_beacon_pkt(int upper)
{
    packet[0] = ((num_channels << 4) | 0x07);
    u8 enable = 0;
    int max = 8;
    int offset = 0;
    if (upper) {
        packet[0] += 1;
        max = 4;
        offset = 8;
    }
    for(int i = 0; i < max; i++) {
        if (i + offset < Model.num_channels && Model.limits[i+offset].flags & CH_FAILSAFE_EN) {
            enable |= 0x80 >> i;
            packet[i+1] = Model.limits[i+offset].failsafe;
        } else {
            packet[i+1] = 0;
        }
    }
    packet[9] = enable;
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
static u32 text_to_int(u8 *ptr, u8 len)
{
    u32 value = 0;
    for(int i = 0; i < len; i++) {
        value = value * 10 + (ptr[i] - '0');
    }
    return value;
}
static void parse_telemetry_packet()
{
    static const u8 voltpkt[] = {
            TELEM_DEVO_VOLT1, TELEM_DEVO_VOLT2, TELEM_DEVO_VOLT3,
            TELEM_DEVO_RPM1, TELEM_DEVO_RPM2, 0
        };
    static const u8 temppkt[] = {
            TELEM_DEVO_TEMP1, TELEM_DEVO_TEMP2, TELEM_DEVO_TEMP3, TELEM_DEVO_TEMP4, 0
        };
    static const u8 gpslongpkt[] = { TELEM_GPS_LONG, 0};
    static const u8 gpslatpkt[] = { TELEM_GPS_LAT, 0};
    static const u8 gpsaltpkt[] = { TELEM_GPS_ALT, 0};
    static const u8 gpsspeedpkt[] = { TELEM_GPS_SPEED, 0};
    static const u8 gpstimepkt[] = { TELEM_GPS_TIME, 0};

    scramble_pkt(); //This will unscramble the packet
    if (((packet[0] & 0xF0) != TELEMETRY_ENABLE) ||
        ((((u32)packet[15] << 16) | ((u32)packet[14] << 8) | packet[13]) != (fixed_id & 0x00ffffff)))
    {
        return;
    }
    const u8 *update = &gpstimepkt[1];
    u32 step = 1;
    u32 idx = 1;
    //if (packet[0] < 0x37) {
    //    memcpy(Telemetry.line[packet[0]-0x30], packet+1, 12);
    //}
    if (packet[0] == TELEMETRY_ENABLE) {
        update = voltpkt;
        step = 2;
        Telemetry.value[TELEM_DEVO_VOLT1] = packet[1]; //In 1/10 of Volts
        Telemetry.value[TELEM_DEVO_VOLT2] = packet[3]; //In 1/10 of Volts
        Telemetry.value[TELEM_DEVO_VOLT3] = packet[5]; //In 1/10 of Volts
        Telemetry.value[TELEM_DEVO_RPM1]  = packet[7] * 120; //In RPM
        Telemetry.value[TELEM_DEVO_RPM2]  = packet[9] * 120; //In RPM
    }
    if (packet[0] == 0x31) {
        update = temppkt;
        while (*update) {
            Telemetry.value[*update] = packet[idx];
            if (packet[idx++] != 0xff)
                TELEMETRY_SetUpdated(*update);
            update++;
        }
        return;
    }
    /* GPS Data
       32: 30333032302e3832373045fb  = 030°20.8270E
       33: 353935342e373737364e0700  = 59°54.776N
       34: 31322e380000004d4d4e45fb  = 12.8 MMNE (altitude maybe)?
       35: 000000000000302e30300000  = 0.00 (probably speed)
       36: 313832353532313531303132  = 2012-10-15 18:25:52 (UTC)
    */
    if (packet[0] == 0x32) {
        update = gpslongpkt;
        Telemetry.gps.longitude = text_to_int(&packet[1], 3) * 3600000
                                + text_to_int(&packet[4], 2) * 60000
                                + text_to_int(&packet[7], 4) * 6;
        if (packet[11] == 'W')
            Telemetry.gps.longitude *= -1;
    }
    if (packet[0] == 0x33) {
        update = gpslatpkt;
        Telemetry.gps.latitude = text_to_int(&packet[1], 2) * 3600000
                               + text_to_int(&packet[3], 2) * 60000
                               + text_to_int(&packet[6], 4) * 6;
        if (packet[10] == 'S')
            Telemetry.gps.latitude *= -1;
    }
    if (packet[0] == 0x34) {
        update = gpsaltpkt;
        Telemetry.gps.altitude = float_to_int(&packet[1]);
    }
    if (packet[0] == 0x35) {
        update = gpsspeedpkt;
        Telemetry.gps.velocity = float_to_int(&packet[7]);
        idx = 7;
    }
    if (packet[0] == 0x36) {
        update = gpstimepkt;
        u8 hour  = text_to_int(&packet[1], 2);
        u8 min   = text_to_int(&packet[3], 2);
        u8 sec   = text_to_int(&packet[5], 2);
        u8 day   = text_to_int(&packet[7], 2);
        u8 month = text_to_int(&packet[9], 2);
        u8 year  = text_to_int(&packet[11], 2); // + 2000
        Telemetry.gps.time = ((year & 0x3F) << 26)
                           | ((month & 0x0F) << 22)
                           | ((day & 0x1F) << 17)
                           | ((hour & 0x1F) << 12)
                           | ((min & 0x3F) << 6)
                           | ((sec & 0x3F) << 0);
    }
    while (*update) {
        if (packet[idx])
            TELEMETRY_SetUpdated(*update);
        update++;
        idx += step;
    }
}

static void cyrf_set_bound_sop_code()
{
    /* crc == 0 isn't allowed, so use 1 if the math results in 0 */
    u8 crc = (cyrfmfg_id[0] + (cyrfmfg_id[1] >> 6) + cyrfmfg_id[2]);
    if(! crc)
        crc = 1;
    u8 sopidx = (0xff &((cyrfmfg_id[0] << 2) + cyrfmfg_id[1] + cyrfmfg_id[2])) % 10;
    CYRF_SetTxRxMode(TX_EN);
    CYRF_ConfigCRCSeed((crc << 8) + crc);
    CYRF_ConfigSOPCode(sopcodes[sopidx]);
    CYRF_WriteRegister(CYRF_03_TX_CFG, 0x08 | Model.tx_power);
}

static void cyrf_init()
{
    /* Initialise CYRF chip */
    CYRF_WriteRegister(CYRF_1D_MODE_OVERRIDE, 0x38);
    CYRF_WriteRegister(CYRF_03_TX_CFG, 0x08 | Model.tx_power);
    CYRF_WriteRegister(CYRF_06_RX_CFG, 0x4A);
    CYRF_WriteRegister(CYRF_0B_PWR_CTRL, 0x00);
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
    static unsigned cnt = 0;
    char foo[20];
    snprintf(foo, 20, "%d: %d", cnt++, state);
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
            build_beacon_pkt(num_channels > 8 ? failsafe_pkt : 0);
            failsafe_pkt = failsafe_pkt ? 0 : 1;
            scramble_pkt();
            state = DEVO_BOUND_1;
            break;
    }
    pkt_num++;
    if(pkt_num == PKTS_PER_CHANNEL)
        pkt_num = 0;
}
MODULE_CALLTYPE
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
        int i = 0;
        u8 reg;
        while (! ((reg = CYRF_ReadRegister(0x04)) & 0x02)) {
            if (++i >= NUM_WAIT_LOOPS)
                break;
        }
        if (((reg & 0x22) == 0x20) || (CYRF_ReadRegister(0x02) & 0x80)) {
     	    CYRF_Reset();
            cyrf_init();
            cyrf_set_bound_sop_code();
            CYRF_ConfigRFChannel(*radio_ch_ptr);
            //printf("Rst CYRF\n");
            delay = 1500;
            txState = 15;
        } else {
            if (state == DEVO_BOUND) {
                /* exit binding state */
                state = DEVO_BOUND_3;
                cyrf_set_bound_sop_code();
            }
            if(pkt_num == 0 || bind_counter > 0) {
                delay = 1500;
                txState = 15;
            } else {
                CYRF_SetTxRxMode(RX_EN); //Receive mode
                CYRF_WriteRegister(CYRF_07_RX_IRQ_STATUS, 0x80); //Prepare to receive
                CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x80); //Prepare to receive (do not enable any IRQ)
            }
        }
    } else {
        int reg = CYRF_ReadRegister(0x07);
        if ((reg & 0x23) == 0x22)
        //if(CYRF_ReadRegister(0x07) & 0x20)
        { // this won't be true in emulator so we need to simulate it somehow
            CYRF_ReadDataPacket(packet);
            parse_telemetry_packet(packet);
            delay = 100 * (16 - txState);
            txState = 15;
        }
#ifdef EMULATOR
        u8 telem_bit = rand32() % 7; // random number in [0, 7)
        packet[0] =  TELEMETRY_ENABLE + telem_bit; // allow emulator to simulate telemetry parsing to prevent future bugs in the telemetry monitor
        //printf("telem 1st packet: 0x%x\n", packet[0]);
        for(int i = 1; i < 13; i++)
            packet[i] = rand32() % 256;
        parse_telemetry_packet(packet);
        for(int i = 0; i < TELEM_UPDATE_SIZE; i++)
            Telemetry.updated[i] = 0xff;
        delay = 100 * (16 - txState);
        txState = 15;
#endif
    }
    txState++;
    if(txState == 16) { //2.3msec have passed
        CYRF_SetTxRxMode(TX_EN); //Write mode
        if(pkt_num == 0) {
            //Keep tx power updated
            CYRF_WriteRegister(CYRF_03_TX_CFG, 0x08 | Model.tx_power);
            radio_ch_ptr = radio_ch_ptr == &radio_ch[2] ? radio_ch : radio_ch_ptr + 1;
            CYRF_ConfigRFChannel(*radio_ch_ptr);
        }
        txState = 0;
    }
    return delay;
}

MODULE_CALLTYPE
static u16 devo_cb()
{
    if (txState == 0) {
        txState = 1;
        DEVO_BuildPacket();
        CYRF_WriteDataPacket(packet);
        return 1200;
    }
    txState = 0;
    int i = 0;
    while (! (CYRF_ReadRegister(0x04) & 0x02)) {
        if(++i > NUM_WAIT_LOOPS)
            return 1200;
    }
    if (state == DEVO_BOUND) {
        /* exit binding state */
        state = DEVO_BOUND_3;
        cyrf_set_bound_sop_code();
    }   
    if(pkt_num == 0) {
        //Keep tx power updated
        CYRF_WriteRegister(CYRF_03_TX_CFG, 0x08 | Model.tx_power);
        radio_ch_ptr = radio_ch_ptr == &radio_ch[2] ? radio_ch : radio_ch_ptr + 1;
        CYRF_ConfigRFChannel(*radio_ch_ptr);
    }
    return 1200;
}

static void devo_bind()
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
    CYRF_SetTxRxMode(TX_EN);
    CYRF_ConfigCRCSeed(0x0000);
    CYRF_ConfigSOPCode(sopcodes[0]);
    set_radio_channels();
    use_fixed_id = 0;
    failsafe_pkt = 0;
    radio_ch_ptr = radio_ch;
    memset(&Telemetry, 0, sizeof(Telemetry));
    TELEMETRY_SetType(TELEM_DEVO);
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
        use_fixed_id = 1;
        state = DEVO_BOUND_1;
        bind_counter = 0;
        cyrf_set_bound_sop_code();
    }
    if (Model.proto_opts[PROTOOPTS_TELEMETRY] == TELEM_ON) {
        CLOCK_StartTimer(2400, devo_telemetry_cb);
    } else {
        CLOCK_StartTimer(2400, devo_cb);
    }
}

const void *DEVO_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(CYRF_Reset() ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND: return Model.fixed_id ? 0 : (void *)1L;
        case PROTOCMD_BIND:  devo_bind(); return 0;
        case PROTOCMD_NUMCHAN: return (void *)12L;
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)8L;
        case PROTOCMD_CURRENT_ID:  return (void *)((unsigned long)fixed_id);
        case PROTOCMD_GETOPTIONS:
            return devo_opts;
        case PROTOCMD_SETOPTIONS:
            PROTOCOL_Init(0);  // only 1 prot_ops item, it is to enable/disable telemetry
            break;
        case PROTOCMD_TELEMETRYSTATE:
            return (void *)(long)(Model.proto_opts[PROTOOPTS_TELEMETRY] == TELEM_ON ? PROTO_TELEM_ON : PROTO_TELEM_OFF);
        default: break;
    }
    return 0;
}

#endif
