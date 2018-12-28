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
  #define USBHID_Cmds PROTO_Cmds
  #pragma long_calls
#endif

#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"

#ifdef MODULAR
  #pragma long_calls_off
  extern unsigned _data_loadaddr;
  const unsigned long protocol_type = (unsigned long)&_data_loadaddr;
#endif
//To change USBHID_MAX_CHANNELS you must change the Report_Descriptor in hid_usb_desc.c as well
#define USBHID_ANALOG_CHANNELS 8
#define USBHID_DIGITAL_CHANNELS 4
#define USBHID_MAX_CHANNELS (USBHID_ANALOG_CHANNELS + USBHID_DIGITAL_CHANNELS)
#if USBHID_DIGITAL_CHANNELS > 8
    //We only allow one byte of digital channels
    #error "USBHID_DIGITAL_CHANNELS must be <= 8"
#endif
//if sizeof(packet) changes, must change wMaxPacketSize to match in Joystick_ConfigDescriptor
static s8 packet[USBHID_ANALOG_CHANNELS + 1];
static u8 num_channels;
volatile u8 PrevXferComplete;
extern void HID_Write(s8 *packet, u8 num_channels);

static void build_data_pkt()
{
    int i;
    for (i = 0; i < USBHID_ANALOG_CHANNELS; i++) {
        if (i >= num_channels) {
            packet[i] = 0;
            continue;
        }
        s32 value = RANGE_TO_PCT(Channels[i]);
        if (value > 127)
            value = 127;
        else if (value < -127)
            value = -127;
        packet[i] = value;
    }
    u8 digital = 0;
    for (i = 0; i < USBHID_DIGITAL_CHANNELS; i++) {
        if (i >= num_channels)
            continue;
        s32 value = RANGE_TO_PCT(Channels[USBHID_ANALOG_CHANNELS+i]);
        if (value > 0)
            digital |= (1 << i);
    }
    packet[USBHID_ANALOG_CHANNELS] = digital;
}

MODULE_CALLTYPE
static u16 usbhid_cb()
{
    if(PrevXferComplete) {
        build_data_pkt();
        
        HID_Write(packet, sizeof(packet));
    }
    return 50000;
}

static void initialize()
{
    CLOCK_StopTimer();
    num_channels = Model.num_channels;
    PrevXferComplete = 1;
    HID_Enable();
    CLOCK_StartTimer(1000, usbhid_cb);
}

const void * USBHID_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT: HID_Disable(); return 0;
        case PROTOCMD_CHECK_AUTOBIND: return (void *)1L;
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return (void *)((unsigned long)USBHID_MAX_CHANNELS);
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)6L;
	case PROTOCMD_CHANNELMAP: return UNCHG;
	case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        default: break;
    }
    return 0;
}
