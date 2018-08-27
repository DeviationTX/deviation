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
  #define SBUS_Cmds PROTO_Cmds
  #pragma long_calls
#endif

#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"
#include "config/tx.h"
#include "telemetry.h"

#ifdef MODULAR
  #pragma long_calls_off
  extern unsigned _data_loadaddr;
  const unsigned long protocol_type = (unsigned long)&_data_loadaddr;
#endif

#define SBUS_DATARATE             100000
#define SBUS_FRAME_PERIOD         14000   // 14ms
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

static u16 serial_cb()
{
    build_rcdata_pkt();
    UART_Send(packet, sizeof packet);

    return SBUS_FRAME_PERIOD;
}

static void initialize()
{
    CLOCK_StopTimer();
    if (PPMin_Mode())
    {
        return;
    }
#if HAS_EXTENDED_AUDIO
#if HAS_AUDIO_UART5
    if (!Transmitter.audio_uart5)
#endif
    Transmitter.audio_player = AUDIO_DISABLED; // disable voice commands on serial port
#endif
    UART_Initialize();
    UART_SetDataRate(SBUS_DATARATE);
	UART_SetFormat(8, UART_PARITY_EVEN, UART_STOPBITS_2);

    CLOCK_StartTimer(1000, serial_cb);
}

const void * SBUS_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT: UART_Initialize(); return 0;
        case PROTOCMD_CHECK_AUTOBIND: return (void *)1L;
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return (void *)16L;
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)8L;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        default: break;
    }
    return 0;
}
