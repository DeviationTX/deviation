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

#define PPMOUT_MAX_CHANNELS 12
static volatile u16 pulses[PPMOUT_MAX_CHANNELS+1];
u8 num_channels;

#define MIN_PPM_PW 500
#define MAX_PPM_PW 1500
#define NOTCH      400

#ifdef EMULATOR
#define PERIOD 3000
#else
#define PERIOD 50000
#endif
static void build_data_pkt()
{
    int i;
    for (i = 0; i < num_channels; i++) {
        s32 value = (s32)Channels[i] * (MAX_PPM_PW - MIN_PPM_PW) / 2 / CHAN_MAX_VALUE
                    + (MAX_PPM_PW - MIN_PPM_PW) / 2 + MIN_PPM_PW;
        pulses[i] = value;
    }
    pulses[num_channels] = 0;
}


static u16 ppmout_cb()
{
    build_data_pkt();
    PPM_Enable(NOTCH, pulses); //400us 'flat notch'
    return PERIOD;
}

static void initialize()
{
    CLOCK_StopTimer();
    PWM_Initialize();
    num_channels = Model.num_channels;
    CLOCK_StartTimer(1000, ppmout_cb);
}

u32 PPMOUT_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_CHECK_AUTOBIND: return 0;
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return PPMOUT_MAX_CHANNELS;
        default: break;
    }
    return 0;
}
