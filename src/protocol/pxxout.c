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
#if HAS_EXTENDED_TELEMETRY
#include "telemetry.h"
#endif

#define PXX_SEND_BIND           0x01
#define PXX_SEND_FAILSAFE       (1 << 4)
#define PXX_SEND_RANGECHECK     (1 << 5)

#define START_STOP    0x7e
#define PXX_PKT_BYTES 18
#define PW_HIGH        8    //  8 microcseconds high, rest of pulse low
#define PW_ZERO       16    // 16 microsecond pulse is zero
#define PW_ONE        24    // 24 microsecond pulse is one

static const char * const pxx_opts[] = {
  _tr_noop("Failsafe"), "Hold", "NoPulse", "RX", NULL,
  _tr_noop("Country"), "US", "JP", "EU", NULL,
  _tr_noop("Rx PWM out"), "1-8", "9-16", NULL,
  _tr_noop("Rx Telem"), "On", "Off", NULL,
  NULL
};
enum {
    PROTO_OPTS_FAILSAFE,
    PROTO_OPTS_COUNTRY,
    PROTO_OPTS_RXPWM,
    PROTO_OPTS_RXTELEM,
    LAST_PROTO_OPT,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

#define FAILSAFE_HOLD    0
#define FAILSAFE_NOPULSE 1
#define FAILSAFE_RX      2


static u8 packet[PXX_PKT_BYTES];
static u16 failsafe_count;
static u8 chan_offset;
static u8 FS_flag;
 
enum XJTRFProtocols {
  RF_PROTO_OFF = -1,
  RF_PROTO_X16,
  RF_PROTO_D8,
  RF_PROTO_LR12,
  RF_PROTO_LAST = RF_PROTO_LR12
};

enum R9MSubTypes
{
  MODULE_SUBTYPE_R9M_FCC,
  MODULE_SUBTYPE_R9M_LBT,
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

static u8 power_to_r9m() {
    if (Model.tx_power >= TXPOWER_10mW) return Model.tx_power - TXPOWER_10mW;
    return TXPOWER_100uW;
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

#if 0
    if (Model.proto_opts[PROTO_OPTS_RSSICHAN] && (chan == Model.num_channels - 1) && !failsafe)
        chan_val = Telemetry.value[TELEM_FRSKY_RSSI] * 21;      // Max RSSI value seems to be 99, scale it to around 2000
    else
#endif
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

static void build_data_pkt(u8 bind)
{
    u16 chan_0;
    u16 chan_1;
    u8 startChan = chan_offset;

    // data frames sent every 8ms; failsafe every 8 seconds
    if (FS_flag == 0 && failsafe_count > FAILSAFE_TIMEOUT && chan_offset == 0 &&  Model.proto_opts[PROTO_OPTS_FAILSAFE] != FAILSAFE_RX) {
        FS_flag = 0x10;
    } else if (FS_flag & 0x10 && chan_offset == 0) {
        FS_flag = 0;
        failsafe_count = 0;
    }
    failsafe_count += 1;

    // only need packet contents here. Start and end flags added by pxx_enable
//  packet[0] = set in initialize() to tx id

    // FLAG1,
    // b0: BIND / set Rx number -> if this bit is set, every rx listening
    // should change it's rx number to the one described in byte 2
    // b4: if 1, set failsafe positions for all channels
    // b5: rangecheck if set
    // b6..b7: rf protocol
    packet[1] = RF_PROTO_X16 << 6;
    if (bind) {
        // if b0, then b1..b2 = country code (us 0, japan 1, eu 2 ?)
        packet[1] |= PXX_SEND_BIND | (Model.proto_opts[PROTO_OPTS_COUNTRY] << 1);
    } else if (Model.tx_power == TXPOWER_100uW) {   // RANGE_test() sets power to 100uW
        packet[1] |= PXX_SEND_RANGECHECK;
    } else {
        packet[1] |= FS_flag;
    }

    packet[2] = 0;  // FLAG2, Reserved for future use, must be “0” in this version.

    for(u8 i = 0; i < 12 ; i += 3) {    // 12 bytes of channel data
        chan_0 = scaleForPXX(startChan++, FS_flag == 0x10 ? 1 : 0);
        chan_1 = scaleForPXX(startChan++, FS_flag == 0x10 ? 1 : 0);

        packet[3+i]   = chan_0;
        packet[3+i+1] = (((chan_0 >> 8) & 0x0F) | (chan_1 << 4));
        packet[3+i+2] = chan_1 >> 4;
    }

    // extra_flags byte definitions pulled from openTX
    // b0: antenna selection on Horus and Xlite
    // b1: turn off telemetry at receiver
    // b2: set receiver PWM output to channels 9-16
    // b3-4: RF power setting
    // b5: set to disable R9M S.Port output
    packet[15] = (power_to_r9m() << 3)
               | (Model.proto_opts[PROTO_OPTS_RXTELEM] << 1)
               | (Model.proto_opts[PROTO_OPTS_RXPWM] << 2);

    u16 lcrc = crc(packet, PXX_PKT_BYTES-2);
    packet[16] = lcrc >> 8;
    packet[17] = lcrc;

    chan_offset ^= 0x08;
}

static enum {
  PXX_BIND,
#ifndef EMULATOR
  PXX_BIND_DONE = 600,
#else
  PXX_BIND_DONE = 5,
#endif
  PXX_DATA1,
  PXX_DATA2,
} state;

static u16 mixer_runtime;

#if HAS_EXTENDED_TELEMETRY
// Support S.Port telemetry on RX pin
// couple defines to avoid errors from include file
static void serial_echo(u8 *packet) {(void)packet;}
#define PROTO_OPTS_AD2GAIN 0
#include "frsky_d_telem._c"
#include "frsky_s_telem._c"
#endif  // HAS_EXTENDED_TELEMETRY

#ifndef EMULATOR
#define STD_DELAY   9000
#else
#define STD_DELAY   300
#endif
static u16 pxxout_cb()
{
    switch (state) {
    default: // binding
        build_data_pkt(1);
        PXX_Enable(packet);
        state++;
        return STD_DELAY;
    case PXX_BIND_DONE:
        PROTOCOL_SetBindState(0);
        state++;
        // intentional fall-through
    case PXX_DATA1:
        CLOCK_RunMixer();    // clears mixer_sync, which is then set when mixer update complete
        state = PXX_DATA2;
        return mixer_runtime;
    case PXX_DATA2:
#ifndef EMULATOR
        if (mixer_sync != MIX_DONE && mixer_runtime < 2000) mixer_runtime += 50;
#endif
        build_data_pkt(0);
        PXX_Enable(packet);
        state = PXX_DATA1;
        return STD_DELAY - mixer_runtime;
    }
}

static void initialize(u8 bind)
{
    CLOCK_StopTimer();
    if (PPMin_Mode())
        return;

#if HAS_EXTENDED_AUDIO
#if HAS_AUDIO_UART
    if (!Transmitter.audio_uart)
#endif
        Transmitter.audio_player = AUDIO_DISABLED; // disable voice commands on serial port
#endif
#if HAS_EXTENDED_TELEMETRY
    SSER_Initialize(); // soft serial receiver
    SSER_StartReceive(frsky_parse_sport_stream);
#endif

    PWM_Initialize();

    failsafe_count = 0;
    chan_offset = 0;
    FS_flag = 0;
    packet[0] = (u8) Model.fixed_id & 0x3f;  // limit to valid range - 6 bits
    mixer_runtime = 50;

    if (bind) {
        state = PXX_BIND;
        PROTOCOL_SetBindState(5000);
    } else {
        state = PXX_DATA1;
    }
    CLOCK_StartTimer(1000, pxxout_cb);
}


uintptr_t PXXOUT_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(0); return 0;
        case PROTOCMD_DEINIT:
          PWM_Stop();
#if HAS_EXTENDED_TELEMETRY
          SSER_Stop();
#endif
          return 0;
        case PROTOCMD_CHECK_AUTOBIND: return 0;
        case PROTOCMD_BIND:  initialize(1); return 0;
        case PROTOCMD_NUMCHAN: return 16;
        case PROTOCMD_DEFAULT_NUMCHAN: return 8;
        case PROTOCMD_GETOPTIONS: return (uintptr_t)pxx_opts;
#if HAS_EXTENDED_TELEMETRY
        case PROTOCMD_TELEMETRYSTATE:
            return PROTO_TELEM_ON;
        case PROTOCMD_TELEMETRYTYPE:
            return TELEM_FRSKY;
        case PROTOCMD_TELEMETRYRESET:
            frsky_telem_reset();
            return 0;
#endif
        case PROTOCMD_CHANNELMAP: return UNCHG;
        default: break;
    }
    return 0;
}
