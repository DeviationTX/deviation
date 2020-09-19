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

static const char * const usbhid_opts[] = {
  _tr_noop("Period (Hz)"),  "125", "250", "500", "1000", NULL,
  NULL
};
enum {
    PROTO_OPTS_PERIOD,
    LAST_PROTO_OPT,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

# define USBHID_PERIOD_MAX_INDEX 3
static u16 period_index_to_ms(s16 idx)
{
    switch (idx) {
        case 3: return 1;
        case 2: return 2;
        case 1: return 4;
        default: return 8;
    }
    return 8;
}

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

static enum {
    ST_DATA1,
    ST_DATA2,
} state;

static u16 mixer_runtime;
static u16 usbhid_period_ms;
static u16 usbhid_cb()
{
    u16 protoopts_period = period_index_to_ms(Model.proto_opts[PROTO_OPTS_PERIOD]);
    if (usbhid_period_ms != protoopts_period) {
        usbhid_period_ms = protoopts_period;
        // HID should be restarted when period changes
        // this lets us update the endpoint descriptor's bInterval field
        HID_Disable();
        HID_SetInterval(usbhid_period_ms);
        HID_Enable();
        return 0;
    }
    switch (state) {
        case ST_DATA1:
            CLOCK_RunMixer();  // clears mixer_sync, which is then set when mixer update complete
            state = ST_DATA2;
            return mixer_runtime;

        case ST_DATA2:
            if (mixer_sync != MIX_DONE && mixer_runtime < 2000) {
                mixer_runtime += 50;
                return 50;  // wait for mixer instead of forcing a write
            }
            build_data_pkt();
            HID_Write(packet, sizeof(packet));
            state = ST_DATA1;
            return 0;  // stop the clock and let USB interrupts trigger us instead
    }
    return 0;  // avoid compiler warning
}

static void deinit()
{
    HID_Disable();
}

static void initialize()
{
    CLOCK_StopTimer();
    state = ST_DATA1;
    mixer_runtime = 50;
    num_channels = Model.num_channels;
    usbhid_period_ms = period_index_to_ms(Model.proto_opts[PROTO_OPTS_PERIOD]);
    HID_SetInterval(usbhid_period_ms);
    HID_SetCallback(usbhid_cb);
    HID_Enable();
}

uintptr_t USBHID_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT: deinit(); return 0;
        case PROTOCMD_CHECK_AUTOBIND: return 1;
        case PROTOCMD_BIND: return 0;
        case PROTOCMD_NUMCHAN: return USBHID_MAX_CHANNELS;
        case PROTOCMD_DEFAULT_NUMCHAN: return 6;
        case PROTOCMD_CHANNELMAP: return UNCHG;
        case PROTOCMD_TELEMETRYSTATE: return PROTO_TELEM_UNSUPPORTED;
        case PROTOCMD_GETOPTIONS:
            if (Model.proto_opts[PROTO_OPTS_PERIOD] > USBHID_PERIOD_MAX_INDEX)
                Model.proto_opts[PROTO_OPTS_PERIOD] = USBHID_PERIOD_MAX_INDEX;
            return (uintptr_t)usbhid_opts;
        default: break;
    }
    return 0;
}
