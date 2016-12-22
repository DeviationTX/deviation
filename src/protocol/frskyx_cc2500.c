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
  #define FRSKYX_Cmds PROTO_Cmds
  #pragma long_calls
#endif
#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"
#include "telemetry.h"

#ifdef MODULAR
  //Some versions of gcc applythis to definitions, others to calls
  //So just use long_calls everywhere
  //#pragma long_calls_off
  extern unsigned _data_loadaddr;
  const unsigned long protocol_type = (unsigned long)&_data_loadaddr;
#endif

#ifdef PROTO_HAS_CC2500

#include "iface_cc2500.h"


static const char * const frskyx_opts[] = {
  _tr_noop("Failsafe"), "Hold", "NoPulse", "RX", NULL,
  _tr_noop("AD2GAIN"),  "0", "2000", "655361", NULL,       // big step 10, little step 1
  _tr_noop("Freq-Fine"),  "-127", "127", NULL,
  NULL
};
enum {
    PROTO_OPTS_FAILSAFE,
    PROTO_OPTS_AD2GAIN,
    PROTO_OPTS_FREQFINE,
    LAST_PROTO_OPT,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

#define FAILSAFE_HOLD    0
#define FAILSAFE_NOPULSE 1
#define FAILSAFE_RX      2

#define PACKET_SIZE 30

// Statics are not initialized on 7e so in initialize() if necessary
static u8 chanskip;
static u8 calData[48][3];
static u8 channr;
static u8 counter_rst;
static u8 ctr;
static s8 fine;
static u8 seq_last_sent;
static u8 seq_last_rcvd;


// u8 ptr[4] = {0x01,0x12,0x23,0x30};
//u8 ptr[4] = {0x00,0x11,0x22,0x33};
static enum {
  FRSKY_BIND,
#ifndef EMULATOR
  FRSKY_BIND_DONE = 1000,
#else
  FRSKY_BIND_DONE = 50,
#endif
  FRSKY_DATA1,
  FRSKY_DATA2,
  FRSKY_DATA3,
  FRSKY_DATA4,
  FRSKY_DATA5
} state;

static u16 fixed_id;
static u8 packet[PACKET_SIZE];

static const u8 hop_data[] = {
  0x02, 0xD4, 0xBB, 0xA2, 0x89,
  0x70, 0x57, 0x3E, 0x25, 0x0C,
  0xDE, 0xC5, 0xAC, 0x93, 0x7A,
  0x61, 0x48, 0x2F, 0x16, 0xE8,
  0xCF, 0xB6, 0x9D, 0x84, 0x6B,
  0x52, 0x39, 0x20, 0x07, 0xD9,
  0xC0, 0xA7, 0x8E, 0x75, 0x5C,
  0x43, 0x2A, 0x11, 0xE3, 0xCA,
  0xB1, 0x98, 0x7F, 0x66, 0x4D,
  0x34, 0x1B, 0x00, 0x1D, 0x03 
};


static const u16 CRCTable[] = {
  0x0000,0x1189,0x2312,0x329b,0x4624,0x57ad,0x6536,0x74bf,
  0x8c48,0x9dc1,0xaf5a,0xbed3,0xca6c,0xdbe5,0xe97e,0xf8f7,
  0x1081,0x0108,0x3393,0x221a,0x56a5,0x472c,0x75b7,0x643e,
  0x9cc9,0x8d40,0xbfdb,0xae52,0xdaed,0xcb64,0xf9ff,0xe876,
  0x2102,0x308b,0x0210,0x1399,0x6726,0x76af,0x4434,0x55bd,
  0xad4a,0xbcc3,0x8e58,0x9fd1,0xeb6e,0xfae7,0xc87c,0xd9f5,
  0x3183,0x200a,0x1291,0x0318,0x77a7,0x662e,0x54b5,0x453c,
  0xbdcb,0xac42,0x9ed9,0x8f50,0xfbef,0xea66,0xd8fd,0xc974,
  0x4204,0x538d,0x6116,0x709f,0x0420,0x15a9,0x2732,0x36bb,
  0xce4c,0xdfc5,0xed5e,0xfcd7,0x8868,0x99e1,0xab7a,0xbaf3,
  0x5285,0x430c,0x7197,0x601e,0x14a1,0x0528,0x37b3,0x263a,
  0xdecd,0xcf44,0xfddf,0xec56,0x98e9,0x8960,0xbbfb,0xaa72,
  0x6306,0x728f,0x4014,0x519d,0x2522,0x34ab,0x0630,0x17b9,
  0xef4e,0xfec7,0xcc5c,0xddd5,0xa96a,0xb8e3,0x8a78,0x9bf1,
  0x7387,0x620e,0x5095,0x411c,0x35a3,0x242a,0x16b1,0x0738,
  0xffcf,0xee46,0xdcdd,0xcd54,0xb9eb,0xa862,0x9af9,0x8b70,
  0x8408,0x9581,0xa71a,0xb693,0xc22c,0xd3a5,0xe13e,0xf0b7,
  0x0840,0x19c9,0x2b52,0x3adb,0x4e64,0x5fed,0x6d76,0x7cff,
  0x9489,0x8500,0xb79b,0xa612,0xd2ad,0xc324,0xf1bf,0xe036,
  0x18c1,0x0948,0x3bd3,0x2a5a,0x5ee5,0x4f6c,0x7df7,0x6c7e,
  0xa50a,0xb483,0x8618,0x9791,0xe32e,0xf2a7,0xc03c,0xd1b5,
  0x2942,0x38cb,0x0a50,0x1bd9,0x6f66,0x7eef,0x4c74,0x5dfd,
  0xb58b,0xa402,0x9699,0x8710,0xf3af,0xe226,0xd0bd,0xc134,
  0x39c3,0x284a,0x1ad1,0x0b58,0x7fe7,0x6e6e,0x5cf5,0x4d7c,
  0xc60c,0xd785,0xe51e,0xf497,0x8028,0x91a1,0xa33a,0xb2b3,
  0x4a44,0x5bcd,0x6956,0x78df,0x0c60,0x1de9,0x2f72,0x3efb,
  0xd68d,0xc704,0xf59f,0xe416,0x90a9,0x8120,0xb3bb,0xa232,
  0x5ac5,0x4b4c,0x79d7,0x685e,0x1ce1,0x0d68,0x3ff3,0x2e7a,
  0xe70e,0xf687,0xc41c,0xd595,0xa12a,0xb0a3,0x8238,0x93b1,
  0x6b46,0x7acf,0x4854,0x59dd,0x2d62,0x3ceb,0x0e70,0x1ff9,
  0xf78f,0xe606,0xd49d,0xc514,0xb1ab,0xa022,0x92b9,0x8330,
  0x7bc7,0x6a4e,0x58d5,0x495c,0x3de3,0x2c6a,0x1ef1,0x0f78
};



static u16 crc(u8 *data, u8 len) {
  u16 crc = 0;
  for(int i=0; i < len; i++)
      crc = (crc<<8) ^ CRCTable[((u8)(crc>>8) ^ *data++) & 0xFF];
  return crc;
}

static void initialize_data(u8 adr)
{
  CC2500_WriteReg(CC2500_0C_FSCTRL0, fine);  // Frequency offset hack 
  CC2500_WriteReg(CC2500_18_MCSM0,    0x8); 
  CC2500_WriteReg(CC2500_09_ADDR, adr ? 0x03 : (fixed_id & 0xff));
  CC2500_WriteReg(CC2500_07_PKTCTRL1,0x05);
}


static void set_start(u8 ch) {
  CC2500_Strobe(CC2500_SIDLE);
  CC2500_WriteReg(CC2500_23_FSCAL3, calData[ch][0]);
  CC2500_WriteReg(CC2500_24_FSCAL2, calData[ch][1]);
  CC2500_WriteReg(CC2500_25_FSCAL1, calData[ch][2]);
  CC2500_WriteReg(CC2500_0A_CHANNR, ch == 47 ? 0 : hop_data[ch]);
}   

#define RXNUM 16
static void frskyX_build_bind_packet()
{
    packet[0] = 0x1D;       
    packet[1] = 0x03;          
    packet[2] = 0x01;               

    packet[3] = fixed_id;
    packet[4] = fixed_id >> 8;
    int idx = ((state - FRSKY_BIND) % 10) * 5;
    packet[5] = idx;  
    packet[6] = hop_data[idx++];
    packet[7] = hop_data[idx++];
    packet[8] = hop_data[idx++];
    packet[9] = hop_data[idx++];
    packet[10] = hop_data[idx++];
    packet[11] = 0x02;
    packet[12] = RXNUM;

    memset(&packet[13], 0, 15);

    u16 lcrc = crc(&packet[3], 25);
    packet[28] = lcrc >> 8;
    packet[29] = lcrc;

}


//#define STICK_SCALE    819  // full scale at +-125
#define STICK_SCALE    751  // +/-100 gives 2000/1000 us pwm
static u16 scaleForPXX(u8 chan, u8 failsafe)
{ //mapped 860,2140(125%) range to 64,1984(PXX values);
//  return (u16)(((Servo_data[i]-PPM_MIN)*3)>>1)+64;
// 0-2047, 0 = 817, 1024 = 1500, 2047 = 2182
    s32 chan_val;

    if (chan >= Model.num_channels) {
        if (chan > 7 && !failsafe)
            return scaleForPXX(chan-8, 0);
        return (chan < 8) ? 1024 : 3072;   // center values
    }

    if (failsafe)
        if (Model.limits[chan].flags & CH_FAILSAFE_EN)
            chan_val = Model.limits[chan].failsafe * CHAN_MULTIPLIER;
        else if (Model.proto_opts[PROTO_OPTS_FAILSAFE] == FAILSAFE_HOLD)
            return (chan < 8) ? 2047 : 4095;    // Hold
        else
            return (chan < 8) ? 0 : 2048;       // No Pulses
    else
        chan_val = Channels[chan];
    
    chan_val = chan_val * STICK_SCALE / CHAN_MAX_VALUE + 1024;

    if (chan_val > 2046)   chan_val = 2046;
    else if (chan_val < 1) chan_val = 1;

    if (chan > 7) chan_val += 2048;   // upper channels offset

    return chan_val;
}
 
#ifdef EMULATOR
#define FAILSAFE_TIMEOUT 64
#else
#define FAILSAFE_TIMEOUT 1032
#endif

// These are local to function but 7e modules don't initialize statics
// so make file scope and set in initialize()
    static u16 failsafe_count;
    static u8 chan_offset;
    static u8 FS_flag;
static void frskyX_data_frame() {
    //0x1D 0xB3 0xFD 0x02 0x56 0x07 0x15 0x00 0x00 0x00 0x04 0x40 0x00 0x04 0x40 0x00 0x04 0x40 0x00 0x04 0x40 0x08 0x00 0x00 0x00 0x00 0x00 0x00 0x96 0x12
    // channel packing: H (0)7-4, L (0)3-0; H (1)3-0, L (0)11-8; H (1)11-8, L (1)7-4 etc

    u16 chan_0;
    u16 chan_1; 
    static u8 failsafe_chan;
    u8 startChan = 0;


    // data frames sent every 9ms; failsafe every 9 seconds
    if (FS_flag == 0  &&  failsafe_count > FAILSAFE_TIMEOUT  &&  chan_offset == 0  &&  Model.proto_opts[PROTO_OPTS_FAILSAFE] != FAILSAFE_RX) {
        FS_flag = 0x10;
        failsafe_chan = 0;
    } else if (FS_flag & 0x10 && failsafe_chan < (Model.num_channels-1)) {
        FS_flag = 0x10 | ((FS_flag + 2) & 0x0f);
        failsafe_chan += 1;
    } else if (FS_flag & 0x10) {
        FS_flag = 0;
        failsafe_count = 0;
    }
    failsafe_count += 1;


    packet[0] = 0x1D; 
    packet[1] = fixed_id;
    packet[2] = fixed_id >> 8;

    packet[3] = 0x02;
    packet[4] = (ctr << 6) + channr;  //*64
    packet[5] = counter_rst;
    packet[6] = RXNUM;

    // packet[7] is FLAGS
    // 00 - standard packet
    // 10, 12, 14, 16, 18, 1A, 1C, 1E - failsafe packet
    packet[7] = 0;    // may be replaced by failsafe below
    packet[8] = 0;

    startChan = chan_offset;

    for(u8 i = 0; i < 12 ; i += 3) {    // 12 bytes of channel data
        if (FS_flag & 0x10 && (((failsafe_chan & 0x7) | chan_offset) == startChan)) {
            packet[7] = FS_flag;
            chan_0 = scaleForPXX(failsafe_chan, 1);
        } else {
            chan_0 = scaleForPXX(startChan, 0);
        }
        startChan++;

        if (FS_flag & 0x10 && (((failsafe_chan & 0x7) | chan_offset) == startChan)) {
            packet[7] = FS_flag;
            chan_1 = scaleForPXX(failsafe_chan, 1);
        } else {
            chan_1 = scaleForPXX(startChan, 0);
        }
        startChan++;

        packet[9+i]   = chan_0;
        packet[9+i+1] = (((chan_0 >> 8) & 0x0F) | (chan_1 << 4));
        packet[9+i+2] = chan_1 >> 4;
    }

    packet[21] = seq_last_sent << 4 | seq_last_rcvd;
    if (seq_last_sent < 8 && seq_last_rcvd < 8)
        seq_last_sent = (seq_last_sent + 1) % 4;
    else if (seq_last_rcvd == 0)
        seq_last_sent = 1;
    
    chan_offset ^= 0x08;
    
    for (u8 i = 22;i<28;i++)
      packet[i] = 0;
    
    u16 lcrc = crc(&packet[3], 25);
    packet[28] = lcrc >> 8;
    packet[29] = lcrc;
} 


// Telemetry
/* SPORT details serial
  100K 8E2 normal-multiprotocol
  -every 12ms-
  1  2  3  4  5  6  7  8  9  CRC DESCR
  7E 98 10 05 F1 20 23 0F 00 A6 SWR_ID 
  7E 98 10 01 F1 33 00 00 00 C9 RSSI_ID 
  7E 98 10 04 F1 58 00 00 00 A1 BATT_ID 
  7E BA 10 03 F1 E2 00 00 00 18 ADC2_ID 
  7E BA 10 03 F1 E2 00 00 00 18 ADC2_ID 
  7E BA 10 03 F1 E2 00 00 00 18 ADC2_ID 
  7E BA 10 03 F1 E2 00 00 00 18 ADC2_ID 
  7E BA 10 03 F1 E2 00 00 00 18 ADC2_ID 
  7E BA 10 03 F1 E2 00 00 00 18 ADC2_ID   
  
  
  Telemetry frames(RF) SPORT info 15 bytes
  SPORT frame 6+3 bytes
  [00] PKLEN  0E 0E 0E 0E 
  [01] TXID1  DD DD DD DD 
  [02] TXID2  6D 6D 6D 6D 
  [03] CONST  02 02 02 02 
  [04] RS/RB  2C D0 2C CE // D0,CE = 2*RSSI; ....2C = RX battery voltage(5V from Bec)
  [05] ?????  03 10 21 32 // TX/RX telemetry hand-shake bytes
  [06] NO.BT  00 00 06 03 // No.of valid SPORT frame bytes in the frame    
  [07] STRM1  00 00 7E 00 
  [08] STRM2  00 00 1A 00 
  [09] STRM3  00 00 10 00 
  [10] STRM4  03 03 03 03  
  [11] STRM5  F1 F1 F1 F1 
  [12] STRM6  D1 D1 D0 D0
  [13] CHKSUM1
  [14] CHKSUM2
  [15] RSSI
  [16] LQI
*/

#define START_STOP              0x7e
#define BYTESTUFF               0x7d
#define STUFF_MASK              0x20
#define FRSKY_SPORT_PACKET_SIZE    8
#define TELEM_PKT_SIZE            17

// FrSky PRIM IDs (1 byte)
#define DATA_FRAME                0x10

#define SPORT_DATA_U8(packet)   (packet[4])
#define SPORT_DATA_S32(packet)  (*((int32_t *)(packet+4)))
#define SPORT_DATA_U32(packet)  (*((uint32_t *)(packet+4)))
#define HUB_DATA_U16(packet)    (*((uint16_t *)(packet+4)))


#if HAS_EXTENDED_TELEMETRY

#include "frsky_d_telem._c"


static void processSportPacket(u8 *packet) {
//    u8  instance = (packet[0] & 0x1F) + 1;    // all instances of same sensor write to same telemetry value
    u8  prim                = packet[1];
    u16 id                  = *((u16 *)(packet+2));

    if (prim != DATA_FRAME)
        return;

    if ((id >> 8) == 0) {
        // The old FrSky IDs
        processHubPacket(id, HUB_DATA_U16(packet));
        return;
    }

    // rx telemetry in stream data
    // 8 bit voltages are raw ADC with 3.3V reference => 13.2mV/bit
    // Convert to two decimal places (hundredths of volts)
    switch(id) {
    case ADC1_ID:    // put in VOLT3 so ADC2_ID (external input) stays consistent with frsky2way
        set_telemetry(TELEM_FRSKY_VOLT3, SPORT_DATA_U8(packet) * 132 / 100);
        break;
    case ADC2_ID:    // put in VOLT2 for consistency with frsky2way
        set_telemetry(TELEM_FRSKY_VOLT2, ((s32)SPORT_DATA_U8(packet) * 132 * Model.proto_opts[PROTO_OPTS_AD2GAIN]) / 10000);
        break;
    case BATT_ID:
        set_telemetry(TELEM_FRSKY_VOLTA, SPORT_DATA_U8(packet) * 132 / 100);
        break;
    }
    
    s32 data = SPORT_DATA_S32(packet);
    switch (id & 0xfff0) {
    case ALT_FIRST_ID & 0xfff0:
        if (ground_level == 0) ground_level = data;
        set_telemetry(TELEM_FRSKY_ALTITUDE, data - ground_level);
        break;
    case VARIO_FIRST_ID & 0xfff0:
        set_telemetry(TELEM_FRSKY_VARIO, data);
        break;
    case CURR_FIRST_ID & 0xfff0:
        set_telemetry(TELEM_FRSKY_CURRENT, data);
        if (discharge_time == 0) discharge_time = CLOCK_getms();
        discharge_dAms += data * (CLOCK_getms() - discharge_time);
        discharge_time = CLOCK_getms();
        set_telemetry(TELEM_FRSKY_DISCHARGE, discharge_dAms / 36000);
        break;;
    case VFAS_FIRST_ID & 0xfff0:
        set_telemetry(TELEM_FRSKY_VOLTA, data);
        break;

    case CELLS_FIRST_ID & 0xfff0:{
        u8 cells_count = (data & 0xF0) >> 4;
        u8 cell_index  = (data & 0x0F);
        update_cell(cell_index, ((data & 0x000FFF00) >> 8) / 5);
        if (cell_index+1 < cells_count)
            update_cell(cell_index+1, ((data & 0xFFF00000) >> 20) / 5);
        update_min_cell(cells_count);
        break;}

    case T1_FIRST_ID & 0xfff0:
        set_telemetry(TELEM_FRSKY_TEMP1, data);
        break;
    case T2_FIRST_ID & 0xfff0:
        set_telemetry(TELEM_FRSKY_TEMP2, data);
        break;
    case RPM_FIRST_ID & 0xfff0:
        set_telemetry(TELEM_FRSKY_RPM, data);
        break;
    case FUEL_FIRST_ID & 0xfff0:
        set_telemetry(TELEM_FRSKY_FUEL, data);
        break;

    case GPS_LONG_LATI_FIRST_ID & 0xfff0:{
        u32 gps_long_lati_b1w = (data & 0x3fffffff) / 10000;
        u32 gps_long_lati_a1w = (data & 0x3fffffff) % 10000;
        s32 bp = (gps_long_lati_b1w / 60 * 100) + (gps_long_lati_b1w % 60);
        s32 ap = gps_long_lati_a1w;
        s32 deg = bp / 100;
        s32 min = bp % 100;
        if (data & (1 << 31)) {
            Telemetry.gps.longitude = (deg * 60 + min) * 60 * 1000 + ap * 6;
            if (data & (1 << 30))
                Telemetry.gps.longitude = -Telemetry.gps.longitude;   // west negative
            TELEMETRY_SetUpdated(TELEM_GPS_LONG);
        } else {
            Telemetry.gps.latitude = (deg * 60 + min) * 60 * 1000 + ap * 6;
            if (data & (1 << 30))
                Telemetry.gps.latitude = -Telemetry.gps.latitude;   // south negative
            TELEMETRY_SetUpdated(TELEM_GPS_LAT);
        }
        break;}

    case GPS_ALT_FIRST_ID & 0xfff0:
        Telemetry.gps.altitude = data * 10;
        TELEMETRY_SetUpdated(TELEM_GPS_ALT);
        break;
    case GPS_SPEED_FIRST_ID & 0xfff0:
        Telemetry.gps.velocity = data * 5556 / 10800;
        TELEMETRY_SetUpdated(TELEM_GPS_SPEED);
        break;
    case GPS_COURS_FIRST_ID & 0xfff0:
        // = TELEM_GPS_HEADING;
        break;
    case GPS_TIME_DATE_FIRST_ID & 0xfff0:
        if (data & 0x000000ff) {
            fr_gps.year =      (u16) ((data & 0xff000000) >> 24);
            fr_gps.day_month = (u16) ((data & 0x00ffff00) >> 8);
        } else {
            fr_gps.hour_min = (u16) ((data & 0xffff0000) >> 16);
            fr_gps.second =   (u16) ((data & 0x0000ff00) >> 8);
            Telemetry.gps.time = ( (u32)fr_gps.year & 0x3f)            << 26
                               | (((u32)fr_gps.day_month >> 8) & 0x0f) << 22
                               | ( (u32)fr_gps.day_month & 0x1f)       << 17
                               | (((u32)fr_gps.hour_min >> 8) & 0x1f)  << 12
                               | ( (u32)fr_gps.hour_min & 0x3f)        << 6
                               | ( (u32)fr_gps.second & 0x3f);
            TELEMETRY_SetUpdated(TELEM_GPS_TIME);
        }
        break;
    } // switch
// not sure about these
//    case A3_FIRST_ID & 0xfff0:
//    case A4_FIRST_ID & 0xfff0:
//    case AIR_SPEED_FIRST_ID & 0xfff0:
//    case ACCX_FIRST_ID & 0xfff0:
//    case ACCY_FIRST_ID & 0xfff0:
//    case ACCZ_FIRST_ID & 0xfff0:
}


typedef enum {
    STATE_DATA_IDLE,
    STATE_DATA_START,
    STATE_DATA_IN_FRAME,
    STATE_DATA_XOR,
} SportStates;
static SportStates dataState = STATE_DATA_IDLE;   // file scope so can be reset on loss of rx packet sync

static void frsky_parse_sport_stream(u8 data) {
    static u8 sportRxBufferCount;
    static u8 sportRxBuffer[FRSKY_SPORT_PACKET_SIZE];   // Receive buffer. 8 bytes (full packet)

    switch (dataState) {
    case STATE_DATA_START:
        if (data == START_STOP) {
            dataState = STATE_DATA_IN_FRAME ;
            sportRxBufferCount = 0;
        } else {
            if (sportRxBufferCount < FRSKY_SPORT_PACKET_SIZE)
                sportRxBuffer[sportRxBufferCount++] = data;
            dataState = STATE_DATA_IN_FRAME;
        }
        break;

    case STATE_DATA_IN_FRAME:
        if (data == BYTESTUFF) {
            dataState = STATE_DATA_XOR; // XOR next byte
        }
        else if (data == START_STOP) {
            dataState = STATE_DATA_IN_FRAME ;
            sportRxBufferCount = 0;
        }
        else if (sportRxBufferCount < FRSKY_SPORT_PACKET_SIZE) {
            sportRxBuffer[sportRxBufferCount++] = data;
        }
        break;

    case STATE_DATA_XOR:
        if (sportRxBufferCount < FRSKY_SPORT_PACKET_SIZE) {
          sportRxBuffer[sportRxBufferCount++] = data ^ STUFF_MASK;
        }
        dataState = STATE_DATA_IN_FRAME;
        break;

    case STATE_DATA_IDLE:
        if (data == START_STOP) {
          sportRxBufferCount = 0;
          dataState = STATE_DATA_START;
        }
        break;
    } // switch

    if (sportRxBufferCount >= FRSKY_SPORT_PACKET_SIZE) {
        processSportPacket(sportRxBuffer);
        dataState = STATE_DATA_IDLE;
    }
}

#endif // HAS_EXTENDED_TELEMETRY

static void frsky_check_telemetry(u8 *pkt, u8 len) {
    // only process packets with the required id and packet length and good crc
    if (len == TELEM_PKT_SIZE
        && pkt[0] == TELEM_PKT_SIZE - 3
        && pkt[1] == (fixed_id & 0xff)
        && pkt[2] == (fixed_id >> 8)
        && crc(&pkt[3], TELEM_PKT_SIZE-7) == (pkt[TELEM_PKT_SIZE-4] << 8 | pkt[TELEM_PKT_SIZE-3])
       ) {
        if (pkt[4] > 0x36) {   // distinguish RSSI from VOLT1 (maybe bit 7 always set for RSSI?)
            Telemetry.value[TELEM_FRSKY_RSSI] = pkt[4] / 2; 	// Value in Db
            TELEMETRY_SetUpdated(TELEM_FRSKY_RSSI);
        } else {
            Telemetry.value[TELEM_FRSKY_VOLT1] = pkt[4] * 10;      // In 1/100 of Volts
            TELEMETRY_SetUpdated(TELEM_FRSKY_VOLT1);
        }

        if ((pkt[5] >> 4 & 0x0f) == 0x08) {   // restart or somesuch
            seq_last_sent = 8;
            seq_last_rcvd = 0;
#if HAS_EXTENDED_TELEMETRY
            dataState = STATE_DATA_IDLE;    // reset sport decoder
#endif // HAS_EXTENDED_TELEMETRY
        } else {
            if ((pkt[5] >> 4 & 0x03) == (seq_last_rcvd + 1) % 4) {
                seq_last_rcvd = (seq_last_rcvd + 1) % 4;
            }
#if HAS_EXTENDED_TELEMETRY
            else 
                dataState = STATE_DATA_IDLE;    // reset sport decoder if sequence number wrong
#endif // HAS_EXTENDED_TELEMETRY
        }

#if HAS_EXTENDED_TELEMETRY
        if (pkt[6] <= 6)
            for (u8 i=0; i < pkt[6]; i++)
                frsky_parse_sport_stream(pkt[7+i]);
#endif // HAS_EXTENDED_TELEMETRY
    }
}



#ifdef EMULATOR
// for emulation to match fixed_id set tx fixed id to 123456 and add
// txid=F6C4FBB3 to top of hardware.ini
static u8 telem_idx;
static const u8 telem_test[][17] = {
{0x0e, 0x20, 0xe6, 0x02, 0x2d, 0x88, 0x00, 0x00, 0x00, 0x00, 0x10, 0x01, 0xd3, 0xba, 0x56, 0xc4, 0x28},
{0x0e, 0x20, 0xe6, 0x02, 0xa7, 0x10, 0x06, 0x7e, 0x1a, 0x10, 0x10, 0x01, 0x2d, 0x7f, 0x18, 0xea, 0x48},
{0x0e, 0x20, 0xe6, 0x02, 0x2d, 0x21, 0x06, 0xff, 0xff, 0xff, 0x7e, 0x1a, 0x10, 0x91, 0xbb, 0xe4, 0xb3},
{0x0e, 0x20, 0xe6, 0x02, 0xbf, 0x32, 0x06, 0x03, 0xf1, 0xd3, 0x00, 0x00, 0x00, 0x8e, 0xad, 0xca, 0xd3},
{0x0e, 0x20, 0xe6, 0x02, 0x2d, 0x03, 0x06, 0x7e, 0x1a, 0x10, 0x10, 0x01, 0xd3, 0x25, 0xc3, 0xe5, 0x2a},
{0x0e, 0x20, 0xe6, 0x02, 0xcb, 0x10, 0x06, 0x00, 0x00, 0x00, 0x7e, 0x1a, 0x10, 0x6a, 0x1a, 0xb3, 0xb5},
{0x0e, 0x20, 0xe6, 0x02, 0x2d, 0x21, 0x06, 0x10, 0x01, 0xd3, 0x00, 0x00, 0x00, 0xc9, 0x10, 0x58, 0x0e},
{0x0e, 0x20, 0xe6, 0x02, 0xd0, 0x32, 0x06, 0x7e, 0x1a, 0x10, 0x03, 0x01, 0xd3, 0xd9, 0x44, 0x7a, 0xd1},
{0x0e, 0x20, 0xe6, 0x02, 0x2d, 0x03, 0x06, 0x00, 0x00, 0x00, 0x7e, 0x1a, 0x10, 0xb1, 0x20, 0xe1, 0xff},
{0x0e, 0x20, 0xe6, 0x02, 0xd5, 0x10, 0x06, 0x10, 0x01, 0xd3, 0x00, 0x00, 0x00, 0x8c, 0xc9, 0xc0, 0x26},
{0x0e, 0x20, 0xe6, 0x02, 0x2d, 0x21, 0x06, 0x7e, 0x1a, 0x10, 0x28, 0x00, 0x05, 0x05, 0x58, 0x90, 0x3c},
{0x0e, 0x20, 0xe6, 0x02, 0xd7, 0x32, 0x06, 0x00, 0x00, 0x00, 0x7e, 0x1a, 0x10, 0xc6, 0x72, 0x05, 0x3c},
{0x0e, 0x20, 0xe6, 0x02, 0x2d, 0x03, 0x06, 0x10, 0x01, 0xd2, 0x00, 0x00, 0x00, 0xa9, 0x8b, 0x08, 0xcf},
{0x0e, 0x20, 0xe6, 0x02, 0xda, 0x10, 0x06, 0x7e, 0x1a, 0x10, 0x10, 0x01, 0xd3, 0x02, 0x6a, 0x58, 0x0e},
{0x0e, 0x20, 0xe6, 0x02, 0x2d, 0x21, 0x06, 0x00, 0x00, 0x00, 0x7e, 0x1a, 0x10, 0x91, 0xbb, 0x7a, 0xd1},
{0x0e, 0x20, 0xe6, 0x02, 0xdb, 0x32, 0x06, 0x10, 0x01, 0xd3, 0x00, 0x00, 0x00, 0xc6, 0x72, 0xe1, 0xff},
{0x0e, 0x20, 0xe6, 0x02, 0x2d, 0x03, 0x06, 0x7e, 0x1a, 0x10, 0x10, 0x01, 0xd3, 0x25, 0xc3, 0xc0, 0x26},
{0x0e, 0x20, 0xe6, 0x02, 0xda, 0x10, 0x06, 0x00, 0x00, 0x00, 0x7e, 0x1a, 0x10, 0x96, 0x89, 0x90, 0x3c},
{0x0e, 0x20, 0xe6, 0x02, 0x2d, 0x21, 0x06, 0x10, 0x01, 0xd3, 0x00, 0x00, 0x00, 0xc9, 0x10, 0x05, 0x3c},
};
#endif

  
static u16 frskyx_cb() {
  u8 len;

  switch(state) { 
    default: 
      set_start(47);    
      CC2500_SetPower(Model.tx_power);
      CC2500_Strobe(CC2500_SFRX);
      frskyX_build_bind_packet();
      CC2500_Strobe(CC2500_SIDLE);
      CC2500_WriteData(packet, packet[0]+1);
      state++;
#ifndef EMULATOR
      return 9000;
#else
      return 90;
#endif
    case FRSKY_BIND_DONE:
      PROTOCOL_SetBindState(0);
      initialize_data(0);
      channr = 0;
      state++;      
      break;    
    case FRSKY_DATA1:
      CC2500_SetTxRxMode(TX_EN);
      set_start(channr);
      CC2500_SetPower(Model.tx_power);    
      CC2500_Strobe(CC2500_SFRX);
      frskyX_data_frame();
      CC2500_Strobe(CC2500_SIDLE);    
      CC2500_WriteData(packet, packet[0]+1);
      channr = (channr + chanskip) % 47;
      state++;
#ifndef EMULATOR
      return 5500;
#else
      return 55;
#endif
    case FRSKY_DATA2:
      CC2500_SetTxRxMode(RX_EN);
      CC2500_Strobe(CC2500_SIDLE);
      state++;
#ifndef EMULATOR
      return 200;
#else
      return 2;
#endif
    case FRSKY_DATA3:   
      CC2500_Strobe(CC2500_SRX);
      state++;
#ifndef EMULATOR
      return 3000;
#else
      return 30;
#endif
    case FRSKY_DATA4:
      len = CC2500_ReadReg(CC2500_3B_RXBYTES | CC2500_READ_BURST) & 0x7F; 
#ifndef EMULATOR
      if (len && len < PACKET_SIZE) {
          CC2500_ReadData(packet, len);
          frsky_check_telemetry(packet, len);
      } else {
          // restart sequence on missed packet - might need count or timeout instead of one missed
          seq_last_sent = 0;
          seq_last_rcvd = 8;
      }
#else
      memcpy(packet, &telem_test[telem_idx], sizeof(telem_test[0]));
      telem_idx = (telem_idx + 1) % (sizeof(telem_test)/sizeof(telem_test[0]));
      packet[1] = fixed_id & 0xff;
      packet[2] = fixed_id >> 8;
      frsky_check_telemetry(packet, sizeof(telem_test[0]));
#endif
      state = FRSKY_DATA1;
#ifndef EMULATOR
      return 300;
#else
      return 3;
#endif
  }   
  return 1;   
}

static void frskyX_init() {
  CC2500_Reset();
  CC2500_WriteReg(CC2500_02_IOCFG0, 0x06);    
  CC2500_WriteReg(CC2500_00_IOCFG2, 0x06);
  CC2500_WriteReg(CC2500_17_MCSM1, 0x0C);
  CC2500_WriteReg(CC2500_18_MCSM0, 0x18);
  CC2500_WriteReg(CC2500_06_PKTLEN, 0x1E);
  CC2500_WriteReg(CC2500_07_PKTCTRL1, 0x04);
  CC2500_WriteReg(CC2500_08_PKTCTRL0, 0x01);
  CC2500_WriteReg(CC2500_3E_PATABLE, 0xff);
  CC2500_WriteReg(CC2500_0B_FSCTRL1, 0x0A);
  CC2500_WriteReg(CC2500_0C_FSCTRL0, 0x00);
  CC2500_WriteReg(CC2500_0D_FREQ2, 0x5c);
  CC2500_WriteReg(CC2500_0E_FREQ1, 0x76);
//  CC2500_WriteReg(CC2500_0F_FREQ0, 0x27); 
  CC2500_WriteReg(CC2500_0F_FREQ0, 0x27);
  CC2500_WriteReg(CC2500_10_MDMCFG4, 0x7B); 
  CC2500_WriteReg(CC2500_11_MDMCFG3, 0x61);
  CC2500_WriteReg(CC2500_12_MDMCFG2, 0x13);
  CC2500_WriteReg(CC2500_13_MDMCFG1, 0x23);
  CC2500_WriteReg(CC2500_14_MDMCFG0, 0x7a);
  CC2500_WriteReg(CC2500_15_DEVIATN, 0x51);
  CC2500_WriteReg(CC2500_19_FOCCFG, 0x16);
  CC2500_WriteReg(CC2500_1A_BSCFG, 0x6c); 
  CC2500_WriteReg(CC2500_1B_AGCCTRL2,0x43);
  CC2500_WriteReg(CC2500_1C_AGCCTRL1,0x40);
  CC2500_WriteReg(CC2500_1D_AGCCTRL0,0x91);
  CC2500_WriteReg(CC2500_21_FREND1, 0x56);
  CC2500_WriteReg(CC2500_22_FREND0, 0x10);
  CC2500_WriteReg(CC2500_23_FSCAL3, 0xa9);
  CC2500_WriteReg(CC2500_24_FSCAL2, 0x0A);
  CC2500_WriteReg(CC2500_25_FSCAL1, 0x00);
  CC2500_WriteReg(CC2500_26_FSCAL0, 0x11);
  CC2500_WriteReg(CC2500_29_FSTEST, 0x59);
  CC2500_WriteReg(CC2500_2C_TEST2, 0x88);
  CC2500_WriteReg(CC2500_2D_TEST1, 0x31);
  CC2500_WriteReg(CC2500_2E_TEST0, 0x0B);
  CC2500_WriteReg(CC2500_03_FIFOTHR, 0x07);
  CC2500_WriteReg(CC2500_09_ADDR, 0x00);
  CC2500_WriteReg(CC2500_07_PKTCTRL1, 0x04);      
  CC2500_WriteReg(CC2500_0C_FSCTRL0, fine);
  CC2500_Strobe(CC2500_SIDLE);    
  //calibrate hop channels
  for (u8 c = 0; c < 47; c++) {
      CC2500_Strobe(CC2500_SIDLE);    
      CC2500_WriteReg(CC2500_0A_CHANNR, hop_data[c]);
      CC2500_Strobe(CC2500_SCAL);
      usleep(900);
      calData[c][0] = CC2500_ReadReg(CC2500_23_FSCAL3);
      calData[c][1] = CC2500_ReadReg(CC2500_24_FSCAL2); 
      calData[c][2] = CC2500_ReadReg(CC2500_25_FSCAL1);
  }
  CC2500_Strobe(CC2500_SIDLE);      
  CC2500_WriteReg(CC2500_0A_CHANNR, 0x00);
  CC2500_Strobe(CC2500_SCAL);
  usleep(900);
  calData[47][0] = CC2500_ReadReg(CC2500_23_FSCAL3);
  calData[47][1] = CC2500_ReadReg(CC2500_24_FSCAL2);  
  calData[47][2] = CC2500_ReadReg(CC2500_25_FSCAL1);
}



// Generate internal id from TX id and manufacturer id (STM32 unique id)
static int get_tx_id()
{
    u32 lfsr = 0x7649eca9ul;

    u8 var[12];
    MCU_SerialNumber(var, 12);
    for (int i = 0; i < 12; ++i) {
        rand32_r(&lfsr, var[i]);
    }
    for (u8 i = 0, j = 0; i < sizeof(Model.fixed_id); ++i, j += 8)
        rand32_r(&lfsr, (Model.fixed_id >> j) & 0xff);
    return rand32_r(&lfsr, 0);
}

static void initialize(int bind)
{
    CLOCK_StopTimer();

    // initialize statics since 7e modules don't initialize
    fine = Model.proto_opts[PROTO_OPTS_FREQFINE];
    fixed_id = (u16) get_tx_id();
    failsafe_count = 0;
    chan_offset = 0;
    FS_flag = 0;
    channr = 0;
    chanskip = 0;
    ctr = 0;
    seq_last_sent = 0;
    seq_last_rcvd = 8;
#if HAS_EXTENDED_TELEMETRY
    Telemetry.value[TELEM_FRSKY_MIN_CELL] = TELEMETRY_GetMaxValue(TELEM_FRSKY_MIN_CELL);
#endif

    u32 seed = get_tx_id();
    while (!chanskip)
        chanskip = (rand32_r(&seed, 0) & 0xfefefefe) % 47;
    while((chanskip - ctr) % 4)
        ctr = (ctr+1) % 4;
    counter_rst = (chanskip - ctr) >> 2;

    frskyX_init(); 
    CC2500_SetTxRxMode(TX_EN);  // enable PA 

    if (bind) {
        PROTOCOL_SetBindState(0xFFFFFFFF);
        state = FRSKY_BIND;
        initialize_data(1);
    } else {
        state = FRSKY_DATA1;
        initialize_data(0);
    }

#ifndef EMULATOR
    CLOCK_StartTimer(10000, frskyx_cb);
#else
    CLOCK_StartTimer(100, frskyx_cb);
#endif
}

const void *FRSKYX_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(0); return 0;
        case PROTOCMD_CHECK_AUTOBIND: return 0; //Never Autobind
        case PROTOCMD_BIND:  initialize(1); return 0;
        case PROTOCMD_NUMCHAN: return (void *)12L;
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)8L;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
        case PROTOCMD_GETOPTIONS:
            if (!Model.proto_opts[PROTO_OPTS_AD2GAIN]) Model.proto_opts[PROTO_OPTS_AD2GAIN] = 100;  // if not set, default to no gain
            return frskyx_opts;
        case PROTOCMD_TELEMETRYSTATE:
            return (void *)(long) PROTO_TELEM_ON;
        case PROTOCMD_TELEMETRYTYPE: 
            return (void *)(long) TELEM_FRSKY;
        case PROTOCMD_RESET:
        case PROTOCMD_DEINIT:
            CLOCK_StopTimer();
            return (void *)(CC2500_Reset() ? 1L : -1L);
        default: break;
    }
    return 0;
}
#endif
