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

static void initialize(int bind);
static void redpine_init(unsigned int format);

static const char * const redpine_opts[] = {
  _tr_noop("Format"),  "Fast", "Slow", NULL,
  _tr_noop("Fast .1ms"),  "1", "250", NULL,
  _tr_noop("Slow 1ms"),  "1", "250", NULL,
  _tr_noop("VTX Band"),  "A", "B", "E", "F", "R",  NULL,
  _tr_noop("VTX Channel"),  "1", "8", NULL,
  _tr_noop("VTX Power"),  "0", "4", NULL,
  _tr_noop("VTX Send"),  "OFF", "SENDING", NULL,
  NULL
};
enum {
    PROTO_OPTS_FORMAT,
    PROTO_OPTS_LOOPTIME_FAST,
    PROTO_OPTS_LOOPTIME_SLOW,
    PROTO_OPTS_VTX_BAND,
    PROTO_OPTS_VTX_CHANNEL,
    PROTO_OPTS_VTX_POWER,
    PROTO_OPTS_VTX_SEND,
    LAST_PROTO_OPT,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

#define PACKET_SIZE 11
// #define REDPINE_FEC_SLOW   // from cc2500 datasheet: The convolutional coder is a rate 1/2 code with a constraint length of m=4
#define NUM_HOPS 50

// Statics are not initialized on 7e so in initialize() if necessary
static u8 calData[NUM_HOPS][3];
static u8 channr;
static u8 ctr;
static s8 fine;

static enum {
  REDPINE_BIND,
#ifndef EMULATOR
  REDPINE_BIND_DONE = 2000,
#else
  REDPINE_BIND_DONE = 50,
#endif
  REDPINE_DATAM,
  REDPINE_DATA1
} state;

static u16 fixed_id;
static u8 packet[PACKET_SIZE];
static u16 mixer_runtime;

static u8 hop_data[NUM_HOPS];

unsigned format;  // todo: use enum

static void initialize_data()
{
    CC2500_WriteReg(CC2500_0C_FSCTRL0, fine);  // Frequency offset hack
    CC2500_WriteReg(CC2500_18_MCSM0, 0x8);
}

static void set_start(u8 ch)
{
    CC2500_Strobe(CC2500_SIDLE);
    CC2500_WriteReg(CC2500_23_FSCAL3, calData[ch][0]);
    CC2500_WriteReg(CC2500_24_FSCAL2, calData[ch][1]);
    CC2500_WriteReg(CC2500_25_FSCAL1, calData[ch][2]);
    CC2500_WriteReg(CC2500_0A_CHANNR, hop_data[ch]);
}

#define RXNUM 16
static void redpine_build_bind_packet()
{
    memset(&packet[0], 0, PACKET_SIZE);

    packet[0] = PACKET_SIZE - 1;
    packet[1] = 0x03;
    packet[2] = 0x01;
    packet[3] = fixed_id;
    packet[4] = fixed_id >> 8;
    int idx = ((state - REDPINE_BIND) % 10) * 5;
    packet[5] = idx;
    packet[6] = hop_data[idx++];
    packet[7] = hop_data[idx++];
    packet[8] = hop_data[idx++];
    packet[9] = hop_data[idx++];
    packet[10] = hop_data[idx++];
    // packet[11] = 0x02;
    // packet[12] = RXNUM;
}

static u16 scaleForRedpine(u8 chan)
{
    s32 chan_val;

    chan_val = Channels[chan] * 15 * 100 / (2 * CHAN_MAX_VALUE) + 1024;

    if (chan_val > 2046)   chan_val = 2046;
    else if (chan_val < 10) chan_val = 10;

    return chan_val;
}


#define GET_FLAG(ch, mask) (Channels[ch] > 0 ? mask : 0)

// For code readability
enum {
    CHANNEL1 = 0,
    CHANNEL2,
    CHANNEL3,
    CHANNEL4,
    CHANNEL5,
    CHANNEL6,
    CHANNEL7,
    CHANNEL8,
    CHANNEL9,
    CHANNEL10,
    CHANNEL11,
    CHANNEL12,
    CHANNEL13,
    CHANNEL14,
    CHANNEL15,
    CHANNEL16
};

static void redpine_data_frame() {
    u16 chan[4];

    memset(&packet[0], 0, PACKET_SIZE);

    packet[0] = PACKET_SIZE - 1;
    packet[1] = fixed_id;
    packet[2] = fixed_id >> 8;

    chan[0] = scaleForRedpine(0);
    chan[1] = scaleForRedpine(1);
    chan[2] = scaleForRedpine(2);
    chan[3] = scaleForRedpine(3);

    packet[3] = chan[0];
    packet[4] = (((chan[0] >> 8) & 0x07) | (chan[1] << 4)) | GET_FLAG(CHANNEL5, 0x08);
    packet[5] = ((chan[1] >> 4) & 0x7F) | GET_FLAG(CHANNEL6, 0x80);
    packet[6] = chan[2];
    packet[7] = (((chan[2] >> 8) & 0x07) | (chan[3] << 4))  | GET_FLAG(CHANNEL7, 0x08);
    packet[8] = ((chan[3] >> 4) & 0x7F) | GET_FLAG(CHANNEL8, 0x80);
    packet[9] = GET_FLAG(CHANNEL9, 0x01)
            | GET_FLAG(CHANNEL10, 0x02)
            | GET_FLAG(CHANNEL11, 0x04)
            | GET_FLAG(CHANNEL12, 0x08)
            | GET_FLAG(CHANNEL13, 0x10)
            | GET_FLAG(CHANNEL14, 0x20)
            | GET_FLAG(CHANNEL15, 0x40)
            | GET_FLAG(CHANNEL16, 0x80);

    if (Model.proto_opts[PROTO_OPTS_FORMAT] == 0) {
        packet[10] = Model.proto_opts[PROTO_OPTS_LOOPTIME_FAST];
    } else {
        packet[10] = Model.proto_opts[PROTO_OPTS_LOOPTIME_SLOW];
    }
}

static void redpine_vtx_frame() {
    static u8 send_num = 0;

    send_num++;
    memset(&packet[0], 0, PACKET_SIZE);

    packet[0] = PACKET_SIZE - 1;
    packet[1] = fixed_id;
    packet[2] = fixed_id >> 8;

    packet[3] = 1;
    packet[4] = 0;
    packet[5] = Model.proto_opts[PROTO_OPTS_VTX_BAND];
    packet[6] = Model.proto_opts[PROTO_OPTS_VTX_CHANNEL];
    packet[7] = Model.proto_opts[PROTO_OPTS_VTX_POWER];

    packet[10] = Model.proto_opts[PROTO_OPTS_LOOPTIME_FAST];

    if (send_num > 20) {
        Model.proto_opts[PROTO_OPTS_VTX_SEND] = 0;
        send_num = 0;
    }
}


static u16 redpine_cb() {
  static u8 status_send = 0;
  switch (state) {
    default:
        if (state == REDPINE_BIND) {
            redpine_init(0);
        }
        set_start(49);
        CC2500_SetPower(Model.tx_power);
        CC2500_Strobe(CC2500_SFRX);
        redpine_build_bind_packet();
        CC2500_Strobe(CC2500_SIDLE);
        CC2500_WriteData(packet, PACKET_SIZE);
        state++;
#ifndef EMULATOR
        return 4000;
#else
        return 40;
#endif
    case REDPINE_BIND_DONE:
        PROTOCOL_SetBindState(0);
        initialize_data();
        redpine_init(format);
        channr = 0;
        state++;
break;

    case REDPINE_DATAM:
#ifndef EMULATOR
        CLOCK_RunMixer();    // clears mixer_sync, which is then set when mixer update complete
        state = REDPINE_DATA1;
        return mixer_runtime;
#else
        return 5;
#endif

    case REDPINE_DATA1:
        if (format != (unsigned)Model.proto_opts[PROTO_OPTS_FORMAT]) {
            format = (unsigned)Model.proto_opts[PROTO_OPTS_FORMAT];
            redpine_init(format);
            mixer_runtime = 50;
            return 5000;
        }

        CC2500_SetTxRxMode(TX_EN);
        set_start(channr);
        CC2500_SetPower(Model.tx_power);
        CC2500_Strobe(CC2500_SFRX);
        if (mixer_sync != MIX_DONE && mixer_runtime < 2000) {
            mixer_runtime += 1;
        }

        if ((unsigned)Model.proto_opts[PROTO_OPTS_VTX_SEND] == 0) {
            redpine_data_frame();
        } else {
            if (status_send == 20) {
                redpine_vtx_frame();
                status_send = 0;
            } else {
                redpine_data_frame();
                status_send++;
            }
        }

        CC2500_Strobe(CC2500_SIDLE);
        channr = (channr + 1) % 49;
        CC2500_WriteData(packet, PACKET_SIZE);
        state = REDPINE_DATAM;
#ifndef EMULATOR
        if (Model.proto_opts[PROTO_OPTS_FORMAT] == 0) {
            return (Model.proto_opts[PROTO_OPTS_LOOPTIME_FAST]*100 - mixer_runtime);
        } else {
            return (Model.proto_opts[PROTO_OPTS_LOOPTIME_SLOW]*1000 - mixer_runtime);
        }
#else
        if (Model.proto_opts[PROTO_OPTS_FORMAT] == 0) {
            return (Model.proto_opts[PROTO_OPTS_LOOPTIME_FAST]);
        } else {
            return (Model.proto_opts[PROTO_OPTS_LOOPTIME_SLOW]);
        }
#endif
  }
  return 1;
}

// register, fast 250k, slow
static const u8 init_data[][3] = {
    {CC2500_00_IOCFG2,    0x06, 0x06},
    {CC2500_02_IOCFG0,    0x06, 0x06},
    {CC2500_03_FIFOTHR,   0x07, 0x07},
    {CC2500_07_PKTCTRL1,  0x04, 0x04},
    {CC2500_08_PKTCTRL0,  0x05, 0x05},
    {CC2500_09_ADDR,      0x00, 0x00},
    {CC2500_0B_FSCTRL1,   0x0A, 0x06},
    {CC2500_0C_FSCTRL0,   0x00, 0x00},
    {CC2500_0D_FREQ2,     0x5D, 0x5D},
    {CC2500_0E_FREQ1,     0x93, 0x93},
    {CC2500_0F_FREQ0,     0xB1, 0xB1},
    {CC2500_10_MDMCFG4,   0x2D, 0x78},
    {CC2500_11_MDMCFG3,   0x3B, 0x93},
    {CC2500_12_MDMCFG2,   0x73, 0x03},
    #ifdef REDPINE_FEC_SLOW
        {CC2500_13_MDMCFG1,   0x23, 0xA2},
    #else
        {CC2500_13_MDMCFG1,   0x23, 0x22},
    #endif
    {CC2500_14_MDMCFG0,   0x56, 0xF8},  // Chan space
    {CC2500_15_DEVIATN,   0x00, 0x44},
    {CC2500_17_MCSM1,     0x0c, 0x0c},
    {CC2500_18_MCSM0,     0x18, 0x18},
    {CC2500_19_FOCCFG,    0x1D, 0x16},
    {CC2500_1A_BSCFG,     0x1C, 0x6c},
    {CC2500_1B_AGCCTRL2,  0xC7, 0x43},
    {CC2500_1C_AGCCTRL1,  0x00, 0x40},
    {CC2500_1D_AGCCTRL0,  0xB0, 0x91},
    {CC2500_21_FREND1,    0xB6, 0x56},
    {CC2500_22_FREND0,    0x10, 0x10},
    {CC2500_23_FSCAL3,    0xEA, 0xA9},
    {CC2500_24_FSCAL2,    0x0A, 0x0A},
    {CC2500_25_FSCAL1,    0x00, 0x00},
    {CC2500_26_FSCAL0,    0x11, 0x11},
    {CC2500_29_FSTEST,    0x59, 0x59},
    {CC2500_2C_TEST2,     0x88, 0x88},
    {CC2500_2D_TEST1,     0x31, 0x31},
    {CC2500_2E_TEST0,     0x0B, 0x0B},
};

static const u8 init_data_shared[][2] = {
    {CC2500_3E_PATABLE,   0xff}
};


static void redpine_init(unsigned int format) {
  CC2500_Reset();

  CC2500_WriteReg(CC2500_06_PKTLEN, PACKET_SIZE);

  for (unsigned i=0; i < ((sizeof init_data) / (sizeof init_data[0])); i++) {
      CC2500_WriteReg(init_data[i][0], init_data[i][format+1]);
  }
  for (unsigned i=0; i < ((sizeof init_data_shared) / (sizeof init_data_shared[0])); i++) {
      CC2500_WriteReg(init_data_shared[i][0], init_data_shared[i][1]);
  }

  CC2500_WriteReg(CC2500_0C_FSCTRL0, fine);
  CC2500_Strobe(CC2500_SIDLE);

  // calibrate hop channels
  for (u8 c = 0; c < sizeof(hop_data); c++) {
      CC2500_Strobe(CC2500_SIDLE);
      CC2500_WriteReg(CC2500_0A_CHANNR, hop_data[c]);
      CC2500_Strobe(CC2500_SCAL);
      usleep(900);
      calData[c][0] = CC2500_ReadReg(CC2500_23_FSCAL3);
      calData[c][1] = CC2500_ReadReg(CC2500_24_FSCAL2);
      calData[c][2] = CC2500_ReadReg(CC2500_25_FSCAL1);
  }
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
    for (u8 i = 0, j = 0; i < sizeof(Model.fixed_id); ++i, j += 8) {
        rand32_r(&lfsr, (Model.fixed_id >> j) & 0xff);
    }
    return rand32_r(&lfsr, 0);
}

static void initialize(int bind)
{
    CLOCK_StopTimer();
    mixer_runtime = 50;

    // initialize statics since 7e modules don't initialize
    fine = 0;
    fixed_id = (u16) get_tx_id();
    channr = 0;
    ctr = 0;
    format = 0;

    // Used from kn_nrf24l01.c : kn_calculate_freqency_hopping_channels
    u32 idx = 0;
    u32 rnd = get_tx_id();
    #define MAX_RF_CHANNEL 255
    while (idx < sizeof(hop_data)-1) {
        u32 i;
        rnd = rnd * 0x0019660D + 0x3C6EF35F;  // Randomization
        // Drop least-significant byte for better randomization. Start from 1
        u8 next_ch = (rnd >> 8) % MAX_RF_CHANNEL + 1;
        // Check that it's not duplicate nor adjacent nor channel 0 or 1
        for (i = 0; i < idx; i++) {
            u8 ch = hop_data[i];
            if ((ch <= next_ch + 1) && (ch >= next_ch - 1) && (ch >= 1)) {
                break;
            }
        }
        if (i != idx) {
            continue;
        }
        hop_data[idx++] = next_ch;
    }
    hop_data[49] = 0;  // Last channel is the bind channel at hop 0

    redpine_init(format);
    CC2500_SetTxRxMode(TX_EN);  // enable PA

    if (bind) {
        PROTOCOL_SetBindState(0xFFFFFFFF);
        state = REDPINE_BIND;
    } else {
        state = REDPINE_DATA1;
    }
    initialize_data();

#ifndef EMULATOR
    CLOCK_StartTimer(10000, redpine_cb);
#else
    CLOCK_StartTimer(100, redpine_cb);
#endif
}

uintptr_t REDPINE_Cmds(enum ProtoCmds cmd)
{
    switch (cmd) {
        case PROTOCMD_INIT: initialize(0); return 0;
        case PROTOCMD_CHECK_AUTOBIND: return 0;  // Never Autobind
        case PROTOCMD_BIND:  initialize(1); return 0;
        case PROTOCMD_NUMCHAN: return 16L;
        case PROTOCMD_DEFAULT_NUMCHAN: return 16L;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id;
        case PROTOCMD_GETOPTIONS:
            if (!Model.proto_opts[PROTO_OPTS_LOOPTIME_FAST]) {
                Model.proto_opts[PROTO_OPTS_LOOPTIME_FAST] = 20;  // if not set, default to no gain
            }
            if (!Model.proto_opts[PROTO_OPTS_LOOPTIME_SLOW]) {
                Model.proto_opts[PROTO_OPTS_LOOPTIME_SLOW] = 19;  // if not set, default to no gain
            }
            Model.proto_opts[PROTO_OPTS_VTX_SEND] = 0;  // if not set, default to no gain

            return (uintptr_t)redpine_opts;
        case PROTOCMD_RESET:
        case PROTOCMD_DEINIT:
            CLOCK_StopTimer();
            return (CC2500_Reset() ? 1 : -1);
        case PROTOCMD_TELEMETRYSTATE: return PROTO_TELEM_UNSUPPORTED;
        case PROTOCMD_CHANNELMAP: return AETRG;
        default: break;
    }
    return 0;
}
#endif
