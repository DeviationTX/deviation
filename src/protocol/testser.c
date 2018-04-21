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
static s16 speed, databits, parity, stopbits, duplex;

static const char * const testser_opts[] = {
  "Mode", "Loopback", "Send H", NULL,
  "Speed", "115200", "9600", "1000000", "400000", NULL,
  "Bits", "Eight", "Seven", NULL,
  "Parity", "None", "Even", "Odd", NULL,
  "StopBits", "One", "One+", "Two", NULL,
  "Duplex", "Full", "Half", NULL,
  NULL
};
enum {
    TESTSER_MODE = 0,
    TESTSER_SPEED,
    TESTSER_BITS,
    TESTSER_PARITY,
    TESTSER_STOP,
    TESTSER_DUPLEX,
    LAST_PROTO_OPT,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

void check_options(u8 init) {
    uart_parity newparity;
    uart_stopbits newstopbits;

    if (init || Model.proto_opts[TESTSER_SPEED] != speed) {
        speed = Model.proto_opts[TESTSER_SPEED];
        switch(speed) {
        case 0: UART_SetDataRate(115200); break;
        case 1: UART_SetDataRate(9600); break;
        case 2: UART_SetDataRate(100000); break;
        case 3: UART_SetDataRate(400000); break;
        }
    }

    if (init
     || Model.proto_opts[TESTSER_BITS] != databits
     || Model.proto_opts[TESTSER_PARITY] != parity
     || Model.proto_opts[TESTSER_STOP] != stopbits) {
        databits = Model.proto_opts[TESTSER_BITS];
        parity = Model.proto_opts[TESTSER_PARITY];
        stopbits = Model.proto_opts[TESTSER_STOP];
        switch(parity) {
        case 1:  newparity = UART_PARITY_EVEN; break;
        case 2:  newparity = UART_PARITY_ODD; break;
        default: newparity = UART_PARITY_NONE; break;
        }
        switch(stopbits) {
        case 1:  newstopbits = UART_STOPBITS_1_5; break;
        case 2:  newstopbits = UART_STOPBITS_2; break;
        default: newstopbits = UART_STOPBITS_1; break;
        }
        UART_SetFormat(databits, newparity, newstopbits);
    }

    if (init || Model.proto_opts[TESTSER_DUPLEX] != duplex) {
        duplex = Model.proto_opts[TESTSER_DUPLEX];
        UART_SetDuplex(duplex ? UART_DUPLEX_HALF : UART_DUPLEX_FULL);
    }
}


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
    int i;

    check_options(0);

    switch(Model.proto_opts[TESTSER_MODE]) {
    case 0: // Loopback
        if (packet_idx > 0) {
            UART_Send(packet, packet_idx);
            packet_idx = 0;
        }
        break;

    case 1: // Send H character
        for (i=0; i <= 20; i++) packet[i] = 'H';
        UART_Send(packet, 20);
        break;
    }
    return 4000;
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

    check_options(1);
    UART_SetReceive(isr_callback);
    CLOCK_StartTimer(10000, serial_cb);
}

const void * TESTSER_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT: UART_Stop(); return 0;
        case PROTOCMD_CHECK_AUTOBIND: return (void *)1L;
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return (void *)16L;
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)8L;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        case PROTOCMD_GETOPTIONS: return testser_opts;
        default: break;
    }
    return 0;
}
