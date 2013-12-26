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
#include "../common/emu/fltk.h"
#include "mixer.h"

s16 CHAN_ReadInput(int channel)
{
    s32 step = (CHAN_MAX_VALUE - CHAN_MIN_VALUE) / 10;
    switch(channel) {
        case INP_THROTTLE: return CHAN_MIN_VALUE + step * gui.throttle;
        case INP_RUDDER:   return CHAN_MIN_VALUE + step * gui.rudder;
        case INP_ELEVATOR: return CHAN_MIN_VALUE + step * gui.elevator;
        case INP_AILERON:  return CHAN_MIN_VALUE + step * gui.aileron;
        case INP_HOLD0:    return (gui.rud_dr & 0x01) ? CHAN_MIN_VALUE : CHAN_MAX_VALUE;
        case INP_HOLD1:    return (gui.rud_dr & 0x01) ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_FMOD0:    return gui.gear ? CHAN_MIN_VALUE : CHAN_MAX_VALUE;
        case INP_FMOD1:    return gui.gear ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
    }
    return 0;
}
s32 CHAN_ReadRawInput(int channel)
{
    return CHAN_ReadInput(channel);
}
