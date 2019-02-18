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
#include "target/drivers/mcu/emu/fltk.h"
#include "mixer.h"
#include "config/tx.h"

#define SWITCH_3x2  0
#define SWITCH_2x2  ((1 << INP_SWA2) | (1 << INP_SWB2))
#define SWITCH_3x1  ((1 << INP_SWB0) | (1 << INP_SWB1) | (1 << INP_SWB2))
#define SWITCH_NONE ((1 << INP_SWA0) | (1 << INP_SWA1) | (1 << INP_SWA2) \
                   | (1 << INP_SWB0) | (1 << INP_SWB1) | (1 << INP_SWB2))

s32 CHAN_ReadInput(int channel)
{
    s32 step = (CHAN_MAX_VALUE - CHAN_MIN_VALUE) / 10;
    switch(channel) {
        case INP_THROTTLE: return CHAN_MIN_VALUE + step * gui.throttle;
        case INP_RUDDER:   return CHAN_MIN_VALUE + step * gui.rudder;
        case INP_ELEVATOR: return CHAN_MIN_VALUE + step * gui.elevator;
        case INP_AILERON:  return CHAN_MIN_VALUE + step * gui.aileron;
        case INP_HOLD0:    return (gui.rud_dr & 0x01) ? CHAN_MIN_VALUE : CHAN_MAX_VALUE;
        case INP_HOLD1:    return (gui.rud_dr & 0x01) ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_FMOD0:    return (gui.fmod % 3) == 0 ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_FMOD1:    return (gui.fmod % 3) == 1 ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_FMOD2:    return (gui.fmod % 3) == 2 ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_MIX0:     return (gui.mix % 3) == 0  ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_MIX1:     return (gui.mix % 3) == 1  ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_MIX2:     return (gui.mix % 3) == 2  ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_GEAR0:    return (gui.gear % 2)   ? CHAN_MIN_VALUE : CHAN_MAX_VALUE;
        case INP_GEAR1:    return (gui.gear % 2)   ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_AIL_DR0:  return (gui.ail_dr & 0x01) ? CHAN_MIN_VALUE : CHAN_MAX_VALUE;
        case INP_AIL_DR1:  return (gui.ail_dr & 0x01) ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
    }
    return 0;
}
s32 CHAN_ReadRawInput(int channel)
{
    return CHAN_ReadInput(channel);
}
void CHAN_SetSwitchCfg(const char *str)
{
    if(strcmp(str, "3x2") == 0) {
        Transmitter.ignore_src = SWITCH_3x2;
    } else if(strcmp(str, "2x2") == 0) {
        Transmitter.ignore_src = SWITCH_2x2;
    } else if(strcmp(str, "3x1") == 0) {
        Transmitter.ignore_src = SWITCH_3x1;
    } else {
        Transmitter.ignore_src = SWITCH_NONE;
    }
}
