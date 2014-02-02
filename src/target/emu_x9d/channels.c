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

        case INP_AUX2:     return CHAN_MIN_VALUE + step * gui.aux2;
        case INP_AUX3:     return CHAN_MIN_VALUE + step * gui.aux3;
        case INP_AUX4:     return CHAN_MIN_VALUE + step * gui.aux4;
        case INP_AUX5:     return CHAN_MIN_VALUE + step * gui.aux5;

        case INP_SWA0:  return (gui.rud_dr % 3) == 0 ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_SWA1:  return (gui.rud_dr % 3) == 1 ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_SWA2:  return (gui.rud_dr % 3) == 2 ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;

        case INP_SWB0:  return (gui.ele_dr % 3) == 0 ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_SWB1:  return (gui.ele_dr % 3) == 1 ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_SWB2:  return (gui.ele_dr % 3) == 2 ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;

        case INP_SWC0:  return (gui.ail_dr % 3) == 0 ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_SWC1:  return (gui.ail_dr % 3) == 1 ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_SWC2:  return (gui.ail_dr % 3) == 2 ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;

        case INP_SWD0:  return (gui.gear % 3) == 0 ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_SWD1:  return (gui.gear % 3) == 1 ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_SWD2:  return (gui.gear % 3) == 2 ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;

        case INP_SWE0:     return (gui.mix % 3) == 0  ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_SWE1:     return (gui.mix % 3) == 1  ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_SWE2:     return (gui.mix % 3) == 2  ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;

        case INP_SWF0:    return (gui.fmod % 2) == 0 ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_SWF1:    return (gui.fmod % 2) == 1 ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;

        case INP_SWG0:    return (gui.hold % 3) == 0 ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_SWG1:    return (gui.hold % 3) == 1 ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_SWG2:    return (gui.hold % 3) == 2 ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;

        case INP_SWH0:     return (gui.trn % 2)   ? CHAN_MIN_VALUE : CHAN_MAX_VALUE;
        case INP_SWH1:     return (gui.trn % 2)   ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;

    }
    return 0;
}
s32 CHAN_ReadRawInput(int channel)
{
    return CHAN_ReadInput(channel);
}
