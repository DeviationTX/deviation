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

#ifdef MODULAR
  #pragma long_calls_off
  extern unsigned _data_loadaddr;
  const unsigned long protocol_type = (unsigned long)&_data_loadaddr;
#endif
#define PPMOUT_MAX_CHANNELS NUM_OUT_CHANNELS
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
#define STEP_SIZE "3276810"  // == 10small/50large == (50 << 16) | 10
#define STEPSIZE2 "32768100" // == 100small / 500large == (500 << 16) | 100
static const char * const ppm_opts[] = {
  _tr_noop("Center PW"),  "1000",  "1800",  STEP_SIZE, NULL,
  _tr_noop("Delta PW"),   "100",   "700",   STEP_SIZE, NULL,
  _tr_noop("Notch PW"),   "100",   "500",   STEP_SIZE, NULL,
  _tr_noop("Frame Size"), "10000", "30000", STEPSIZE2, NULL,
  NULL
};
enum {
    CENTER_PW,
    DELTA_PW,
    NOTCH_PW,
    PERIOD_PW,
    LAST_PROTO_OPT,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

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
            return Model.proto_opts[PERIOD_PW] - accum;
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
    if (PPMin_Mode())
        return;
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
        case PROTOCMD_NUMCHAN:
            if (Model.proto_opts[CENTER_PW] != 0) {
                uint32_t chan = (Model.proto_opts[PERIOD_PW] - Model.proto_opts[NOTCH_PW])
                              / (Model.proto_opts[CENTER_PW] + Model.proto_opts[DELTA_PW] + Model.proto_opts[NOTCH_PW]);
                if (chan > NUM_OUT_CHANNELS)
                    return (void *)(long)NUM_OUT_CHANNELS;
                return (void *)(long)chan;
            }
            return (void *)10L;
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)6L;
        case PROTOCMD_GETOPTIONS:
            if (Model.proto_opts[CENTER_PW] == 0) {
                Model.proto_opts[CENTER_PW] = 1100;
                Model.proto_opts[DELTA_PW] = 400;
                Model.proto_opts[NOTCH_PW] = 400;
                Model.proto_opts[PERIOD_PW] = 22500;
            }
            return ppm_opts;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        default: break;
    }
    return 0;
}
