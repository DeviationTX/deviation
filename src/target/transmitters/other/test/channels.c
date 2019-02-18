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
#include "emu.h"
#include "mixer.h"

void TEST_CHAN_SetChannelValue(int channel, s32 value)
{
    s32 step = (CHAN_MAX_VALUE - CHAN_MIN_VALUE) / 100;
    switch(channel) {
        case INP_THROTTLE: gui.throttle = (value - CHAN_MIN_VALUE) / step; break;
        case INP_RUDDER:   gui.rudder = (value - CHAN_MIN_VALUE) / step; break;
        case INP_ELEVATOR: gui.elevator = (value - CHAN_MIN_VALUE) / step; break;
        case INP_AILERON:  gui.aileron = (value - CHAN_MIN_VALUE) / step; break;

        case INP_RUD_DR0:  gui.rud_dr = 0; break;
        case INP_RUD_DR1:  gui.rud_dr = 1; break;

        case INP_ELE_DR0:  gui.ele_dr = 0; break;
        case INP_ELE_DR1:  gui.ele_dr = 1; break;

        case INP_AIL_DR0:  gui.ail_dr = 0; break;
        case INP_AIL_DR1:  gui.ail_dr = 1; break;

        case INP_GEAR0:  gui.gear = 0; break;
        case INP_GEAR1:  gui.gear = 1; break;

        case INP_MIX0:  gui.mix = 0; break;
        case INP_MIX1:  gui.mix = 1; break;
        case INP_MIX2:  gui.mix = 2; break;

        case INP_FMOD0:  gui.fmod = 0; break;
        case INP_FMOD1:  gui.fmod = 1; break;
        case INP_FMOD2:  gui.fmod = 2; break;
    }
    return;
}

s32 CHAN_ReadInput(int channel)
{
    s32 step = (CHAN_MAX_VALUE - CHAN_MIN_VALUE) / 100;
    switch(channel) {
        case INP_THROTTLE: return CHAN_MIN_VALUE + step * gui.throttle;
        case INP_RUDDER:   return CHAN_MIN_VALUE + step * gui.rudder;
        case INP_ELEVATOR: return CHAN_MIN_VALUE + step * gui.elevator;
        case INP_AILERON:  return CHAN_MIN_VALUE + step * gui.aileron;

        case INP_RUD_DR0:  return (gui.rud_dr % 2) ? CHAN_MIN_VALUE : CHAN_MAX_VALUE;
        case INP_RUD_DR1:  return (gui.rud_dr % 2) ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;

        case INP_ELE_DR0:  return (gui.ele_dr % 2) ? CHAN_MIN_VALUE : CHAN_MAX_VALUE;
        case INP_ELE_DR1:  return (gui.ele_dr % 2) ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;

        case INP_AIL_DR0:  return (gui.ail_dr % 2) ? CHAN_MIN_VALUE : CHAN_MAX_VALUE;
        case INP_AIL_DR1:  return (gui.ail_dr % 2) ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;

        case INP_GEAR0:    return (gui.gear % 2)  ? CHAN_MIN_VALUE : CHAN_MAX_VALUE;
        case INP_GEAR1:    return (gui.gear % 2)  ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;

        case INP_MIX0:     return (gui.mix % 3) == 0  ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_MIX1:     return (gui.mix % 3) == 1  ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_MIX2:     return (gui.mix % 3) == 2  ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;

        case INP_FMOD0:    return (gui.fmod % 3) == 0 ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_FMOD1:    return (gui.fmod % 3) == 1 ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_FMOD2:    return (gui.fmod % 3) == 2 ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
    }
    return 0;
}
s32 CHAN_ReadRawInput(int channel)
{
    return CHAN_ReadInput(channel);
}
