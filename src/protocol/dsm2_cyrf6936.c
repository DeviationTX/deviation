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
  NULL
};
enum {
    PROTOOPTS_TELEMETRY = 0,
    LAST_PROTO_OPT,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);
#define TELEM_ON 1
#define TELEM_OFF 0

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
  /* Col 8 */ {0x88, 0xE1, 0xD6, 0x31, 0x26, 0x5F, 0xBD, 0x40}
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

static const u8 pn_bind[] = { 0xc6,0x94,0x22,0xfe,0x48,0xe6,0x57,0x4e };

static const u8 ch_map4[] = {0, 1, 2, 3, 0xff, 0xff, 0xff};    //Guess
static const u8 ch_map5[] = {0, 1, 2, 3, 4,    0xff, 0xff}; //Guess
static const u8 ch_map6[] = {1, 5, 2, 3, 0,    4,    0xff}; //HP6DSM
static const u8 ch_map7[] = {1, 5, 2, 4, 3,    6,    0}; //DX6i
static const u8 ch_map8[] = {1, 5, 2, 3, 6,    0xff, 0xff, 4, 0, 7,    0xff, 0xff, 0xff, 0xff}; //DX8
static const u8 ch_map9[] = {3, 2, 1, 5, 0,    4,    6,    7, 8, 0xff, 0xff, 0xff, 0xff, 0xff}; //DM9
static const u8 ch_map10[] = {3, 2, 1, 5, 0,    4,    6,    7, 8, 9, 0xff, 0xff, 0xff, 0xff};
static const u8 ch_map11[] = {3, 2, 1, 5, 0,    4,    6,    7, 8, 9, 10, 0xff, 0xff, 0xff};
static const u8 ch_map12[] = {3, 2, 1, 5, 0,    4,    6,    7, 8, 9, 10, 11, 0xff, 0xff};
static const u8 * const ch_map[] = {ch_map4, ch_map5, ch_map6, ch_map7, ch_map8, ch_map9, ch_map10, ch_map11, ch_map12};

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
    const u8 *chmap = ch_map[num_channels - 4];
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
    u16 max = 1 << bits;
    u16 pct_100 = (u32)max * 100 / 150;
    for (i = 0; i < 7; i++) {
       unsigned idx = chmap[upper * 7 + i];
       s32 value;
       if (chmap[upper*7 + i] == 0xff) {
           value = 0xffff;
       } else {
           if (binding && Model.limits[idx].flags & CH_FAILSAFE_EN) {
               value = (s32)Model.limits[idx].failsafe * (pct_100 / 2) / 100 + (max / 2);
           } else {
               value = (s32)Channels[idx] * (pct_100 / 2) / CHAN_MAX_VALUE + (max / 2);
           }
           if (value >= max)
               value = max-1;
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
    {CYRF_02_TX_CTRL, 0x00},
    {CYRF_05_RX_CTRL, 0x00},
    {CYRF_28_CLK_EN, 0x02},
    {CYRF_32_AUTO_CAL_TIME, 0x3c},
    {CYRF_35_AUTOCAL_OFFSET, 0x14},
    {CYRF_06_RX_CFG, 0x48},
    {CYRF_1B_TX_OFFSET_LSB, 0x55},
    {CYRF_1C_TX_OFFSET_MSB, 0x05},
    {CYRF_0F_XACT_CFG, 0x24},
    {CYRF_03_TX_CFG, 0x38 | 7},
    {CYRF_12_DATA64_THOLD, 0x0a},
    {CYRF_0F_XACT_CFG, 0x04},
    {CYRF_39_ANALOG_CTRL, 0x01},
    {CYRF_0F_XACT_CFG, 0x24}, //Force IDLE
    {CYRF_29_RX_ABORT, 0x00}, //Clear RX abort
    {CYRF_12_DATA64_THOLD, 0x0a}, //set pn correlation threshold
    {CYRF_10_FRAMING_CFG, 0x4a}, //set sop len and threshold
    {CYRF_29_RX_ABORT, 0x0f}, //Clear RX abort?
    {CYRF_03_TX_CFG, 0x38 | 7}, //Set 64chip, SDE mode, max-power
    {CYRF_10_FRAMING_CFG, 0x4a}, //set sop len and threshold
    {CYRF_1F_TX_OVERRIDE, 0x04}, //disable tx CRC
    {CYRF_1E_RX_OVERRIDE, 0x14}, //disable rx crc
    {CYRF_14_EOP_CTRL, 0x02}, //set EOP sync == 2
    {CYRF_01_TX_LENGTH, 0x10}, //16byte packet
};

static void cyrf_config()
{
    for(u32 i = 0; i < sizeof(init_vals) / 2; i++)
        CYRF_WriteRegister(init_vals[i][0], init_vals[i][1]);
    CYRF_WritePreamble(0x333304);
    CYRF_ConfigRFChannel(0x61);
}

void initialize_bind_state()
{
    u8 data_code[32];
    CYRF_ConfigRFChannel(BIND_CHANNEL); //This seems to be random?
    u8 pn_row = get_pn_row(BIND_CHANNEL);
    //printf("Ch: %d Row: %d SOP: %d Data: %d\n", BIND_CHANNEL, pn_row, sop_col, data_col);
    CYRF_ConfigCRCSeed(crc);
    CYRF_ConfigSOPCode(pncodes[pn_row][sop_col]);
    memcpy(data_code, pncodes[pn_row][data_col], 16);
    memcpy(data_code + 16, pncodes[0][8], 8);
    memcpy(data_code + 24, pn_bind, 8);
    CYRF_ConfigDataCode(data_code, 32);
    build_bind_packet();
}

static const u8 data_vals[][2] = {
    {CYRF_05_RX_CTRL, 0x83}, //Initialize for reading RSSI
    {CYRF_29_RX_ABORT, 0x20},
    {CYRF_0F_XACT_CFG, 0x24},
    {CYRF_29_RX_ABORT, 0x00},
    {CYRF_03_TX_CFG, 0x08 | 7},
    {CYRF_10_FRAMING_CFG, 0xea},
    {CYRF_1F_TX_OVERRIDE, 0x00},
    {CYRF_1E_RX_OVERRIDE, 0x00},
    {CYRF_03_TX_CFG, 0x28 | 7},
    {CYRF_12_DATA64_THOLD, 0x3f},
    {CYRF_10_FRAMING_CFG, 0xff},
    {CYRF_0F_XACT_CFG, 0x24}, //Switch from reading RSSI to Writing
    {CYRF_29_RX_ABORT, 0x00},
    {CYRF_12_DATA64_THOLD, 0x0a},
    {CYRF_10_FRAMING_CFG, 0xea},
};

static void cyrf_configdata()
{
    for(u32 i = 0; i < sizeof(data_vals) / 2; i++)
        CYRF_WriteRegister(data_vals[i][0], data_vals[i][1]);
}

static void set_sop_data_crc()
{
    u8 pn_row = get_pn_row(channels[chidx]);
    //printf("Ch: %d Row: %d SOP: %d Data: %d\n", ch[chidx], pn_row, sop_col, data_col);
    CYRF_ConfigRFChannel(channels[chidx]);
    CYRF_ConfigCRCSeed(crcidx ? ~crc : crc);
    CYRF_ConfigSOPCode(pncodes[pn_row][sop_col]);
    CYRF_ConfigDataCode(pncodes[pn_row][data_col], 16);
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
        u8 next_ch = ((id_tmp >> 8) % 0x49) + 3;       // Use least-significant byte and must be larger than 3
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

static int bcd_to_u8(u8 data)
{
     return (data >> 4) * 10 + (data & 0x0f);
}
static int pkt16_to_u8(u8 *ptr)
{
    u32 value = ((u32)ptr[0] <<8) | ptr[1];
    return value > 255 ? 255 : value;
}
static u32 pkt16_to_volt(u8 *ptr)
{
    return ((((u32)ptr[0] << 8) | ptr[1]) + 5) / 10;  //In 1/10 of Volts
}
static int pkt16_to_flog(u8 *ptr)
{
    u32 value = ((u32)ptr[0] <<8) | ptr[1];
    return value > 9999 ? 9999 : value;
}


static u32 pkt16_to_rpm(u8 *ptr)
{
    u32 value = ((u32)ptr[0] << 8) | ptr[1];
    //In RPM (2 = number of poles)
    //RPM = 120000000 / number_of_poles(2, 4, ... 32) / gear_ratio(0.01 - 30.99) / Telemetry.rpm[0];
    //by default number_of_poles = 2, gear_ratio = 1.00
    if (value == 0xffff || value < 200)
        value = 0;
    else
        value = 120000000 / 2 / value;
    return value;
}

static s32 pkt16_to_temp(u8 *ptr)
{
    s32 value = ((s32)((s16)(ptr[0] << 8) | ptr[1]) - 32) * 5 / 9; //In degrees-C (16Bit signed integer)
    if (value > 500 || value < -100)
        value = 0;
    return value;
}

static u32 pkt32_to_coord(u8 *ptr)
{
    u8 tmp[4];
    for(int i = 0; i < 4; i++)
        tmp[i] = bcd_to_u8(ptr[i]);
    return tmp[3] * 3600000
         + tmp[2] * 60000
         + tmp[1] * 600
         + tmp[0] * 6; // (decimal, format DD MM.SSSS)
}

static void parse_telemetry_packet()
{
    static s32 altitude;
    static const u8 update7f[] = {
                 TELEM_DSM_FLOG_FADESA, TELEM_DSM_FLOG_FADESB,
                 TELEM_DSM_FLOG_FADESL, TELEM_DSM_FLOG_FADESR,
                 TELEM_DSM_FLOG_FRAMELOSS, TELEM_DSM_FLOG_HOLDS,
                 TELEM_DSM_FLOG_VOLT2, 0};
    static const u8 update7e[] = {
                TELEM_DSM_FLOG_RPM1, TELEM_DSM_FLOG_VOLT1, TELEM_DSM_FLOG_TEMP1, 0};
    static const u8 update16[] = { TELEM_GPS_ALT, TELEM_GPS_LAT, TELEM_GPS_LONG, 0};
    static const u8 update17[] = { TELEM_GPS_SPEED, TELEM_GPS_TIME, 0};
    const u8 *update = NULL;
    switch(packet[0]) {
        case 0x7f: //TM1000 Flight log
        case 0xff: //TM1100 Flight log
            update = update7f;
            Telemetry.p.dsm.flog.fades[0] = pkt16_to_flog(packet+2); //FadesA 0xFFFF = (not connected)
            Telemetry.p.dsm.flog.fades[1] = pkt16_to_flog(packet+4); //FadesB 0xFFFF = (not connected)
            Telemetry.p.dsm.flog.fades[2] = pkt16_to_flog(packet+6); //FadesL 0xFFFF = (not connected)
            Telemetry.p.dsm.flog.fades[3] = pkt16_to_flog(packet+8); //FadesR 0xFFFF = (not connected)
            Telemetry.p.dsm.flog.frameloss = pkt16_to_flog(packet+10);
            Telemetry.p.dsm.flog.holds = pkt16_to_flog(packet+12);
            Telemetry.p.dsm.flog.volt[1] = pkt16_to_volt(packet+14);
            break;
        case 0x7e: //TM1000
        case 0xfe: //TM1100
            update = update7e;
            Telemetry.p.dsm.flog.rpm = pkt16_to_rpm(packet+2);
            Telemetry.p.dsm.flog.volt[0] = pkt16_to_volt(packet+4);  //In 1/10 of Volts
            Telemetry.p.dsm.flog.temp = pkt16_to_temp(packet+6);
            break;
#if HAS_DSM_EXTENDED_TELEMETRY
        case 0x03: //High Current sensor
            //Telemetry.current = (s32)((s16)(packet[2] << 8) | packet[3]) * 196791 / 100000; //In 1/10 of Amps (16bit signed integer, 1 unit is 0.196791A)
            break;
        case 0x0a: //Powerbox sensor
            //Telemetry.pwb.volt1 = (((s32)packet[2] << 8) | packet[3] + 5) /10; //In 1/10 of Volts
            //Telemetry.pwb.volt1 = (((s32)packet[4] << 8) | packet[5] + 5) /10; //In 1/10 of Volts
            //Telemetry.pwb.capacity1 = ((s32)packet[6] << 8) | packet[7]; //In mAh
            //Telemetry.pwb.capacity2 = ((s32)packet[8] << 8) | packet[9]; //In mAh
            //Telemetry.pwb.alarm_v1 = packet[15] & 0x01; //0 = disable, 1 = enable
            //Telemetry.pwb.alarm_v2 = (packet[15] >> 1) & 0x01; //0 = disable, 1 = enable
            //Telemetry.pwb.alarm_c1 = (packet[15] >> 2) & 0x01; //0 = disable, 1 = enable
            //Telemetry.pwb.alarm_c2 = (packet[15] >> 3) & 0x01; //0 = disable, 1 = enable
            break;
        case 0x11: //AirSpeed sensor
            //Telemetry.airspeed = ((s32)packet[2] << 8) | packet[3]; //In km/h (16Bit value, 1 unit is 1 km/h)
            break;
        case 0x12: //Altimeter sensor
            //Telemetry.altitude = (s16)(packet[2] << 8) | packet[3]; //In 0.1 meters (16Bit signed integer, 1 unit is 0.1m)
            break;
        case 0x14: //G-Force sensor
            //Telemetry.gforce.x = (s16)(packet[2] << 8) | packet[3]; //In 0.01g (16Bit signed integers, unit is 0.01g)
            //Telemetry.gforce.y = (s16)(packet[4] << 8) | packet[5];
            //Telemetry.gforce.z = (s16)(packet[6] << 8) | packet[7];
            //Telemetry.gforce.xmax = (s16)(packet[8] << 8) | packet[9];
            //Telemetry.gforce.ymax = (s16)(packet[10] << 8) | packet[11];
            //Telemetry.gforce.zmax = (s16)(packet[12] << 8) | packet[13];
            //Telemetry.gforce.zmin = (s16)(packet[14] << 8) | packet[15];
            break;
        case 0x15: //JetCat sensor
            //Telemetry.jc.status = packet[2];
                //Possible messages for status:
                //0x00:OFF
                //0x01:WAIT FOR RPM
                //0x02:IGNITE
                //0x03;ACCELERATE
                //0x04:STABILIZE
                //0x05:LEARN HIGH
                //0x06:LEARN LOW
                //0x07:undef
                //0x08:SLOW DOWN
                //0x09:MANUAL
                //0x0a,0x10:AUTO OFF
                //0x0b,0x11:RUN
                //0x0c,0x12:ACCELERATION DELAY
                //0x0d,0x13:SPEED REG
                //0x0e,0x14:TWO SHAFT REGULATE
                //0x0f,0x15:PRE HEAT
                //0x16:PRE HEAT 2
                //0x17:MAIN F START
                //0x18:not used
                //0x19:KERO FULL ON
                //0x1a:MAX STATE
            //Telemetry.jc.throttle = (packet[3] >> 4) * 10 + (packet[3] & 0x0f); //up to 159% (the upper nibble is 0-f, the lower nibble 0-9)
            //Telemetry.jc.pack_volt = (((packet[5] >> 4) * 10 + (packet[5] & 0x0f)) * 100 
            //                         + (packet[4] >> 4) * 10 + (packet[4] & 0x0f) + 5) / 10; //In 1/10 of Volts
            //Telemetry.jc.pump_volt = (((packet[7] >> 6) * 10 + (packet[7] & 0x0f)) * 100 
            //                         + (packet[6] >> 4) * 10 + (packet[6] & 0x0f) + 5) / 10; //In 1/10 of Volts (low voltage)
            //Telemetry.jc.rpm = ((packet[10] >> 4) * 10 + (packet[10] & 0x0f)) * 10000 
            //                 + ((packet[9] >> 4) * 10 + (packet[9] & 0x0f)) * 100 
            //                 + ((packet[8] >> 4) * 10 + (packet[8] & 0x0f)); //RPM up to 999999
            //Telemetry.jc.tempEGT = (packet[13] & 0x0f) * 100 + (packet[12] >> 4) * 10 + (packet[12] & 0x0f); //EGT temp up to 999°C
            //Telemetry.jc.off_condition = packet[14];
                //Messages for Off_Condition:
                //0x00:NA
                //0x01:OFF BY RC
                //0x02:OVER TEMPERATURE
                //0x03:IGNITION TIMEOUT
                //0x04:ACCELERATION TIMEOUT
                //0x05:ACCELERATION TOO SLOW
                //0x06:OVER RPM
                //0x07:LOW RPM OFF
                //0x08:LOW BATTERY
                //0x09:AUTO OFF
                //0x0a,0x10:LOW TEMP OFF
                //0x0b,0x11:HIGH TEMP OFF
                //0x0c,0x12:GLOW PLUG DEFECTIVE
                //0x0d,0x13:WATCH DOG TIMER
                //0x0e,0x14:FAIL SAFE OFF
                //0x0f,0x15:MANUAL OFF
                //0x16:POWER BATT FAIL
                //0x17:TEMP SENSOR FAIL
                //0x18:FUEL FAIL
                //0x19:PROP FAIL
                //0x1a:2nd ENGINE FAIL
                //0x1b:2nd ENGINE DIFFERENTIAL TOO HIGH
                //0x1c:2nd ENGINE NO COMMUNICATION
                //0x1d:MAX OFF CONDITION
            break;
#endif //HAS_DSM_EXTENDED_TELEMETRY
        case 0x16: //GPS sensor (always second GPS packet)
            update = update16;
            altitude += (bcd_to_u8(packet[3]) * 100 
                       + bcd_to_u8(packet[2])) * 100; //In meters * 1000 (16Bit decimal, 1 unit is 0.1m)
            Telemetry.gps.altitude = altitude;
            Telemetry.gps.latitude = pkt32_to_coord(&packet[4]);
            if ((packet[15] & 0x01)  == 0)
                Telemetry.gps.latitude *= -1; //1=N(+), 0=S(-)
            Telemetry.gps.longitude = pkt32_to_coord(&packet[8]);
            if ((packet[15] & 0x04) == 4)
                Telemetry.gps.longitude += 360000000; //1=+100 degrees
            if ((packet[15] & 0x02) == 0)
                Telemetry.gps.longitude *= -1; //1=E(+), 0=W(-)
            //Telemetry.gps.heading = ((packet[13] >> 4) * 10 + (packet[13] & 0x0f)) * 10 
            //                      + ((packet[12] >> 4) * 10 + (packet[12] & 0x0f)) / 10; //In degrees (16Bit decimal, 1 unit is 0.1 degree)
            break;
        case 0x17: //GPS sensor (always first GPS packet)
            update = update17;
            Telemetry.gps.velocity = (bcd_to_u8(packet[3]) * 100 
                                    + bcd_to_u8(packet[2])) * 5556 / 108; //In m/s * 1000
            u8 sec   = bcd_to_u8(packet[5]);
            u8 min   = bcd_to_u8(packet[6]);
            u8 hour  = bcd_to_u8(packet[7]);
            //u8 ssec   = (packet[4] >> 4) * 10 + (packet[4] & 0x0f);
            u8 day   = 0;
            u8 month = 0;
            u8 year  = 0; // + 2000
            Telemetry.gps.time = ((year & 0x3F) << 26)
                               | ((month & 0x0F) << 22)
                               | ((day & 0x1F) << 17)
                               | ((hour & 0x1F) << 12)
                               | ((min & 0x3F) << 6)
                               | ((sec & 0x3F) << 0);
            //Telemetry.gps.sats = ((packet[8] >> 4) * 10 + (packet[8] & 0x0f));
            altitude = bcd_to_u8(packet[9]) * 1000000; //In 1000 meters * 1000 (8Bit decimal, 1 unit is 1000m)
            break;
    }
    if (update) {
        while(*update) {
            TELEMETRY_SetUpdated(*update++);
        }
    }
}

MODULE_CALLTYPE
static u16 dsm2_cb()
{
#define CH1_CH2_DELAY 4010  // Time between write of channel 1 and channel 2
#define WRITE_DELAY   1550  // Time after write to verify write complete
#define READ_DELAY     400  // Time before write to check read state, and switch channels
    if(state < DSM2_CHANSEL) {
        //Binding
        state += 1;
        if(state & 1) {
            //Send packet on even states
            //Note state has already incremented,
            // so this is actually 'even' state
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
        cyrf_configdata();
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
        while (! (CYRF_ReadRegister(0x04) & 0x02)) {
            if(++i > NUM_WAIT_LOOPS)
                break;
        }
        set_sop_data_crc();
        state++;
        return CH1_CH2_DELAY - WRITE_DELAY;
    } else if(state == DSM2_CH2_CHECK_A || state == DSM2_CH2_CHECK_B) {
        int i = 0;
        while (! (CYRF_ReadRegister(0x04) & 0x02)) {
            if(++i > NUM_WAIT_LOOPS)
                break;
        }
        if (state == DSM2_CH2_CHECK_A) {
            //Keep transmit power in sync
            CYRF_WriteRegister(CYRF_03_TX_CFG, 0x28 | Model.tx_power);
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
            CYRF_WriteRegister(0x07, 0x80); //Prepare to receive
            CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x87); //Prepare to receive
            return 11000 - CH1_CH2_DELAY - WRITE_DELAY - READ_DELAY;
        }
    } else if(state == DSM2_CH2_READ_A || state == DSM2_CH2_READ_B) {
        //Read telemetry if needed
        if(CYRF_ReadRegister(0x07) & 0x02) {
           CYRF_ReadDataPacket(packet);
           parse_telemetry_packet();
        }
        if (state == DSM2_CH2_READ_A && num_channels < 8) {
            state = DSM2_CH2_READ_B;
            CYRF_WriteRegister(0x07, 0x80); //Prepare to receive
            CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x87); //Prepare to receive
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
    cyrf_config();

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
    CYRF_SetTxRxMode(TX_EN);
    if (bind) {
        state = DSM2_BIND;
        PROTOCOL_SetBindState((BIND_COUNT > 200 ? BIND_COUNT / 2 : 200) * 10); //msecs
        initialize_bind_state();
        binding = 1;
    } else {
        state = DSM2_CHANSEL;
        binding = 0;
    }
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
