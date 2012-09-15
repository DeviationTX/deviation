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

#define PPMOUT_MAX_CHANNELS 10
static volatile u16 pulses[PPMOUT_MAX_CHANNELS+1];
u8 num_channels;

#define MIN_PPM_PW 500
#define MAX_PPM_PW 1500
#define NOTCH      400

/* FIXME:  The original imlementation used a PWM to output the PPM signal.
           However, I could not get TIM1 woring properly.
           The current implementation just bit-bangs the output.
           It works fine but is less efficient
*/
#ifdef EMULATOR
#define PERIOD 3000
#else
#define PERIOD 20000
#define BITBANG_PPM
#endif
volatile u8 state;
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

#ifdef BITBANG_PPM
static u16 ppmout_cb()
{
    static volatile u16 accum;
    u16 val;
    if (state == 0) {
        accum = 0;
        build_data_pkt();
    }
    if(state & 0x01) {
        PWM_Set(1);
        if(state == num_channels * 2 + 1) {
            state = 0;
            return PERIOD - accum;
        }
        val = pulses[state / 2];
    } else {
        PWM_Set(0);
        val = NOTCH;
    }
    state++;
    accum += val;
    return val;
}
#else
static u16 ppmout_cb()
{
    build_data_pkt();
    PPM_Enable(NOTCH, pulses); //400us 'flat notch'
    return PERIOD;
}
#endif

static void initialize()
{
    CLOCK_StopTimer();
    PWM_Initialize();
    num_channels = Model.num_channels;
    state = 0;
    CLOCK_StartTimer(1000, ppmout_cb);
}

u32 PPMOUT_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_CHECK_AUTOBIND: return 1;
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return PPMOUT_MAX_CHANNELS;
        default: break;
    }
    return 0;
}
