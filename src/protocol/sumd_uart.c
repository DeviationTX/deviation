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

static const char * const sumd_opts[] = {
  _tr_noop("Period (ms)"),  "6", "14", NULL,
  NULL
};
enum {
    PROTO_OPTS_PERIOD,
    LAST_PROTO_OPT,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

#define SUMD_DATARATE             115200
#define SUMD_FRAME_PERIOD_STD     10000   // 10ms in spec
#define SUMD_MAX_CHANNELS         16      // Spec supports 32, deviation current max is 16
#define SUMD_MAX_PACKET_SIZE      (3 + 2*SUMD_MAX_CHANNELS + 2)   // 3 header bytes, 16bit channels, 16bit CRC

static u8 packet[SUMD_MAX_PACKET_SIZE];

#define CRC_POLYNOME 0x1021
static u16 crc16(u16 crc, u8 value) {
    u8 i;

    crc = crc ^ (s16)value << 8;

    for (i=0; i < 8; i++) {
        if (crc & 0x8000)
            crc = (crc << 1) ^ CRC_POLYNOME;
        else
            crc = (crc << 1);
    }
    return crc;
}

static u16 crc(u8 *data, u8 len) {
  u16 crc = 0;
  for (int i=0; i < len; i++)
      crc = crc16(crc, *data++);
  return crc;
}


// #define STICK_SCALE    869  // full scale at +-125
#define STICK_SCALE     3200  // +/-100 gives 15200/8800
#define STICK_CENTER   12000
static int build_rcdata_pkt()
{
    u16 chanval;
    u16 crc_val = 0;
    int j = 0;


    packet[j++] = 0xa8;     // manufacturer id
    packet[j++] = 0x01;     // 0x01 normal packet, 0x81 failsafe setting
    packet[j++] = Model.num_channels;

    for (int i=0; i < Model.num_channels; i++) {
        chanval = (u16)(Channels[i] * STICK_SCALE / CHAN_MAX_VALUE + STICK_CENTER);
        packet[j++] = chanval >> 8;
        packet[j++] = chanval;
    }

    crc_val = crc(packet, j);
    packet[j++] = crc_val >> 8;
    packet[j++] = crc_val;

    return j;
}


static enum {
    ST_DATA1,
    ST_DATA2,
} state;

static u16 mixer_runtime;
static u16 sumd_period;
static u16 serial_cb() {
    if (sumd_period != Model.proto_opts[PROTO_OPTS_PERIOD] * 1000)
        sumd_period = Model.proto_opts[PROTO_OPTS_PERIOD] * 1000;

    switch (state) {
    case ST_DATA1:
        CLOCK_RunMixer();    // clears mixer_sync, which is then set when mixer update complete
        state = ST_DATA2;
        return mixer_runtime;

    case ST_DATA2:
        if (mixer_sync != MIX_DONE && mixer_runtime < 2000) mixer_runtime += 50;
        UART_Send(packet, build_rcdata_pkt());
        state = ST_DATA1;
        return sumd_period - mixer_runtime;
    }
    return sumd_period;   // avoid compiler warning
}

static void initialize()
{
    CLOCK_StopTimer();
    if (PPMin_Mode())
    {
        return;
    }
#if HAS_EXTENDED_AUDIO
#if HAS_AUDIO_UART
    if (!Transmitter.audio_uart)
#endif
    Transmitter.audio_player = AUDIO_DISABLED;  // disable voice commands on serial port
#endif
    UART_Initialize();
    UART_SetDataRate(SUMD_DATARATE);
    state = ST_DATA1;
    mixer_runtime = 50;
    sumd_period = Model.proto_opts[PROTO_OPTS_PERIOD] ? (Model.proto_opts[PROTO_OPTS_PERIOD] * 1000) : SUMD_FRAME_PERIOD_STD;

    CLOCK_StartTimer(1000, serial_cb);
}

uintptr_t SUMD_Cmds(enum ProtoCmds cmd)
{
    switch (cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT: UART_Initialize(); return 0;
        case PROTOCMD_CHECK_AUTOBIND: return 1;
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return 16;
        case PROTOCMD_DEFAULT_NUMCHAN: return 8;
        case PROTOCMD_CHANNELMAP: return UNCHG;
        case PROTOCMD_TELEMETRYSTATE: return PROTO_TELEM_UNSUPPORTED;
        case PROTOCMD_GETOPTIONS:
            if (!Model.proto_opts[PROTO_OPTS_PERIOD])
                Model.proto_opts[PROTO_OPTS_PERIOD] = SUMD_FRAME_PERIOD_STD;
            return (uintptr_t)sumd_opts;
        default: break;
    }
    return 0;
}
