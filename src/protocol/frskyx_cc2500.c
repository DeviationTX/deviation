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
#include "config/model.h"
#include "config/tx.h"
#include "telemetry.h"

#ifdef PROTO_HAS_CC2500

static const char * const frskyx_opts[] = {
  _tr_noop("Failsafe"), "Hold", "NoPulse", "RX", NULL,
  _tr_noop("AD2GAIN"),  "0", "2000", "655361", NULL,       // big step 10, little step 1
  _tr_noop("Freq-Fine"),  "-127", "127", NULL,
  _tr_noop("Format"),  "FCC", "EU", NULL,
  _tr_noop("RSSIChan"),  "None", "LastChan", NULL,
  _tr_noop("S.PORT Out"), _tr_noop("Disabled"), _tr_noop("Enabled"), NULL,
  _tr_noop("Bind Mode"), _tr_noop("1-8, On"), _tr_noop("1-8, Off"), _tr_noop("9-16, On"), _tr_noop("9-16, Off"), NULL,
  _tr_noop("Version"), _tr_noop("V1"), _tr_noop("V2"), NULL,
  NULL
};
enum {
    PROTO_OPTS_FAILSAFE,
    PROTO_OPTS_AD2GAIN,
    PROTO_OPTS_FREQFINE,
    PROTO_OPTS_FORMAT,
    PROTO_OPTS_RSSICHAN,
    PROTO_OPTS_SPORTOUT,
    PROTO_OPTS_BINDMODE,
    PROTO_OPTS_VERSION,
    LAST_PROTO_OPT,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

#define FAILSAFE_HOLD    0
#define FAILSAFE_NOPULSE 1
#define FAILSAFE_RX      2

#define MAX_PACKET_SIZE  33

// Statics are not initialized on 7e so in initialize() if necessary
static u8 chanskip;
static u8 calData[48][3];
static u8 channr;
static u8 counter_rst;
static u8 ctr;
static s8 fine;
static u8 seq_rx_expected;
static u8 seq_tx_send;
static u8 packet_size;
static u8 binding_idx;


// u8 ptr[4] = {0x01,0x12,0x23,0x30};
// u8 ptr[4] = {0x00,0x11,0x22,0x33};
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
  FRSKY_DATAM
} state, datam_state;

#define TELEM_PKT_SIZE            17
#define HOP_DATA_SIZE             48

static u16 fixed_id;
static u8 packet[MAX_PACKET_SIZE];
static u8 hop_data_v2[HOP_DATA_SIZE];

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


static const u16 CRC_Short[] = {
  0x0000,0x1189,0x2312,0x329b,0x4624,0x57ad,0x6536,0x74bf,
  0x8c48,0x9dc1,0xaf5a,0xbed3,0xca6c,0xdbe5,0xe97e,0xf8f7,
};



static void init_hop_FRSkyX2(void)
{
    u8 inc = (fixed_id % (HOP_DATA_SIZE - 2)) + 1;              // Increment
    if ( inc == 12 || inc == 35 ) inc++;                        // Exception list from dumps
    u8 offset = fixed_id % 5;                                   // Start offset

    u8 channel;
    for (u8 i = 0; i < (HOP_DATA_SIZE - 1); i++)
    {
        channel = 5 * ((u16)(inc * i) % (HOP_DATA_SIZE - 1)) + offset;
        // Exception list from dumps
        if (Model.proto_opts[PROTO_OPTS_FORMAT]) {              // LBT or FCC
            // LBT
            if (channel <= 1 || channel == 43 || channel == 44 || channel == 87 || channel == 88 || channel == 129 || channel == 130 || channel == 173 || channel == 174)
                channel += 2;
            else if (channel == 216 || channel == 217 || channel == 218)
                channel += 3;
        } else {
            // FCC
            if (channel == 3 || channel == 4 || channel == 46 || channel == 47 || channel == 90 || channel == 91  || channel == 133 || channel == 134 || channel == 176 || channel == 177 || channel == 220 || channel == 221)
                channel += 2;
        }
        hop_data_v2[i] = channel;                               // Store
    }
    hop_data_v2[HOP_DATA_SIZE - 1] = 0;                                        // Bind freq
}

static u16 crcTable(u8 val)
{
  u16 word;
  word = pgm_read_word(&CRC_Short[val & 0x0F]);
  val /= 16;
  return word ^ (0x1081 * val);
}

static u16 crc(u8 *data, u8 len) {
  u16 crc = 0;
  for (int i = 0; i < len; i++)
      crc = (crc << 8) ^ crcTable((u8)(crc >> 8) ^ *data++);
  return crc;
}

static void initialize_data(u8 adr)
{
  CC2500_WriteReg(CC2500_0C_FSCTRL0, fine);                     // Frequency offset hack
  CC2500_WriteReg(CC2500_18_MCSM0,    0x8);
  CC2500_WriteReg(CC2500_09_ADDR, adr ? 0x03 : (fixed_id & 0xff));
  CC2500_WriteReg(CC2500_07_PKTCTRL1,0x05);
}


static void set_start(u8 ch) {
  CC2500_Strobe(CC2500_SIDLE);
  CC2500_WriteReg(CC2500_23_FSCAL3, calData[ch][0]);
  CC2500_WriteReg(CC2500_24_FSCAL2, calData[ch][1]);
  CC2500_WriteReg(CC2500_25_FSCAL1, calData[ch][2]);
  if (!Model.proto_opts[PROTO_OPTS_VERSION])
      CC2500_WriteReg(CC2500_0A_CHANNR, ch == 47 ? 0 : hop_data[ch]);
  else
      CC2500_WriteReg(CC2500_0A_CHANNR, hop_data_v2[ch]);
}

#define RXNUM 16
static void frskyX_build_bind_packet()
{
    packet[0] = packet_size;                // Number of bytes in the packet (after this one)
    packet[1] = 0x03;                       // Bind packet
    packet[2] = 0x01;                       // Bind packet

    packet[3] = fixed_id;                   // ID
    packet[4] = fixed_id >> 8;              // ID

    if (!Model.proto_opts[PROTO_OPTS_VERSION]) {
        int idx = ((state - FRSKY_BIND) % 10) * 5;
        packet[5] = idx;
        packet[6] = hop_data[idx++];
        packet[7] = hop_data[idx++];
        packet[8] = hop_data[idx++];
        packet[9] = hop_data[idx++];
        packet[10] = hop_data[idx++];
        packet[11] = 0x02;                  // ID
        packet[12] = RXNUM;

        memset(&packet[13], 0, packet_size - 14);
        if (binding_idx & 0x01)
            memcpy(&packet[13], (void *)"\x55\xAA\x5A\xA5", 4);   // Telem off
        if (binding_idx & 0x02)
            memcpy(&packet[17], (void *)"\x55\xAA\x5A\xA5", 4);   // CH9-16
    } else {
        // packet 1D 03 01 0E 1C 02 00 00 32 0B 00 00 A8 26 28 01 A1 00 00 00 3E F6 87 C7 00 00 00 00 C9 C9
        // Unknown bytes
#ifndef MODULAR
        if (state & 0x01)
            memcpy(&packet[7], "\x00\x18\x0A\x00\x00\xE0\x02\x0B\x01\xD3\x08\x00\x00\x4C\xFE\x87\xC7", 17);
        else
            memcpy(&packet[7], "\x27\xAD\x02\x00\x00\x64\xC8\x46\x00\x64\x00\x00\x00\xFB\xF6\x87\xC7", 17);
#else
        memcpy(&packet[7], "\x00\x32\x0B\x00\x00\xA8\x26\x28\x01\xA1\x00\x00\x00\x3E\xF6\x87\xC7", 17);
#endif
        packet[5] = 0x02;                   // ID
        packet[6] = RXNUM;
        // Bind flags
        if (binding_idx & 0x01)
            packet[7] |= 0x40;              // Telem off
        if (binding_idx & 0x02)
            packet[7] |= 0x80;              // CH9-16
        // Unknown bytes
        packet[20]^= 0x0E ^ fixed_id;       // Update the ID
        packet[21]^= 0x1C ^ fixed_id >> 8;  // Update the ID
        // Xor
        for (u8 i = 3; i < packet_size - 1; i++)
            packet[i] ^= 0xA7;
    }

    u16 lcrc = crc(&packet[3], packet_size - 4);
    packet[packet_size - 1] = lcrc >> 8;
    packet[packet_size] = lcrc;

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

    if (Model.proto_opts[PROTO_OPTS_RSSICHAN] && (chan == Model.num_channels - 1) && !failsafe)
        chan_val = Telemetry.value[TELEM_FRSKY_RSSI] * 21;      // Max RSSI value seems to be 99, scale it to around 2000
    else
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


    packet[0] = packet_size;
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

    for (u8 i = 0; i < 12 ; i += 3) {    // 12 bytes of channel data
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

    packet[21] = seq_rx_expected << 4 | seq_tx_send;

    chan_offset ^= 0x08;

    memset(&packet[22], 0, packet_size - 23);

    u16 lcrc = crc(&packet[3], packet_size - 4);
    packet[packet_size - 1] = lcrc >> 8;
    packet[packet_size] = lcrc;
}


#if HAS_EXTENDED_TELEMETRY

#include "frsky_d_telem._c"
#include "frsky_s_telem._c"

static s8 telem_save_seq;
static u8 telem_save_data[FRSKY_SPORT_PACKET_SIZE];

static void setup_serial_port() {
    if ( Model.proto_opts[PROTO_OPTS_SPORTOUT] ) {
        UART_SetDataRate(57600);    // set for s.port compatibility
#if HAS_EXTENDED_AUDIO
#if HAS_AUDIO_UART
        if (Transmitter.audio_uart) return;
#endif
        Transmitter.audio_player = AUDIO_DISABLED; // disable voice commands on serial port
#endif
    }
}

static void serial_echo(u8 *packet) {
  static u8 outbuf[FRSKY_SPORT_PACKET_SIZE + 1] = {0x7e};

  if (PPMin_Mode() || !Model.proto_opts[PROTO_OPTS_SPORTOUT]) return;

  memcpy(outbuf+1, packet, FRSKY_SPORT_PACKET_SIZE - 1);
  outbuf[FRSKY_SPORT_PACKET_SIZE] = sport_crc(outbuf + 2);
  UART_Send(outbuf, FRSKY_SPORT_PACKET_SIZE + 1);
}

#endif // HAS_EXTENDED_TELEMETRY

static void frsky_check_telemetry(u8 *pkt, u8 len) {
    // only process packets with the required id and packet length and good crc
    if (len == TELEM_PKT_SIZE
        && pkt[0] == TELEM_PKT_SIZE - 3
        && pkt[1] == (fixed_id & 0xff)
        && pkt[2] == (fixed_id >> 8)
        && crc(&pkt[3], TELEM_PKT_SIZE - 7) == (pkt[TELEM_PKT_SIZE - 4] << 8 | pkt[TELEM_PKT_SIZE - 3])
       ) {
        if (pkt[4] & 0x80) {   // distinguish RSSI from VOLT1
            Telemetry.value[TELEM_FRSKY_RSSI] = pkt[4] & 0x7f;
            TELEMETRY_SetUpdated(TELEM_FRSKY_RSSI);
        } else {
            Telemetry.value[TELEM_FRSKY_VOLT1] = pkt[4] * 10;      // In 1/100 of Volts
            TELEMETRY_SetUpdated(TELEM_FRSKY_VOLT1);
        }

        Telemetry.value[TELEM_FRSKY_LQI] = pkt[len - 1] & 0x7f;
        TELEMETRY_SetUpdated(TELEM_FRSKY_LQI);

        Telemetry.value[TELEM_FRSKY_LRSSI] = (s8)pkt[len - 2] / 2 - 70;  // Value in dBm
        TELEMETRY_SetUpdated(TELEM_FRSKY_LRSSI);

        if (((pkt[5] & 0x0f) == 0x08) || ((pkt[5] >> 4) == 0x08)) {   // restart
            seq_rx_expected = 8;
            seq_tx_send = 0;
#if HAS_EXTENDED_TELEMETRY
            dataState = STATE_DATA_IDLE;    // reset sport decoder
#endif
        } else {
            if ((pkt[5] & 0x03) == (seq_rx_expected & 0x03)) {
                seq_rx_expected = (seq_rx_expected + 1) % 4;
#if HAS_EXTENDED_TELEMETRY
                if (pkt[6] <= 6) {
                    for (u8 i = 0; i < pkt[6]; i++)
                        frsky_parse_sport_stream(pkt[7 + i], SPORT_NOCRC);
                }
                // process any saved data from out-of-sequence packet if
                // it's the next expected packet
                if (telem_save_seq == seq_rx_expected) {
                    seq_rx_expected = (seq_rx_expected + 1) % 4;
                    for (u8 i = 0; i < telem_save_data[0]; i++)
                        frsky_parse_sport_stream(telem_save_data[1 + i], SPORT_NOCRC);
                }
                telem_save_seq = -1;
#endif
            }
#if HAS_EXTENDED_TELEMETRY
            else {
                seq_rx_expected = (seq_rx_expected & 0x03) | 0x04;  // incorrect sequence - request resend
                // if this is the packet after the expected one, save the sport data
                if ((pkt[5] & 0x03) == ((seq_rx_expected+1) % 4) && pkt[6] <= 6) {
                    telem_save_seq = (seq_rx_expected+1) % 4;
                    memcpy(telem_save_data, &pkt[6], pkt[6] + 1);
                }
            }
#endif
        }
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


static u16 mixer_runtime;
static u16 frskyx_cb() {
  u8 len;

  switch(state) {
    default:
      set_start(47);
      CC2500_SetPower(Model.tx_power);
      CC2500_Strobe(CC2500_SFRX);
      frskyX_build_bind_packet();
      CC2500_Strobe(CC2500_SIDLE);
      CC2500_WriteData(packet, packet[0] + 1);
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
      if (fine != (s8)Model.proto_opts[PROTO_OPTS_FREQFINE]) {
          fine = (s8)Model.proto_opts[PROTO_OPTS_FREQFINE];
          CC2500_WriteReg(CC2500_0C_FSCTRL0, fine);
      }
      CC2500_SetTxRxMode(TX_EN);
      set_start(channr);
      CC2500_SetPower(Model.tx_power);
      CC2500_Strobe(CC2500_SFRX);
      if (mixer_sync != MIX_DONE && mixer_runtime < 2000) mixer_runtime += 50;
      frskyX_data_frame();
      CC2500_Strobe(CC2500_SIDLE);
      CC2500_WriteData(packet, packet[0] + 1);
      channr = (channr + chanskip) % 47;
      state++;
#ifndef EMULATOR
      return 5200;
#else
      return 52;
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
#ifndef EMULATOR
      if (mixer_runtime <= 500) {
          state = FRSKY_DATA4;
          return 3100;
      } else {
          state = FRSKY_DATAM;
          datam_state = FRSKY_DATA4;
          return 3100 - mixer_runtime;
      }
#else
      state = FRSKY_DATA4;
      return 31;
#endif

    case FRSKY_DATAM:
      state = datam_state;
#ifndef EMULATOR
      CLOCK_RunMixer();
      return mixer_runtime;
#else
      return 5;
#endif

    case FRSKY_DATA4:
      len = CC2500_ReadReg(CC2500_3B_RXBYTES | CC2500_READ_BURST) & 0x7F;
#ifndef EMULATOR
      if (len && len < MAX_PACKET_SIZE) {
          CC2500_ReadData(packet, len);
          frsky_check_telemetry(packet, len);
      }
#else
      (void)len;
      memcpy(packet, &telem_test[telem_idx], sizeof(telem_test[0]));
      telem_idx = (telem_idx + 1) % (sizeof(telem_test)/sizeof(telem_test[0]));
      packet[1] = fixed_id & 0xff;
      packet[2] = fixed_id >> 8;
      frsky_check_telemetry(packet, sizeof(telem_test[0]));
#endif
      if (seq_tx_send != 8) seq_tx_send = (seq_tx_send + 1) % 4;
      state = FRSKY_DATA1;
#ifndef EMULATOR
      if (mixer_runtime <= 500) CLOCK_RunMixer();
      return 500;
#else
      return 5;
#endif
  }
  return 1;
}

// register, FCC, EU
static const u8 init_data[][3] = {
    {CC2500_02_IOCFG0,    0x06,  0x06},
    {CC2500_00_IOCFG2,    0x06,  0x06},
    {CC2500_17_MCSM1,     0x0c,  0x0E},
    {CC2500_18_MCSM0,     0x18,  0x18},
    {CC2500_06_PKTLEN,    0x1E,  0x23},
    {CC2500_07_PKTCTRL1,  0x04,  0x04},
    {CC2500_08_PKTCTRL0,  0x01,  0x01},
    {CC2500_3E_PATABLE,   0xff,  0xff},
    {CC2500_0B_FSCTRL1,   0x0A,  0x08},
    {CC2500_0C_FSCTRL0,   0x00,  0x00},
    {CC2500_0D_FREQ2,     0x5c,  0x5c},
    {CC2500_0E_FREQ1,     0x76,  0x80},
    {CC2500_0F_FREQ0,     0x27,  0x00},
    {CC2500_10_MDMCFG4,   0x7B,  0x7B},
    {CC2500_11_MDMCFG3,   0x61,  0xF8},
    {CC2500_12_MDMCFG2,   0x13,  0x03},
    {CC2500_13_MDMCFG1,   0x23,  0x23},
    {CC2500_14_MDMCFG0,   0x7a,  0x7a},
    {CC2500_15_DEVIATN,   0x51,  0x53},
};

// register, value
static const u8 init_data_shared[][2] = {
    {CC2500_19_FOCCFG,    0x16},
    {CC2500_1A_BSCFG,     0x6c},
    {CC2500_1B_AGCCTRL2,  0x43},
    {CC2500_1C_AGCCTRL1,  0x40},
    {CC2500_1D_AGCCTRL0,  0x91},
    {CC2500_21_FREND1,    0x56},
    {CC2500_22_FREND0,    0x10},
    {CC2500_23_FSCAL3,    0xa9},
    {CC2500_24_FSCAL2,    0x0A},
    {CC2500_25_FSCAL1,    0x00},
    {CC2500_26_FSCAL0,    0x11},
    {CC2500_29_FSTEST,    0x59},
    {CC2500_2C_TEST2,     0x88},
    {CC2500_2D_TEST1,     0x31},
    {CC2500_2E_TEST0,     0x0B},
    {CC2500_03_FIFOTHR,   0x07},
    {CC2500_09_ADDR,      0x00},
};

static void frskyX_init() {
  CC2500_Reset();

  u8 format = Model.proto_opts[PROTO_OPTS_FORMAT] + 1;

  for (u32 i = 0; i < ((sizeof init_data) / (sizeof init_data[0])); i++)
      CC2500_WriteReg(init_data[i][0], init_data[i][format]);
  for (u32 i = 0; i < ((sizeof init_data_shared) / (sizeof init_data_shared[0])); i++)
      CC2500_WriteReg(init_data_shared[i][0], init_data_shared[i][1]);

  if (Model.proto_opts[PROTO_OPTS_VERSION]) {
      CC2500_WriteReg(CC2500_08_PKTCTRL0, 0x05);                // Enable CRC
      if (!Model.proto_opts[PROTO_OPTS_FORMAT]) {               // FCC
          CC2500_WriteReg(CC2500_17_MCSM1, 0x0E);               // Go/Stay in RX mode
          CC2500_WriteReg(CC2500_11_MDMCFG3, 0x84);             // bitrate 70K->77K
      }
  }

  CC2500_WriteReg(CC2500_0C_FSCTRL0, fine);
  CC2500_Strobe(CC2500_SIDLE);

  // calibrate hop channels
  for (u8 c = 0; c < (HOP_DATA_SIZE - 1); c++) {
      CC2500_Strobe(CC2500_SIDLE);
      if (!Model.proto_opts[PROTO_OPTS_VERSION]) {
          CC2500_WriteReg(CC2500_0A_CHANNR, hop_data[c]);
      } else {
          init_hop_FRSkyX2();
          CC2500_WriteReg(CC2500_0A_CHANNR, hop_data_v2[c]);
      }
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
  calData[HOP_DATA_SIZE - 1][0] = CC2500_ReadReg(CC2500_23_FSCAL3);
  calData[HOP_DATA_SIZE - 1][1] = CC2500_ReadReg(CC2500_24_FSCAL2);
  calData[HOP_DATA_SIZE - 1][2] = CC2500_ReadReg(CC2500_25_FSCAL1);
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
    mixer_runtime = 50;

    // initialize statics since 7e modules don't initialize
    fine = Model.proto_opts[PROTO_OPTS_FREQFINE];
    packet_size = 0x1D;
    if (!Model.proto_opts[PROTO_OPTS_VERSION] && Model.proto_opts[PROTO_OPTS_FORMAT])
       packet_size = 0x20;               // FrSkyX V1 LBT
    fixed_id = (u16) get_tx_id();
    failsafe_count = 0;
    chan_offset = 0;
    FS_flag = 0;
    channr = 0;
    chanskip = 0;
    ctr = 0;
    seq_rx_expected = 0;
    seq_tx_send = 8;
    binding_idx = Model.proto_opts[PROTO_OPTS_BINDMODE];
#if HAS_EXTENDED_TELEMETRY
    telem_save_seq = -1;
    setup_serial_port();
#endif

    u32 seed = get_tx_id();
    while (!chanskip)
        chanskip = (rand32_r(&seed, 0) & 0xfefefefe) % (HOP_DATA_SIZE - 1);
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

uintptr_t FRSKYX_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT: initialize(0); return 0;
        case PROTOCMD_CHECK_AUTOBIND: return 0;  // Never Autobind
        case PROTOCMD_BIND:  initialize(1); return 0;
        case PROTOCMD_NUMCHAN: return 16;
        case PROTOCMD_DEFAULT_NUMCHAN: return 8;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id;
        case PROTOCMD_GETOPTIONS:
            if (!Model.proto_opts[PROTO_OPTS_AD2GAIN])
                Model.proto_opts[PROTO_OPTS_AD2GAIN] = 100;  // if not set, default to no gain
            return (uintptr_t)frskyx_opts;
        case PROTOCMD_TELEMETRYSTATE:
            return PROTO_TELEM_ON;
        case PROTOCMD_TELEMETRYTYPE:
            return TELEM_FRSKY;
#if HAS_EXTENDED_TELEMETRY
        case PROTOCMD_TELEMETRYRESET:
            frsky_telem_reset();
            return 0;
#endif
        case PROTOCMD_RESET:
        case PROTOCMD_DEINIT:
#if HAS_EXTENDED_TELEMETRY
            if (Model.proto_opts[PROTO_OPTS_SPORTOUT]) {
                UART_SetDataRate(0);  // restore default rate, voice will be reset on next model's load anyways
            }
#endif
            CLOCK_StopTimer();
            return (CC2500_Reset() ? 1 : -1);
        case PROTOCMD_CHANNELMAP: return AETRG;
        default: break;
    }
    return 0;
}
#endif
