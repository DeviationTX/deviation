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
  #define TESTSER_Cmds PROTO_Cmds
  #pragma long_calls
#endif

#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"
#include "config/tx.h"

#ifdef MODULAR
  #pragma long_calls_off
  extern unsigned _data_loadaddr;
  const unsigned long protocol_type = (unsigned long)&_data_loadaddr;
#endif

#define TESTSER_PACKET_SIZE 64
static u8 packet[TESTSER_PACKET_SIZE];
static volatile u32 packet_idx;

/* This ISR signature required for receiving
   serial data.  Must match usart_callback_t type.
*/
void isr_callback(u8 data, u8 status) {
    if (status & UART_RX_RXNE) {
        packet[packet_idx] = data;
        if (packet_idx < TESTSER_PACKET_SIZE) packet_idx++;
    }
}

static u16 serial_cb()
{
    if (packet_idx > 0) {
        UART_Send(packet, packet_idx);
        packet_idx = 0;
    }
    return 10000;
}

static void initialize()
{
    const char *teststr = "Hello, World!\n";

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
    UART_SetDataRate(115200);
    UART_SetReceive(isr_callback);
    memcpy((void *)packet, (const void *)teststr, strlen(teststr));
    UART_Send(packet, strlen(teststr));
    CLOCK_StartTimer(10000, serial_cb);
}

const void * TESTSER_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT: return 0;
        case PROTOCMD_CHECK_AUTOBIND: return (void *)1L;
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return (void *)16L;
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)8L;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        default: break;
    }
    return 0;
}
