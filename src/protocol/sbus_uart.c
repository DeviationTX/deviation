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

static const char * const sbus_opts[] = {
  _tr_noop("Period (ms)"),  "6", "14", NULL,
  NULL
};
enum {
    PROTO_OPTS_PERIOD,
    LAST_PROTO_OPT,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

#define SBUS_DATARATE             100000
#define SBUS_FRAME_PERIOD_MAX     14000   // 14ms
#define SBUS_CHANNELS             16
#define SBUS_PACKET_SIZE          25

static u8 packet[SBUS_PACKET_SIZE];

//#define STICK_SCALE    869  // full scale at +-125
#define STICK_SCALE    800  // +/-100 gives 2000/1000 us
static void build_rcdata_pkt()
{
    int i;
	u16 channels[SBUS_CHANNELS];

    for (i=0; i < SBUS_CHANNELS; i++) {
        if (i < Model.num_channels)
            channels[i] = (u16)(Channels[i] * STICK_SCALE / CHAN_MAX_VALUE + 992);
        else
            channels[i] = 992;  // midpoint
    }

	packet[0] = 0x0f; 

  	packet[1] = (u8) ((channels[0] & 0x07FF));
  	packet[2] = (u8) ((channels[0] & 0x07FF)>>8 | (channels[1] & 0x07FF)<<3);
  	packet[3] = (u8) ((channels[1] & 0x07FF)>>5 | (channels[2] & 0x07FF)<<6);
  	packet[4] = (u8) ((channels[2] & 0x07FF)>>2);
  	packet[5] = (u8) ((channels[2] & 0x07FF)>>10 | (channels[3] & 0x07FF)<<1);
  	packet[6] = (u8) ((channels[3] & 0x07FF)>>7 | (channels[4] & 0x07FF)<<4);
  	packet[7] = (u8) ((channels[4] & 0x07FF)>>4 | (channels[5] & 0x07FF)<<7);
  	packet[8] = (u8) ((channels[5] & 0x07FF)>>1);
  	packet[9] = (u8) ((channels[5] & 0x07FF)>>9 | (channels[6] & 0x07FF)<<2);
  	packet[10] = (u8) ((channels[6] & 0x07FF)>>6 | (channels[7] & 0x07FF)<<5);
  	packet[11] = (u8) ((channels[7] & 0x07FF)>>3);
  	packet[12] = (u8) ((channels[8] & 0x07FF));
  	packet[13] = (u8) ((channels[8] & 0x07FF)>>8 | (channels[9] & 0x07FF)<<3);
  	packet[14] = (u8) ((channels[9] & 0x07FF)>>5 | (channels[10] & 0x07FF)<<6);  
  	packet[15] = (u8) ((channels[10] & 0x07FF)>>2);
  	packet[16] = (u8) ((channels[10] & 0x07FF)>>10 | (channels[11] & 0x07FF)<<1);
  	packet[17] = (u8) ((channels[11] & 0x07FF)>>7 | (channels[12] & 0x07FF)<<4);
  	packet[18] = (u8) ((channels[12] & 0x07FF)>>4 | (channels[13] & 0x07FF)<<7);
  	packet[19] = (u8) ((channels[13] & 0x07FF)>>1);
  	packet[20] = (u8) ((channels[13] & 0x07FF)>>9 | (channels[14] & 0x07FF)<<2);
  	packet[21] = (u8) ((channels[14] & 0x07FF)>>6 | (channels[15] & 0x07FF)<<5);
  	packet[22] = (u8) ((channels[15] & 0x07FF)>>3);

	packet[23] = 0x00; // flags
	packet[24] = 0x00;
}

// static u8 testrxframe[] = { 0x00, 0x0C, 0x14, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x01, 0x03, 0x00, 0x00, 0x00, 0xF4 };

static enum {
    ST_DATA1,
    ST_DATA2,
} state;

static u16 mixer_runtime;
static u16 sbus_period;
static u16 serial_cb()
{
    if (sbus_period != Model.proto_opts[PROTO_OPTS_PERIOD] * 1000)
        sbus_period = Model.proto_opts[PROTO_OPTS_PERIOD] * 1000;

    switch (state) {
    case ST_DATA1:
        CLOCK_RunMixer();    // clears mixer_sync, which is then set when mixer update complete
        state = ST_DATA2;
        return mixer_runtime;

    case ST_DATA2:
        if (mixer_sync != MIX_DONE && mixer_runtime < 2000) mixer_runtime += 50;
        build_rcdata_pkt();
        UART_Send(packet, sizeof packet);
        state = ST_DATA1;
        return sbus_period - mixer_runtime;
    }
    return sbus_period;   // avoid compiler warning
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
    Transmitter.audio_player = AUDIO_DISABLED; // disable voice commands on serial port
#endif
    UART_Initialize();
    UART_SetDataRate(SBUS_DATARATE);
	UART_SetFormat(8, UART_PARITY_EVEN, UART_STOPBITS_2);
    state = ST_DATA1;
    mixer_runtime = 50;
    sbus_period = Model.proto_opts[PROTO_OPTS_PERIOD] ? (Model.proto_opts[PROTO_OPTS_PERIOD] * 1000) : SBUS_FRAME_PERIOD_MAX;

    CLOCK_StartTimer(1000, serial_cb);
}

uintptr_t SBUS_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
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
                Model.proto_opts[PROTO_OPTS_PERIOD] = SBUS_FRAME_PERIOD_MAX;
            return (uintptr_t)sbus_opts;
        default: break;
    }
    return 0;
}
