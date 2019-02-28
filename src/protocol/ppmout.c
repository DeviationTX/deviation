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
#endif

#define PPMOUT_MAX_CHANNELS NUM_OUT_CHANNELS
static volatile u16 pulses[PPMOUT_MAX_CHANNELS+1];  // +1 for "inter-packet" pulse at end

#define STEP_SIZE "3276810"  // == 10small/50large == (50 << 16) | 10
#define STEPSIZE2 "32768100" // == 100small / 500large == (500 << 16) | 100
static const char * const ppm_opts[] = {
  _tr_noop("Center PW"),  "1000",  "1800",  STEP_SIZE, NULL,
  _tr_noop("Delta PW"),   "100",   "700",   STEP_SIZE, NULL,
  _tr_noop("Notch PW"),   "100",   "500",   STEP_SIZE, NULL,
  _tr_noop("Frame Size"), "10000", "30000", STEPSIZE2, NULL,
  _tr_noop("Polarity"), _tr_noop("Normal"), _tr_noop("Inverted"), NULL,
  NULL
};
enum {
    CENTER_PW,
    DELTA_PW,
    NOTCH_PW,
    PERIOD_PW,
    POLARITY,
    LAST_PROTO_OPT,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

static void build_data_pkt()
{
    int i;
    for (i = 0; i < Model.num_channels; i++) {
        s32 value = (s32)Channels[i] * Model.proto_opts[DELTA_PW] / CHAN_MAX_VALUE
                    + Model.proto_opts[CENTER_PW] + Model.proto_opts[NOTCH_PW];
                   //FIXME fix PPM timing options, remove notch here, center to real center value (e.g. 1500)
                   //      requires model file versioning to not break existing models
        pulses[i] = value - 1;  // -1 due to timer hardware adding one count
    }
    // last width just needs to be long enough not to end before callback
    pulses[Model.num_channels] = 50000;
}

static u16 ppmout_cb()
{
    build_data_pkt();
    PPM_Enable(Model.proto_opts[NOTCH_PW], pulses, Model.num_channels+1, Model.proto_opts[POLARITY]);
#ifdef EMULATOR
    return 3000;
#else
    return Model.proto_opts[PERIOD_PW];
#endif
}

static void initialize()
{
    CLOCK_StopTimer();
    if (PPMin_Mode())
        return;

    PWM_Initialize();
    if (Model.proto_opts[CENTER_PW] == 0) {
        Model.proto_opts[CENTER_PW] = 1100;
        Model.proto_opts[DELTA_PW] = 400;
        Model.proto_opts[NOTCH_PW] = 400;
        Model.proto_opts[PERIOD_PW] = 22500;
    }
    
    CLOCK_StartTimer(1000, ppmout_cb);
}


uintptr_t PPMOUT_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT: PWM_Stop(); return 0;
        case PROTOCMD_CHECK_AUTOBIND: return 1L;
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN:
            if (Model.proto_opts[CENTER_PW] != 0) {
                uint32_t chan = (Model.proto_opts[PERIOD_PW] - Model.proto_opts[NOTCH_PW])
                              / (Model.proto_opts[CENTER_PW] + Model.proto_opts[DELTA_PW] + Model.proto_opts[NOTCH_PW]);
                if (chan > NUM_OUT_CHANNELS)
                    return NUM_OUT_CHANNELS;
                return chan;
            }
            return 10;
        case PROTOCMD_DEFAULT_NUMCHAN: return 6;
        case PROTOCMD_GETOPTIONS:
            if (Model.proto_opts[CENTER_PW] == 0) {
                Model.proto_opts[CENTER_PW] = 1100;
                Model.proto_opts[DELTA_PW] = 400;
                Model.proto_opts[NOTCH_PW] = 400;
                Model.proto_opts[PERIOD_PW] = 22500;
            }
            return (uintptr_t)ppm_opts;
        case PROTOCMD_CHANNELMAP: return UNCHG;
        case PROTOCMD_TELEMETRYSTATE: return PROTO_TELEM_UNSUPPORTED;
        default: break;
    }
    return 0;
}
