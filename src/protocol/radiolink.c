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
#include "protospi.h"

#ifdef PROTO_HAS_RADIOLINK_CC2530

#ifdef MODULAR
  //Allows the linker to properly relocate
  #define RADIOLINK_Cmds PROTO_Cmds
  #pragma long_calls
#endif

#define PACKET_SIZE     41
#define PACKET_INTERVAL 4000

static u8 packet[PACKET_SIZE];

static u16 scale_channel(s32 chanval, s32 inMin, s32 inMax, u16 destMin, u16 destMax)
{
    s32 range = (s32)destMax - (s32)destMin;
    s32 chanrange = inMax - inMin;

    if (chanval < inMin)
        chanval = inMin;
    else if (chanval > inMax)
        chanval = inMax;
    return (range * (chanval - inMin)) / chanrange + destMin;
}

static void send_packet()
{
    u8 i = 0;
    u8 chan;
    u16 val;
    packet[i++] = 0xaa;
    packet[i++] = 0xaa;
    packet[i++] = 0x29;
    packet[i++] = 0x01;
    // channels
    for(chan = 0; chan < 10; chan++) {
        // is 0-4000 for 100% endpoints ? can it be extended ?
        val = scale_channel(Channels[chan], CHAN_MIN_VALUE, CHAN_MAX_VALUE, 4000, 0);
        packet[i++] = val & 0xff;
        packet[i++] = val >> 8;
    }
    // failsafe
    for(chan = 0; chan < 8; chan++) {
        // any way to disable failsafe on a channel ? (keep last received value)
        val = scale_channel(Channels[chan], -100, 100, 4000, 0);
        packet[i++] = val & 0xff;
        packet[i++] = val >> 8;
    }
    // checksum
    packet[40] = packet[4];
    for(i = 5; i < PACKET_SIZE-1; i++)
        packet[40] += packet[i];
    
    // send packet to Radiolink CC2530 module
    PROTO_CS_LO(RADIOLINK_MODULE)
    for(i = 0; i < PACKET_SIZE; i++)
            packet[i] = PROTOSPI_xfer(packet[i]);
    PROTO_CS_HI(RADIOLINK_MODULE)
    
    // todo: packet[] now contains the module's response, with telemetry data
}

static u16 radiolink_cb()
{
    send_packet();
    return PACKET_INTERVAL;
}

static void RADIOLINK_Reset()
{
    PROTO_CS_HI(RADIOLINK_MODULE)
    PROTOSPI_pin_clear(RADIOLINK_MODULE_RESET_PIN);
    Delay(100);
    PROTOSPI_pin_set(RADIOLINK_MODULE_RESET_PIN);
}

static void RADIOLINK_Stop()
{
    PROTO_CS_HI(RADIOLINK_MODULE);
    PROTOSPI_pin_clear(RADIOLINK_MODULE_RESET_PIN);
}

static void initialize()
{
    CLOCK_StopTimer();
    RADIOLINK_Reset();
    CLOCK_StartTimer(1000, radiolink_cb);
}

uintptr_t RADIOLINK_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT: RADIOLINK_Stop(); return 0;
        case PROTOCMD_CHECK_AUTOBIND: return 1L;
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return 10;
        case PROTOCMD_DEFAULT_NUMCHAN: return 10;
        case PROTOCMD_CHANNELMAP: return AETR;
        case PROTOCMD_TELEMETRYSTATE: return PROTO_TELEM_UNSUPPORTED;
        default: break;
    }
    return 0;
}

#endif
