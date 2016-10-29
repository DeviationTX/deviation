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
    #define DSM2_Cmds PROTO_Cmds
    #pragma long_calls
#endif

#include <stdlib.h>
#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "telemetry.h"
#include "config/model.h"

#ifdef MODULAR
    #pragma long_calls_off
    extern unsigned _data_loadaddr;
    const unsigned long protocol_type = (unsigned long)&_data_loadaddr;
#endif

#ifdef PROTO_HAS_CYRF6936
#ifdef MODULAR 
    //Reduce size
    #define RANDOM_CHANNELS  0
#else
    #define RANDOM_CHANNELS  1
#endif

#define BIND_CHANNEL 0x0d //This can be any odd channel
#define MODEL 0

#define NUM_WAIT_LOOPS (100 / 5) //each loop is ~5us.  Do not wait more than 100us

static const char * const dsm_opts[] = {
    _tr_noop("Telemetry"),  _tr_noop("Off"), _tr_noop("On"), NULL, 
    _tr_noop("OrangeRx"),  _tr_noop("No"), _tr_noop("Yes"), NULL, 
    _tr_noop("HighSpeed"),  _tr_noop("Off"), _tr_noop("On"), NULL, 
#ifndef _DEVO7E_TARGET_H_
    _tr_noop("F.Log filter"),  _tr_noop("Off"), _tr_noop("On"), NULL, 
#endif
    NULL
};

enum {
    PROTOOPTS_TELEMETRY = 0,
    PROTOOPTS_ORANGERX,
    PROTOOPTS_HIGHSPEED,
#ifndef _DEVO7E_TARGET_H_
    PROTOOPTS_FLOGFILTER,
#endif
    LAST_PROTO_OPT,
};

ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

#define TELEM_ON 1
#define TELEM_OFF 0
#define ORANGERX_YES 1
#define ORANGERX_NO 0
#define HIGHSPEED_ON 1
#define HIGHSPEED_OFF 0
#ifndef _DEVO7E_TARGET_H_
    #define FLOGFILTER_ON 1
    #define FLOGFILTER_OFF 0
#endif

//During binding we will send BIND_COUNT/2 packets
//One packet each 10msec
#ifdef EMULATOR
    #define USE_FIXED_MFGID
    #define BIND_COUNT 2
#else
    #define BIND_COUNT 600
#endif

enum {
    DSM2_BIND = 0,
    DSM2_CHANSEL     = BIND_COUNT + 0,
    DSM2_CH1_WRITE_A = BIND_COUNT + 1,
    DSM2_CH1_CHECK_A = BIND_COUNT + 2,
    DSM2_CH2_WRITE_A = BIND_COUNT + 3,
    DSM2_CH2_CHECK_A = BIND_COUNT + 4,
    DSM2_CH2_READ_A  = BIND_COUNT + 5,
    DSM2_CH1_WRITE_B = BIND_COUNT + 6,
    DSM2_CH1_CHECK_B = BIND_COUNT + 7,
    DSM2_CH2_WRITE_B = BIND_COUNT + 8,
    DSM2_CH2_CHECK_B = BIND_COUNT + 9,
    DSM2_CH2_READ_B  = BIND_COUNT + 10,
};
   
static const u8 pncodes[5][9][8] = {
    /* Note these are in order transmitted (LSB 1st) */
{ /* Row 0 */
  /* Col 0 */ {0x03, 0xBC, 0x6E, 0x8A, 0xEF, 0xBD, 0xFE, 0xF8},
  /* Col 1 */ {0x88, 0x17, 0x13, 0x3B, 0x2D, 0xBF, 0x06, 0xD6},
  /* Col 2 */ {0xF1, 0x94, 0x30, 0x21, 0xA1, 0x1C, 0x88, 0xA9},
  /* Col 3 */ {0xD0, 0xD2, 0x8E, 0xBC, 0x82, 0x2F, 0xE3, 0xB4},
  /* Col 4 */ {0x8C, 0xFA, 0x47, 0x9B, 0x83, 0xA5, 0x66, 0xD0},
  /* Col 5 */ {0x07, 0xBD, 0x9F, 0x26, 0xC8, 0x31, 0x0F, 0xB8},
  /* Col 6 */ {0xEF, 0x03, 0x95, 0x89, 0xB4, 0x71, 0x61, 0x9D},
  /* Col 7 */ {0x40, 0xBA, 0x97, 0xD5, 0x86, 0x4F, 0xCC, 0xD1},
  /* Col 8 */ {0xD7, 0xA1, 0x54, 0xB1, 0x5E, 0x89, 0xAE, 0x86}
},
{ /* Row 1 */
  /* Col 0 */ {0x83, 0xF7, 0xA8, 0x2D, 0x7A, 0x44, 0x64, 0xD3},
  /* Col 1 */ {0x3F, 0x2C, 0x4E, 0xAA, 0x71, 0x48, 0x7A, 0xC9},
  /* Col 2 */ {0x17, 0xFF, 0x9E, 0x21, 0x36, 0x90, 0xC7, 0x82},
  /* Col 3 */ {0xBC, 0x5D, 0x9A, 0x5B, 0xEE, 0x7F, 0x42, 0xEB},
  /* Col 4 */ {0x24, 0xF5, 0xDD, 0xF8, 0x7A, 0x77, 0x74, 0xE7},
  /* Col 5 */ {0x3D, 0x70, 0x7C, 0x94, 0xDC, 0x84, 0xAD, 0x95},
  /* Col 6 */ {0x1E, 0x6A, 0xF0, 0x37, 0x52, 0x7B, 0x11, 0xD4},
  /* Col 7 */ {0x62, 0xF5, 0x2B, 0xAA, 0xFC, 0x33, 0xBF, 0xAF},
  /* Col 8 */ {0x40, 0x56, 0x32, 0xD9, 0x0F, 0xD9, 0x5D, 0x97}
},
{ /* Row 2 */
  /* Col 0 */ {0x40, 0x56, 0x32, 0xD9, 0x0F, 0xD9, 0x5D, 0x97},
  /* Col 1 */ {0x8E, 0x4A, 0xD0, 0xA9, 0xA7, 0xFF, 0x20, 0xCA},
  /* Col 2 */ {0x4C, 0x97, 0x9D, 0xBF, 0xB8, 0x3D, 0xB5, 0xBE},
  /* Col 3 */ {0x0C, 0x5D, 0x24, 0x30, 0x9F, 0xCA, 0x6D, 0xBD},
  /* Col 4 */ {0x50, 0x14, 0x33, 0xDE, 0xF1, 0x78, 0x95, 0xAD},
  /* Col 5 */ {0x0C, 0x3C, 0xFA, 0xF9, 0xF0, 0xF2, 0x10, 0xC9},
  /* Col 6 */ {0xF4, 0xDA, 0x06, 0xDB, 0xBF, 0x4E, 0x6F, 0xB3},
  /* Col 7 */ {0x9E, 0x08, 0xD1, 0xAE, 0x59, 0x5E, 0xE8, 0xF0},
  /* Col 8 */ {0xC0, 0x90, 0x8F, 0xBB, 0x7C, 0x8E, 0x2B, 0x8E}
},
{ /* Row 3 */
  /* Col 0 */ {0xC0, 0x90, 0x8F, 0xBB, 0x7C, 0x8E, 0x2B, 0x8E},
  /* Col 1 */ {0x80, 0x69, 0x26, 0x80, 0x08, 0xF8, 0x49, 0xE7},
  /* Col 2 */ {0x7D, 0x2D, 0x49, 0x54, 0xD0, 0x80, 0x40, 0xC1},
  /* Col 3 */ {0xB6, 0xF2, 0xE6, 0x1B, 0x80, 0x5A, 0x36, 0xB4},
  /* Col 4 */ {0x42, 0xAE, 0x9C, 0x1C, 0xDA, 0x67, 0x05, 0xF6},
  /* Col 5 */ {0x9B, 0x75, 0xF7, 0xE0, 0x14, 0x8D, 0xB5, 0x80},
  /* Col 6 */ {0xBF, 0x54, 0x98, 0xB9, 0xB7, 0x30, 0x5A, 0x88},
  /* Col 7 */ {0x35, 0xD1, 0xFC, 0x97, 0x23, 0xD4, 0xC9, 0x88},
  /* Col 8 */ {0xE1, 0xD6, 0x31, 0x26, 0x5F, 0xBD, 0x40, 0x93}
},
{ /* Row 4 */
  /* Col 0 */ {0xE1, 0xD6, 0x31, 0x26, 0x5F, 0xBD, 0x40, 0x93},
  /* Col 1 */ {0xDC, 0x68, 0x08, 0x99, 0x97, 0xAE, 0xAF, 0x8C},
  /* Col 2 */ {0xC3, 0x0E, 0x01, 0x16, 0x0E, 0x32, 0x06, 0xBA},
  /* Col 3 */ {0xE0, 0x83, 0x01, 0xFA, 0xAB, 0x3E, 0x8F, 0xAC},
  /* Col 4 */ {0x5C, 0xD5, 0x9C, 0xB8, 0x46, 0x9C, 0x7D, 0x84},
  /* Col 5 */ {0xF1, 0xC6, 0xFE, 0x5C, 0x9D, 0xA5, 0x4F, 0xB7},
  /* Col 6 */ {0x58, 0xB5, 0xB3, 0xDD, 0x0E, 0x28, 0xF1, 0xB0},
  /* Col 7 */ {0x5F, 0x30, 0x3B, 0x56, 0x96, 0x45, 0xF4, 0xA1},
  /* Col 8 */ {0x03, 0xBC, 0x6E, 0x8A, 0xEF, 0xBD, 0xFE, 0xF8}
},
};

static const u8 pn_bind[] = {0xD7, 0xA1, 0x54, 0xB1, 0x5E, 0x89, 0xAE, 0x86, 0xc6, 0x94, 0x22, 0xfe, 0x48, 0xe6, 0x57, 0x4e};

//static const u8 pn_bind_transmit[] = {0xD7, 0xA1, 0x54, 0xB1, 0x5E, 0x89, 0xAE, 0x86, 0xc6, 0x94, 0x22, 0xfe, 0x48, 0xe6, 0x57, 0x4e};
//static const u8 pn_bind_receive1[] = {0x98, 0x88, 0x1B, 0xE4, 0x30, 0x79, 0x03, 0x84, 0x06, 0x0C, 0x12, 0x18, 0x1E, 0x24, 0x71, 0x10};
//static const u8 pn_bind_receive2[] = {0x98, 0x88, 0x1B, 0xE4, 0x30, 0x79, 0x03, 0x84, 0xC9, 0x2C, 0x06, 0x93, 0x86, 0xB9, 0x9E, 0xD7};

//up to 7 channels
static const u8 ch_map7[] =  {1, 5, 2, 3, 0,  4,    6}; //DX6i
//from 8 channels up to 12 channels
static const u8 ch_map12[] = {1, 5, 2, 4, 6, 10, 0xff,    0, 7, 3, 8, 9, 11, 0xff}; //12ch - DX18
// "High Speed" 11ms for 8...10 channels
static const u8 ch_map14[] = {1, 5, 2, 3, 4,  6,    8,    1, 5, 2, 3, 0,  7,    9};

u8 packet[16];
u8 channels[23];
u8 chidx;
u8 sop_col;
u8 data_col;
u16 state;
u8 crcidx;
u8 binding;

#ifdef USE_FIXED_MFGID
    //static const u8 cyrfmfg_id[6] = {0x5e, 0x28, 0xa3, 0x1b, 0x00, 0x00}; //dx8
    static const u8 cyrfmfg_id[6] = {0xd4, 0x62, 0xd6, 0xad, 0xd3, 0xff}; //dx6i
#else
    static u8 cyrfmfg_id[6];
#endif

u8 num_channels;
u16 crc;
u8 model;

static void build_bind_packet()
{
    u8 i;
    u16 sum = 384 - 0x10;
    packet[0] = crc >> 8;
    packet[1] = crc & 0xff;
    packet[2] = 0xff ^ cyrfmfg_id[2];
    packet[3] = (0xff ^ cyrfmfg_id[3]) + model;
    packet[4] = packet[0];
    packet[5] = packet[1];
    packet[6] = packet[2];
    packet[7] = packet[3];
    for(i = 0; i < 8; i++)
        sum += packet[i];
    packet[8] = sum >> 8;
    packet[9] = sum & 0xff;
    packet[10] = 0x01; //???
    packet[11] = num_channels;
    if(Model.protocol == PROTOCOL_DSMX) {
#ifdef MODULAR
        packet[12] = 0xb2;
#else
        packet[12] = num_channels < 8 && Model.proto_opts[PROTOOPTS_TELEMETRY] == TELEM_OFF ? 0xa2 : 0xb2;
#endif
    } else {
        packet[12] = num_channels < 8 ? 0x01 : 0x02;
    }
    packet[13] = 0x00; //???
    for(i = 8; i < 14; i++)
        sum += packet[i];
    packet[14] = sum >> 8;
    packet[15] = sum & 0xff;
}

static void build_data_packet(u8 upper)
{
    u8 i;
    const u8 *chmap;
    
    if (binding && PROTOCOL_SticksMoved(0)) {
        //Don't turn off dialog until sticks are moved
        PROTOCOL_SetBindState(0);  //Turn off Bind dialog
        binding = 0;
    }
    if (Model.protocol == PROTOCOL_DSMX) {
        packet[0] = cyrfmfg_id[2];
        packet[1] = cyrfmfg_id[3] + model;
    } else {
        packet[0] = (0xff ^ cyrfmfg_id[2]);
        packet[1] = (0xff ^ cyrfmfg_id[3]) + model;
    }
    u8 bits = Model.protocol == PROTOCOL_DSMX ? 11 : 10;
    if(num_channels < 8) {
        chmap = ch_map7;
    } else if((num_channels < 11) && (Model.proto_opts[PROTOOPTS_HIGHSPEED] == HIGHSPEED_ON)) {
        chmap = ch_map14;
    } else {
        chmap = ch_map12;
    }
    u16 max = 1 << bits;
    u16 pct_100 = (u32)max * 100 / 150;
    for (i = 0; i < 7; i++) {
       unsigned idx = chmap[upper * 7 + i];
       s32 value;
       if ((chmap[upper*7 + i] == 0xff) || ((num_channels > 7) && (chmap[upper*7 + i] > num_channels - 1))) {
           value = 0xffff;
       } else {
           if (binding && Model.limits[idx].flags & CH_FAILSAFE_EN) {
               value = (s32)Model.limits[idx].failsafe * (pct_100 / 2) / 100 + (max / 2);
           } else {
               value = (s32)Channels[idx] * (pct_100 / 2) / CHAN_MAX_VALUE + (max / 2);
           }
           if (value >= max)
               value = max - 1;
           else if (value < 0)
               value = 0;
           value = (upper && i == 0 ? 0x8000 : 0) | (chmap[upper * 7 + i] << bits) | value;
       }
       packet[i*2+2] = (value >> 8) & 0xff;
       packet[i*2+3] = (value >> 0) & 0xff;
    }
}

static u8 get_pn_row(u8 channel)
{
    return Model.protocol == PROTOCOL_DSMX
           ? (channel - 2) % 5
           : channel % 5;
}

static const u8 init_vals[][2] = {
    //{CYRF_1D_MODE_OVERRIDE, 0x01},  //moved to CYRF_Reset()
    {CYRF_03_TX_CFG, 0x38 | 7},     //Data Code Length = 64 chip codes + Data Mode = SDR Mode + max-power(+4 dBm)
    {CYRF_06_RX_CFG, 0x4A},         //LNA + FAST TURN EN + RXOW EN, enable low noise amplifier, fast turning, overwrite enable
    {CYRF_12_DATA64_THOLD, 0x0a},   //TH64 = 0Ah, set pn correlation threshold (0Eh???)
    {CYRF_1B_TX_OFFSET_LSB, 0x55},  //STRIM LSB = 0x55, typical configuration
    {CYRF_1C_TX_OFFSET_MSB, 0x05},  //STRIM MSB = 0x05, typical configuration
    {CYRF_32_AUTO_CAL_TIME, 0x3c},  //AUTO_CAL_TIME = 3Ch, typical configuration
    {CYRF_35_AUTOCAL_OFFSET, 0x14}, //AUTO_CAL_OFFSET = 14h, typical configuration
    //{CYRF_39_ANALOG_CTRL, 0x01},    //ALL SLOW
    {CYRF_01_TX_LENGTH, 0x10},      //TX Length = 16 byte packet
    {CYRF_28_CLK_EN, 0x02},         //RXF, force receive clock enable ???
};

static void cyrf_startup_config()
{
    for(u32 i = 0; i < sizeof(init_vals) / 2; i++)
        CYRF_WriteRegister(init_vals[i][0], init_vals[i][1]);
    //If using 64-SDR, set number of preamble repetitions to four for optimum performance
    CYRF_WritePreamble(0x333304);
    //CYRF_ConfigRFChannel(0x61); // it's way out from used channels (0-79) and don't needed 
}

static const u8 bind_vals[][2] = {
    {CYRF_03_TX_CFG, 0x38 | 7},  //Data Code Length = 64 chip codes + Data Mode = SDR Mode + max-power(+4 dBm)
    {CYRF_10_FRAMING_CFG, 0x4a}, //SOP LEN + SOP TH = 0Ah (0Eh???)
    {CYRF_1E_RX_OVERRIDE, 0x14}, //FRC RXDR + DIS RXCRC (disable rx CRC)
    {CYRF_1F_TX_OVERRIDE, 0x04}, //DIS TXCRC (disable tx CRC)
    {CYRF_14_EOP_CTRL, 0x02},    //EOP = 2 (set EOP sync = 2)
};

static void cyrf_bind_config()
{
    for(u32 i = 0; i < sizeof(bind_vals) / 2; i++)
        CYRF_WriteRegister(bind_vals[i][0], bind_vals[i][1]);
}

void initialize_bind_state()
{
    cyrf_bind_config();
    CYRF_ConfigRFChannel(BIND_CHANNEL); //This seems to be random?
    //In 64-SDR mode, only the first eight bytes are used, but all sixteen bytes must be writed.
    CYRF_ConfigDataCode(pn_bind, 16);
    build_bind_packet();
}

static const u8 transfer_vals[][2] = {
    //{CYRF_29_RX_ABORT, 0x20},    //RX abort anable                   (RX mode abort in time Rx bind responce)
    //{CYRF_0F_XACT_CFG, 0x28},    //Force end state = Synth Mode (TX) (RX mode abort in time Rx bind responce)
    //{CYRF_29_RX_ABORT, 0x00},    //Clear RX abort                    (RX mode abort in time Rx bind responce)
    {CYRF_03_TX_CFG, 0x28 | 7},  //Data Code Length = 64 chip codes + Data Mode = 8DR Mode + max-power(+4 dBm)
    {CYRF_10_FRAMING_CFG, 0xea}, //SOP EN + SOP LEN = 64 chips + LEN EN + SOP TH = 0Ah (0Eh???)
    {CYRF_1E_RX_OVERRIDE, 0x00}, //Reset RX overrides
    {CYRF_1F_TX_OVERRIDE, 0x00}, //Reset TX overrides
};

static void cyrf_transfer_config()
{
    for(u32 i = 0; i < sizeof(transfer_vals) / 2; i++)
        CYRF_WriteRegister(transfer_vals[i][0], transfer_vals[i][1]);
}

static void set_sop_data_crc()
{
    u8 pn_row = get_pn_row(channels[chidx]);
    u8 data_code[16];
    //printf("Ch: %d Row: %d SOP: %d Data: %d\n", ch[chidx], pn_row, sop_col, data_col);
    CYRF_ConfigRFChannel(channels[chidx]);
    CYRF_ConfigCRCSeed(crcidx ? ~crc : crc);
    CYRF_ConfigSOPCode(pncodes[pn_row][sop_col]);
    memcpy(data_code, pncodes[pn_row][data_col], 8);
    if((pn_row == 3) && (data_col == 7) && (Model.proto_opts[PROTOOPTS_ORANGERX] == ORANGERX_YES)) {
        memcpy(data_code + 8, (void *)"\x88\xE1\xD6\x31\x26\x5F\xBD\x40", 8);
    } else {
        memcpy(data_code + 8, pncodes[pn_row][data_col + 1], 8);
    }
    //In 64-8DR mode, all sixteen bytes are used.
    CYRF_ConfigDataCode(data_code, 16);
    /* setup for next iteration */
    if(Model.protocol == PROTOCOL_DSMX)
        chidx = (chidx + 1) % 23;
    else
        chidx = (chidx + 1) % 2;
    crcidx = !crcidx;
}

static void calc_dsmx_channel()
{
    int idx = 0;
    u32 id = ~((cyrfmfg_id[0] << 24) | (cyrfmfg_id[1] << 16) | (cyrfmfg_id[2] << 8) | (cyrfmfg_id[3] << 0));
    u32 id_tmp = id;
    while(idx < 23) {
        int i;
        int count_3_27 = 0, count_28_51 = 0, count_52_76 = 0;
        id_tmp = id_tmp * 0x0019660D + 0x3C6EF35F; // Randomization
        u8 next_ch = ((id_tmp >> 8) % 0x49) + 3;   // Use least-significant byte and must be larger than 3
        if (((next_ch ^ id) & 0x01 )== 0)
            continue;
        for (i = 0; i < idx; i++) {
            if(channels[i] == next_ch)
                break;
            if(channels[i] <= 27)
                count_3_27++;
            else if (channels[i] <= 51)
                count_28_51++;
            else
                count_52_76++;
        }
        if (i != idx)
            continue;
        if ((next_ch < 28 && count_3_27 < 8)
          ||(next_ch >= 28 && next_ch < 52 && count_28_51 < 7)
          ||(next_ch >= 52 && count_52_76 < 8))
        {
            channels[idx++] = next_ch;
        }
    }
}

static u32 bcd_to_int(u32 data)
{
    u32 value = 0, multi = 1;
    while (data) {
        value += (data & 15U) * multi;
        multi *= 10;
        data >>= 4;
    }
    return value;
}

static int pkt32_to_coord(u8 *ptr)
{
    // (decimal, format DD MM.MMMM)
    return bcd_to_int(ptr[3]) * 3600000
         + bcd_to_int(((u32)ptr[2] << 16) | ((u32)ptr[1] << 8) | ptr[0]) * 6;
}
NO_INLINE static void parse_telemetry_packet()
{
    static u8 altitude; // byte from first GPS packet
#if HAS_EXTENDED_TELEMETRY
    static const u8 update0a[] = { TELEM_DSM_PBOX_VOLT1, TELEM_DSM_PBOX_VOLT2,
                                   TELEM_DSM_PBOX_CAPACITY1, TELEM_DSM_PBOX_CAPACITY2,
                                   TELEM_DSM_PBOX_ALARMV1, TELEM_DSM_PBOX_ALARMV2,
                                   TELEM_DSM_PBOX_ALARMC1, TELEM_DSM_PBOX_ALARMC2, 0};
    static const u8 update15[] = { TELEM_DSM_JETCAT_STATUS, TELEM_DSM_JETCAT_THROTTLE,
                                   TELEM_DSM_JETCAT_PACKVOLT, TELEM_DSM_JETCAT_PUMPVOLT,
                                   TELEM_DSM_JETCAT_RPM, TELEM_DSM_JETCAT_TEMPEGT,
                                   TELEM_DSM_JETCAT_OFFCOND, 0};
    static const u8 update20[] = { TELEM_DSM_ESC_RPM, TELEM_DSM_ESC_VOLT1,
                                   TELEM_DSM_ESC_TEMP1, TELEM_DSM_ESC_AMPS1,
                                   TELEM_DSM_ESC_TEMP2, TELEM_DSM_ESC_AMPS2,
                                   TELEM_DSM_ESC_VOLT2, TELEM_DSM_ESC_THROTTLE,
                                   TELEM_DSM_ESC_OUTPUT, 0};
    static const u8 update18[] = { TELEM_DSM_RXPCAP_AMPS, TELEM_DSM_RXPCAP_CAPACITY, TELEM_DSM_RXPCAP_VOLT, 0};
    static const u8 update34[] = { TELEM_DSM_FPCAP_AMPS, TELEM_DSM_FPCAP_CAPACITY, TELEM_DSM_FPCAP_TEMP, 0};
#endif
    static const u8 update16[] = { TELEM_GPS_ALT, TELEM_GPS_LAT, TELEM_GPS_LONG, TELEM_GPS_HEADING, 0};
    static const u8 update17[] = { TELEM_GPS_SPEED, TELEM_GPS_TIME, TELEM_GPS_SATCOUNT, 0};
    static const u8 update7f[] = { TELEM_DSM_FLOG_FADESA, TELEM_DSM_FLOG_FADESB,
                                   TELEM_DSM_FLOG_FADESL, TELEM_DSM_FLOG_FADESR,
                                   TELEM_DSM_FLOG_FRAMELOSS, TELEM_DSM_FLOG_HOLDS,
                                   TELEM_DSM_FLOG_VOLT1, 0};
    static const u8 update7e[] = { TELEM_DSM_FLOG_RPM1, TELEM_DSM_FLOG_VOLT2, TELEM_DSM_FLOG_TEMP1, 0};
    static const u8 update03[] = { TELEM_DSM_AMPS1, 0};
    static const u8 update11[] = { TELEM_DSM_AIRSPEED, 0};
    static const u8 update12[] = { TELEM_DSM_ALTITUDE, TELEM_DSM_ALTITUDE_MAX, 0};
    static const u8 update14[] = { TELEM_DSM_GFORCE_X, TELEM_DSM_GFORCE_Y, TELEM_DSM_GFORCE_Z,
                                   TELEM_DSM_GFORCE_XMAX, TELEM_DSM_GFORCE_YMAX, TELEM_DSM_GFORCE_ZMAX,
                                   TELEM_DSM_GFORCE_ZMIN, 0};
    static const u8 update40[] = { TELEM_DSM_VARIO_ALTITUDE, TELEM_DSM_VARIO_CLIMBRATE1,
                                   TELEM_DSM_VARIO_CLIMBRATE2, TELEM_DSM_VARIO_CLIMBRATE3,
                                   TELEM_DSM_VARIO_CLIMBRATE4, TELEM_DSM_VARIO_CLIMBRATE5,
                                   TELEM_DSM_VARIO_CLIMBRATE6, 0};
    const u8 *update = &update7f[7];
    unsigned idx = 0;

#define data_type  packet[0]
#define end_byte   packet[15]
#define LSB_1st    ((data_type >= 0x15 && data_type <= 0x18) || (data_type == 0x34))

    static u16 pktTelem[8];

    // Convert 8bit packet into 16bit equivalent
    if (LSB_1st) {
        for(u8 i = 1; i < 8; ++i) {
            pktTelem[i] = (packet[i*2+1] << 8) | packet[i*2];
        }
    } else {
        for(u8 i = 1; i < 8; ++i) {
            pktTelem[i] = (packet[i*2] << 8) | packet[i*2+1];
        }
    }
    switch(data_type) {
        case 0x7f: //TM1000 Flight log
        case 0xff: //TM1100 Flight log
#ifndef _DEVO7E_TARGET_H_
            //=================================================================
            // Filter out "Telemetry Range Warning" signals
            //=================================================================
            // if 'Holds'>=255 -> Telemetry Range Warning
            // if 'A' or 'B' or 'L' or 'R' not equal to its previous value 
            // and >=511 (usually =65535) -> Telemetry Range Warning
            // therefore A, B, L, R, F, H -> not valid (must be filtered).
            // Fades A is an 8-bit value, B, R, and L are 16-bit values. 
            //=================================================================
            if (Model.proto_opts[PROTOOPTS_FLOGFILTER] == FLOGFILTER_ON) {
                if(pktTelem[6] > 15) { //holds
                    break;
                } else if(pktTelem[6] > (u16)Telemetry.value[TELEM_DSM_FLOG_HOLDS]) {
                    update = update7f; //refresh "Flight Log" in case "Hold" state
                    break;
                }
                //if(pktTelem[1] > 255) //fadesA - unknown if it's right for third party Rx, so will use generic condition
                if((pktTelem[1] != 0xFFFF) && (pktTelem[1] > (u16)Telemetry.value[TELEM_DSM_FLOG_FADESA] + 510)) //fadesA
                    break;
                if((pktTelem[2] != 0xFFFF) && (pktTelem[2] > (u16)Telemetry.value[TELEM_DSM_FLOG_FADESB] + 510)) //fadesB
                    break;
                if((pktTelem[3] != 0xFFFF) && (pktTelem[3] > (u16)Telemetry.value[TELEM_DSM_FLOG_FADESL] + 510)) //fadesL
                    break;
                if((pktTelem[4] != 0xFFFF) && (pktTelem[4] > (u16)Telemetry.value[TELEM_DSM_FLOG_FADESR] + 510)) //fadesR
                    break;
                if(pktTelem[5] > (u16)Telemetry.value[TELEM_DSM_FLOG_FRAMELOSS] + 510) //frLoss
                    break;
            }
#endif
            update = update7f;
            break;
        case 0x03: //High Current sensor
            update = update03;
            break;
        case 0x11: //AirSpeed sensor
            update = update11;
            break;
        case 0x12: //Altimeter sensor
            update = update12;
            break;
        case 0x14: //G-Force sensor
            update = update14;
            break;
#if HAS_EXTENDED_TELEMETRY
        case 0x18: //RX Pack Cap sensor (SPMA9604)
            update = update18;
            break;
        case 0x34: //Flight Pack Cap sensor (SPMA9605)
            update = update34;
            break;
#endif
        case 0x40: //Variometer sensor (SPMA9589)
            update = update40;
            break;
    }
    if (*update) {
        while (*update) {
            Telemetry.value[*update] = pktTelem[++idx];
            if (pktTelem[idx] != 0xffff)
                TELEMETRY_SetUpdated(*update);
            update++;
        }
        return;
    }
    switch(data_type) {
#if HAS_EXTENDED_TELEMETRY
        case 0x0a: //Powerbox sensor
            update = update0a;
            Telemetry.value[TELEM_DSM_PBOX_VOLT1] = pktTelem[1]; //In 1/100 of Volts
            Telemetry.value[TELEM_DSM_PBOX_VOLT2] = pktTelem[2]; //In 1/100 of Volts
            Telemetry.value[TELEM_DSM_PBOX_CAPACITY1] = pktTelem[3]; //In mAh
            Telemetry.value[TELEM_DSM_PBOX_CAPACITY2] = pktTelem[4]; //In mAh
            Telemetry.value[TELEM_DSM_PBOX_ALARMV1] = end_byte & 0x01; //0 = disable, 1 = enable
            Telemetry.value[TELEM_DSM_PBOX_ALARMV2] = end_byte & 0x02; //0 = disable, 1 = enable
            Telemetry.value[TELEM_DSM_PBOX_ALARMC1] = end_byte & 0x04; //0 = disable, 1 = enable
            Telemetry.value[TELEM_DSM_PBOX_ALARMC2] = end_byte & 0x08; //0 = disable, 1 = enable
            break;
        case 0x15: //JetCat sensor
            update = update15;
            Telemetry.value[TELEM_DSM_JETCAT_STATUS]   = packet[2];
            Telemetry.value[TELEM_DSM_JETCAT_THROTTLE] = bcd_to_int(packet[3]);   //up to 159% (the upper nibble is 0-f, the lower nibble 0-9)
            Telemetry.value[TELEM_DSM_JETCAT_PACKVOLT] = bcd_to_int(pktTelem[2]); //In 1/100 of Volts
            Telemetry.value[TELEM_DSM_JETCAT_PUMPVOLT] = bcd_to_int(pktTelem[3]); //In 1/100 of Volts (low voltage)
            Telemetry.value[TELEM_DSM_JETCAT_RPM]      = bcd_to_int(pktTelem[4] & 0x0fff); //RPM up to 999999
            Telemetry.value[TELEM_DSM_JETCAT_TEMPEGT]  = bcd_to_int(pktTelem[6]); //EGT temp up to 999°C
            Telemetry.value[TELEM_DSM_JETCAT_OFFCOND]  = end_byte;
            break;
        case 0x20: //Electronic Speed Control
            update = update20;
            Telemetry.value[TELEM_DSM_ESC_RPM]   = pktTelem[1] * 10; //In rpm, 0-655340 (0xFFFF --> No data)
            Telemetry.value[TELEM_DSM_ESC_VOLT1] = pktTelem[2];      //Batt in 1/100 of Volts (Volt2) (0-655.34V) (0xFFFF --> No data)
            Telemetry.value[TELEM_DSM_ESC_TEMP1] = pktTelem[3];      //FET Temp in 1/10 of C degree (0-999.8C) (0xFFFF --> No data)
            Telemetry.value[TELEM_DSM_ESC_AMPS1] = pktTelem[4];      //In 1/100 Amp (0-655.34A) (0xFFFF --> No data)
            Telemetry.value[TELEM_DSM_ESC_TEMP2] = pktTelem[5];      //BEC Temp in 1/10 of C degree (0-999.8C) (0xFFFF --> No data)
            Telemetry.value[TELEM_DSM_ESC_AMPS2] = packet[12];       //BEC current in 1/10 Amp (0-25.4A) (0xFF ----> No data)
            Telemetry.value[TELEM_DSM_ESC_VOLT2] = packet[13] * 5;   //BEC voltage in 0.05V (0-12.70V) (0xFF ----> No data)
            Telemetry.value[TELEM_DSM_ESC_THROTTLE] = packet[14]* 5; //Throttle % in 0.5% (0-127%) (0xFF ----> No data)
            Telemetry.value[TELEM_DSM_ESC_OUTPUT] = end_byte * 5;    //Power Output % in 0.5% (0-127%) (0xFF ----> No data)
            break;
#endif //HAS_EXTENDED_TELEMETRY
        case 0x7e: //TM1000
        case 0xfe: //TM1100
            update = update7e;
            Telemetry.value[TELEM_DSM_FLOG_RPM1]  = (pktTelem[1] == 0xffff || pktTelem[1] < 200) ?  0 : (120000000 / 2 / pktTelem[1]);
            Telemetry.value[TELEM_DSM_FLOG_VOLT2] =  pktTelem[2];
            Telemetry.value[TELEM_DSM_FLOG_TEMP1] = (pktTelem[3] - 32) * 5 / 9; //Convert to °C
            break;
        case 0x16: //GPS sensor (always second GPS packet)
            update = update16;
            Telemetry.gps.altitude  = (bcd_to_int((altitude << 24) | ((u32)pktTelem[1] << 8))) 
                                                                   * ((end_byte & 0x80) ? -1 : 1); 
                                                                  	  //In m * 1000 (16Bit decimal, 1 unit is 0.1m)
                                                                  	  //1 = below sea level, 0 = above sea level
            Telemetry.gps.latitude  =  pkt32_to_coord(&packet[4]) * ((end_byte & 0x01) ? 1 : -1); //1 = N(+), 0 = S(-)
            Telemetry.gps.longitude = (pkt32_to_coord(&packet[8]) + ((end_byte & 0x04) ? 360000000 : 0)) //1 = +100 degrees
                                                                  * ((end_byte & 0x02) ? 1 : -1); //1 = E(+), 0 = W(-)
            Telemetry.gps.heading = bcd_to_int(pktTelem[6]); //In degrees (16Bit decimal, 1 unit is 0.1 degree)
            break;
        case 0x17: //GPS sensor (always first GPS packet)
            update = update17;
            Telemetry.gps.velocity = bcd_to_int(pktTelem[1]) * 5556 / 108; //In m/s * 1000
            //u8 ssec  = bcd_to_int(packet[4]);
            u8 sec   = bcd_to_int(packet[5]);
            u8 min   = bcd_to_int(packet[6]);
            u8 hour  = bcd_to_int(packet[7]);
            u8 day   = 0;
            u8 month = 0;
            u8 year  = 0; // + 2000
            Telemetry.gps.time = ((year & 0x3F) << 26)
                               | ((month & 0x0F) << 22)
                               | ((day & 0x1F) << 17)
                               | ((hour & 0x1F) << 12)
                               | ((min & 0x3F) << 6)
                               | ((sec & 0x3F) << 0);
            Telemetry.gps.satcount = bcd_to_int(packet[8]);
            altitude = packet[9];
            break;
    }
    idx = 0;
    while (*update) {
        if (pktTelem[++idx] != 0xffff)
            TELEMETRY_SetUpdated(*update);
        update++;
    }
}

MODULE_CALLTYPE
static u16 dsm2_cb()
{
#define CH1_CH2_DELAY 4010  // Time between write of channel 1 and channel 2
#define WRITE_DELAY   1550  // Time after write to verify write complete
#define READ_DELAY     600  // Time before write to check read state, and switch channels.
                            // Telemetry read+processing =~200us and switch channels =~300us
    if(state < DSM2_CHANSEL) {
        //Binding
        state += 1;
        if(state & 1) {
            //Send packet on even states
            //Note state has already incremented, so this is actually 'even' state
            CYRF_WriteDataPacket(packet);
            return 8500;
        } else {
            //Check status on odd states
            CYRF_ReadRegister(CYRF_04_TX_IRQ_STATUS);
            return 1500;
        }
    } else if(state < DSM2_CH1_WRITE_A) {
        //Select channels and configure for writing data
        //CYRF_FindBestChannels(ch, 2, 10, 1, 79);
        cyrf_transfer_config();
        CYRF_SetTxRxMode(TX_EN);
        chidx = 0;
        crcidx = 0;
        state = DSM2_CH1_WRITE_A;
        set_sop_data_crc();
        return 10000;
    } else if(state == DSM2_CH1_WRITE_A || state == DSM2_CH1_WRITE_B
           || state == DSM2_CH2_WRITE_A || state == DSM2_CH2_WRITE_B)
    {
        if (state == DSM2_CH1_WRITE_A || state == DSM2_CH1_WRITE_B)
            build_data_packet(state == DSM2_CH1_WRITE_B);
        CYRF_WriteDataPacket(packet);
        state++;
        return WRITE_DELAY;
    } else if(state == DSM2_CH1_CHECK_A || state == DSM2_CH1_CHECK_B) {
        int i = 0;
        u8 reg;
        while (! ((reg = CYRF_ReadRegister(CYRF_04_TX_IRQ_STATUS)) & 0x02)) {
            if (++i >= NUM_WAIT_LOOPS)
                break;
        }
        if (Model.proto_opts[PROTOOPTS_TELEMETRY] == TELEM_ON) {
            // reset cyrf6936 in case TX mode and RX mode freezed
            if (((reg & 0x22) == 0x20) || (CYRF_ReadRegister(CYRF_02_TX_CTRL) & 0x80)) {
                CYRF_Reset();
                cyrf_startup_config();
                cyrf_transfer_config();
                CYRF_SetTxRxMode(TX_EN);
                //printf(" Rst CYRF\n");
            }
        }
        set_sop_data_crc();
        state++;
        return CH1_CH2_DELAY - WRITE_DELAY;
    } else if(state == DSM2_CH2_CHECK_A || state == DSM2_CH2_CHECK_B) {
        int i = 0;
        while (! (CYRF_ReadRegister(CYRF_04_TX_IRQ_STATUS) & 0x02)) {
            if(++i > NUM_WAIT_LOOPS)
                break;
        }
        if (state == DSM2_CH2_CHECK_A) {
            //Keep transmit power in sync
            CYRF_WriteRegister(CYRF_03_TX_CFG, 0x28 | Model.tx_power); //Data Code Length = 64 chip codes + Data Mode = 8DR Mode + tx_power
        }
#ifndef MODULAR
        if (Model.proto_opts[PROTOOPTS_TELEMETRY] == TELEM_OFF) {
            set_sop_data_crc();
            if (state == DSM2_CH2_CHECK_A) {
                if(num_channels < 8) {
                    state = DSM2_CH1_WRITE_A;
                    return 22000 - CH1_CH2_DELAY - WRITE_DELAY;
                }
                state = DSM2_CH1_WRITE_B;
            } else {
                state = DSM2_CH1_WRITE_A;
            }
            return 11000 - CH1_CH2_DELAY - WRITE_DELAY;
        } else
#endif
        {
            state++;
            CYRF_SetTxRxMode(RX_EN); //Receive mode
            CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x80); //Prepare to receive
            return 11000 - CH1_CH2_DELAY - WRITE_DELAY - READ_DELAY;
        }
    } else if(state == DSM2_CH2_READ_A || state == DSM2_CH2_READ_B) {
        //Read telemetry if needed
        u8 rx_state = CYRF_ReadRegister(CYRF_07_RX_IRQ_STATUS);
        if((rx_state & 0x03) == 0x02) {  // RXC=1, RXE=0 then 2nd check is required (debouncing)
            rx_state |= CYRF_ReadRegister(CYRF_07_RX_IRQ_STATUS);   
        }
        if((rx_state & 0x07) == 0x02) { // good data (complete with no errors)
            CYRF_WriteRegister(CYRF_07_RX_IRQ_STATUS, 0x80); // need to set RXOW before data read
            CYRF_ReadDataPacketLen(packet, CYRF_ReadRegister(CYRF_09_RX_COUNT));
            //rssi = CYRF_ReadRegister(CYRF_13_RSSI) & 0x1F; // RSSI of the received telemetry signal
            parse_telemetry_packet();
        }
        if (state == DSM2_CH2_READ_A && num_channels < 8) {
            state = DSM2_CH2_READ_B;
            //Reseat RX mode just in case any error
            CYRF_WriteRegister(CYRF_0F_XACT_CFG, (CYRF_ReadRegister(CYRF_0F_XACT_CFG) | 0x20));  // Force end state
            int i = 0;
            while (CYRF_ReadRegister(CYRF_0F_XACT_CFG) & 0x20) {
                if(++i > NUM_WAIT_LOOPS)
                    break;
            }
            CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x80);  //Prepare to receive
            return 11000;
        }
        if (state == DSM2_CH2_READ_A)
            state = DSM2_CH1_WRITE_B;
        else
            state = DSM2_CH1_WRITE_A;
        CYRF_SetTxRxMode(TX_EN); //Write mode
        set_sop_data_crc();
        return READ_DELAY;
    } 
    return 0;
}

static void initialize(u8 bind)
{
    CLOCK_StopTimer();
    CYRF_Reset();
#ifndef USE_FIXED_MFGID
    CYRF_GetMfgData(cyrfmfg_id);
    if (Model.fixed_id) {
        cyrfmfg_id[0] ^= (Model.fixed_id >> 0) & 0xff;
        cyrfmfg_id[1] ^= (Model.fixed_id >> 8) & 0xff;
        cyrfmfg_id[2] ^= (Model.fixed_id >> 16) & 0xff;
        cyrfmfg_id[3] ^= (Model.fixed_id >> 24) & 0xff;
    }
#endif
    
    cyrf_startup_config();

    if (Model.protocol == PROTOCOL_DSMX) {
        calc_dsmx_channel();
    } else {
        if (RANDOM_CHANNELS) {
            u8 tmpch[10];
            CYRF_FindBestChannels(tmpch, 10, 5, 3, 75);
            u8 idx = rand32() % 10;
            channels[0] = tmpch[idx];
            while(1) {
               idx = rand32() % 10;
               if (tmpch[idx] != channels[0])
                   break;
            }
            channels[1] = tmpch[idx];
        } else {
            channels[0] = (cyrfmfg_id[0] + cyrfmfg_id[2] + cyrfmfg_id[4]
                          + ((Model.fixed_id >> 0) & 0xff) + ((Model.fixed_id >> 16) & 0xff)) % 39 + 1;
            channels[1] = (cyrfmfg_id[1] + cyrfmfg_id[3] + cyrfmfg_id[5]
                          + ((Model.fixed_id >> 8) & 0xff) + ((Model.fixed_id >> 8) & 0xff)) % 40 + 40;
        }
    }
    /*
    channels[0] = 0;
    channels[1] = 0;
    if (Model.fixed_id == 0)
        Model.fixed_id = 0x2b9d2952;
    cyrfmfg_id[0] = 0xff ^ ((Model.fixed_id >> 24) & 0xff);
    cyrfmfg_id[1] = 0xff ^ ((Model.fixed_id >> 16) & 0xff);
    cyrfmfg_id[2] = 0xff ^ ((Model.fixed_id >> 8) & 0xff);
    cyrfmfg_id[3] = 0xff ^ ((Model.fixed_id >> 0) & 0xff);
    printf("DSM2 Channels: %02x %02x\n", channels[0], channels[1]);
    */
    crc = ~((cyrfmfg_id[0] << 8) + cyrfmfg_id[1]);
    crcidx = 0;
    sop_col = (cyrfmfg_id[0] + cyrfmfg_id[1] + cyrfmfg_id[2] + 2) & 0x07;
    data_col = 7 - sop_col;
    model = MODEL;
    num_channels = Model.num_channels;
    if (num_channels < 6)
        num_channels = 6;
    else if (num_channels > 12)
        num_channels = 12;

    memset(&Telemetry, 0, sizeof(Telemetry));
    TELEMETRY_SetType(TELEM_DSM);
    if (bind) {
        state = DSM2_BIND;
        PROTOCOL_SetBindState((BIND_COUNT > 200 ? BIND_COUNT / 2 : 200) * 10); //msecs
        initialize_bind_state();
        binding = 1;
    } else {
        state = DSM2_CHANSEL;
        binding = 0;
    }
    CYRF_SetTxRxMode(TX_EN);
    CLOCK_StartTimer(10000, dsm2_cb);
}

const void *DSM2_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(0); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(CYRF_Reset() ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND: return 0; //Never Autobind
        case PROTOCMD_BIND:  initialize(1); return 0;
        case PROTOCMD_NUMCHAN: return (void *)12L;
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)7L;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
#ifdef MODULAR
        case PROTOCMD_TELEMETRYSTATE:
            return (void *)(long)(PROTO_TELEM_ON);
#else
        case PROTOCMD_GETOPTIONS:
            return dsm_opts;
        case PROTOCMD_TELEMETRYSTATE:
            return (void *)(long)(Model.proto_opts[PROTOOPTS_TELEMETRY] == TELEM_ON ? PROTO_TELEM_ON : PROTO_TELEM_OFF);
#endif
        default: break;
    }
    return NULL;
}

#endif
