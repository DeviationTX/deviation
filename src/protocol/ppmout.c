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
  #define PPMOUT_Cmds PROTO_Cmds
  #pragma long_calls
#endif
#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"
#pragma long_calls_off

#ifdef MODULAR
  const long protocol_type = PROTOCOL_PPM;
#endif
#define PPMOUT_MAX_CHANNELS 10
static volatile u16 pulses[PPMOUT_MAX_CHANNELS+1];
u8 num_channels;

/* FIXME:  The original imlementation used a PWM to output the PPM signal.
           However, I could not get TIM1 woring properly.
           The current implementation just bit-bangs the output.
           It works fine but is less efficient
*/
#ifndef EMULATOR
#define BITBANG_PPM
#endif
static const char * const ppm_opts[] = {
  _tr_noop("Center PW"),  "1000", "1800", NULL,
  _tr_noop("Delta PW"),   "100", "700", NULL,
  _tr_noop("Notch PW"),   "100", "500", NULL,
  _tr_noop("Frame Size"),   "20000", "22500", NULL,
  NULL
};
enum {
    CENTER_PW,
    DELTA_PW,
    NOTCH_PW,
    PERIOD_PW,
};

volatile u8 state;
static void build_data_pkt()
{
    int i;
    for (i = 0; i < num_channels; i++) {
        s32 value = (s32)Channels[i] * Model.proto_opts[DELTA_PW] / CHAN_MAX_VALUE
                    + Model.proto_opts[CENTER_PW];
        pulses[i] = value;
    }
    pulses[num_channels] = 0;
}

#ifdef BITBANG_PPM
MODULE_CALLTYPE
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
            return num_channels > 9
                   ? Model.proto_opts[PERIOD_PW] + (num_channels - 9) * 2000 - accum
                   : Model.proto_opts[PERIOD_PW] - accum;
        }
        val = pulses[state / 2];
    } else {
        PWM_Set(0);
        val = Model.proto_opts[NOTCH_PW];
    }
    state++;
    accum += val;
    return val;
}
#else
MODULE_CALLTYPE
static u16 ppmout_cb()
{
    build_data_pkt();
    PPM_Enable(Model.proto_opts[NOTCH_PW], pulses); //400us 'flat notch'
#ifdef EMULATOR
    return 3000;
#else
    return Model.proto_opts[PERIOD_PW];
#endif
}
#endif

static void initialize()
{
    CLOCK_StopTimer();
    PWM_Initialize();
    num_channels = Model.num_channels;
    if (Model.proto_opts[CENTER_PW] == 0) {
        Model.proto_opts[CENTER_PW] = 1100;
        Model.proto_opts[DELTA_PW] = 400;
        Model.proto_opts[NOTCH_PW] = 400;
        Model.proto_opts[PERIOD_PW] = 22500;
    }
    
    state = 0;
    CLOCK_StartTimer(1000, ppmout_cb);
}

const void * PPMOUT_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT: PWM_Stop(); return 0;
        case PROTOCMD_CHECK_AUTOBIND: return (void *)1L;
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return (void *)((unsigned long)PPMOUT_MAX_CHANNELS);
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)6L;
        case PROTOCMD_GETOPTIONS:
            if (Model.proto_opts[CENTER_PW] == 0) {
                Model.proto_opts[CENTER_PW] = 1100;
                Model.proto_opts[DELTA_PW] = 400;
                Model.proto_opts[NOTCH_PW] = 400;
                Model.proto_opts[PERIOD_PW] = 22500;
            }
            return ppm_opts;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)-1;
        default: break;
    }
    return 0;
}
