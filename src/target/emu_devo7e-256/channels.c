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
#include "config/tx.h"

#define IGNORE_MASK ((1 << INP_AILERON) | (1 << INP_ELEVATOR) | (1 << INP_THROTTLE) | (1 << INP_RUDDER) | (1 << INP_NONE) | (1 << INP_LAST))
#define SWITCH_3x4  ((1 << INP_SWA0) | (1 << INP_SWA1) | (1 << INP_SWA2) \
                   | (1 << INP_SWB0) | (1 << INP_SWB1) | (1 << INP_SWB2) \
                   | (1 << INP_SWC0) | (1 << INP_SWC1) | (1 << INP_SWC2) \
                   | (1 << INP_SWD0) | (1 << INP_SWD1) | (1 << INP_SWD2))
#define SWITCH_3x3  ((1 << INP_SWA0) | (1 << INP_SWA1) | (1 << INP_SWA2) \
                   | (1 << INP_SWB0) | (1 << INP_SWB1) | (1 << INP_SWB2) \
                   | (1 << INP_SWC0) | (1 << INP_SWC1) | (1 << INP_SWC2))
#define SWITCH_3x2  ((1 << INP_SWA0) | (1 << INP_SWA1) | (1 << INP_SWA2) \
                   | (1 << INP_SWB0) | (1 << INP_SWB1) | (1 << INP_SWB2))
#define SWITCH_3x1  ((1 << INP_SWA0) | (1 << INP_SWA1) | (1 << INP_SWA2))
#define SWITCH_2x2  ((1 << INP_SWE0) | (1 << INP_SWE1) \
                   | (1 << INP_SWF0) | (1 << INP_SWF1))
#define SWITCH_2x1  ((1 << INP_SWE0) | (1 << INP_SWE1))
#define SWITCH_STOCK ((1 << INP_HOLD0) | (1 << INP_HOLD1) \
                    | (1 << INP_FMOD0) | (1 << INP_FMOD1))

#define POT_2  ((1 << INP_AUX4) | (1 << INP_AUX5))
#define POT_1  (1 << INP_AUX4)

s32 CHAN_ReadInput(int channel)
{
    s32 step = (CHAN_MAX_VALUE - CHAN_MIN_VALUE) / 10;
    switch(channel) {
        case INP_THROTTLE: return CHAN_MIN_VALUE + step * gui.throttle;
        case INP_RUDDER:   return CHAN_MIN_VALUE + step * gui.rudder;
        case INP_ELEVATOR: return CHAN_MIN_VALUE + step * gui.elevator;
        case INP_AILERON:  return CHAN_MIN_VALUE + step * gui.aileron;

        case INP_HOLD0:
            if((~Transmitter.ignore_src & SWITCH_STOCK) != SWITCH_STOCK)
                return CHAN_MIN_VALUE;
             return (gui.rud_dr % 2) ? CHAN_MIN_VALUE : CHAN_MAX_VALUE;
        case INP_HOLD1:
            if((~Transmitter.ignore_src & SWITCH_STOCK) != SWITCH_STOCK)
                return CHAN_MIN_VALUE;
             return (gui.rud_dr % 2) ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_FMOD0:
            if((~Transmitter.ignore_src & SWITCH_STOCK) != SWITCH_STOCK)
                return CHAN_MIN_VALUE;
             return (gui.gear % 2) ? CHAN_MIN_VALUE : CHAN_MAX_VALUE;
        case INP_FMOD1:
            if((~Transmitter.ignore_src & SWITCH_STOCK) != SWITCH_STOCK)
                return CHAN_MIN_VALUE;
             return (gui.gear % 2) ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;

        case INP_SWA0:
            if((~Transmitter.ignore_src & SWITCH_3x1) != SWITCH_3x1)
                return CHAN_MIN_VALUE;
             return (gui.ele_dr % 3) == 0 ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_SWA1:
            if((~Transmitter.ignore_src & SWITCH_3x1) != SWITCH_3x1)
                return CHAN_MIN_VALUE;
             return (gui.ele_dr % 3) == 1 ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_SWA2:
            if((~Transmitter.ignore_src & SWITCH_3x1) != SWITCH_3x1)
                return CHAN_MIN_VALUE;
             return (gui.ele_dr % 3) == 2 ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_SWB0:
            if((~Transmitter.ignore_src & SWITCH_3x2) != SWITCH_3x2)
                return CHAN_MIN_VALUE;
             return (gui.ail_dr % 3) == 0 ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_SWB1:
            if((~Transmitter.ignore_src & SWITCH_3x2) != SWITCH_3x2)
                return CHAN_MIN_VALUE;
             return (gui.ail_dr % 3) == 1 ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_SWB2:
            if((~Transmitter.ignore_src & SWITCH_3x2) != SWITCH_3x2)
                return CHAN_MIN_VALUE;
             return (gui.ail_dr % 3) == 2 ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_SWC0:
            if((~Transmitter.ignore_src & SWITCH_3x3) != SWITCH_3x3)
                return CHAN_MIN_VALUE;
             return (gui.mix % 3) == 0 ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_SWC1:
            if((~Transmitter.ignore_src & SWITCH_3x3) != SWITCH_3x3)
                return CHAN_MIN_VALUE;
             return (gui.mix % 3) == 1 ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_SWC2:
            if((~Transmitter.ignore_src & SWITCH_3x3) != SWITCH_3x3)
                return CHAN_MIN_VALUE;
             return (gui.mix % 3) == 2 ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_SWD0:
            if((~Transmitter.ignore_src & SWITCH_3x4) != SWITCH_3x4)
                return CHAN_MIN_VALUE;
             return (gui.fmod % 3) == 0 ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_SWD1:
            if((~Transmitter.ignore_src & SWITCH_3x4) != SWITCH_3x4)
                return CHAN_MIN_VALUE;
             return (gui.fmod % 3) == 1 ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_SWD2:
            if((~Transmitter.ignore_src & SWITCH_3x4) != SWITCH_3x4)
                return CHAN_MIN_VALUE;
             return (gui.fmod % 3) == 2 ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_SWE0:
            if((~Transmitter.ignore_src & SWITCH_2x1) != SWITCH_2x1)
                return CHAN_MIN_VALUE;
             return (gui.gear % 2) ? CHAN_MIN_VALUE : CHAN_MAX_VALUE;
        case INP_SWE1:
            if((~Transmitter.ignore_src & SWITCH_2x1) != SWITCH_2x1)
                return CHAN_MIN_VALUE;
             return (gui.gear % 2) ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
        case INP_SWF0:
            if((~Transmitter.ignore_src & SWITCH_2x2) != SWITCH_2x2)
                return CHAN_MIN_VALUE;
             return (gui.rud_dr % 2) ? CHAN_MIN_VALUE : CHAN_MAX_VALUE;
        case INP_SWF1:
            if((~Transmitter.ignore_src & SWITCH_2x2) != SWITCH_2x2)
                return CHAN_MIN_VALUE;
             return (gui.rud_dr % 2) ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;

        case INP_AUX4:
            if((~Transmitter.ignore_src & POT_1) != POT_1)
                return CHAN_MIN_VALUE;
             return CHAN_MIN_VALUE + step * gui.aux4;
        case INP_AUX5:
            if((~Transmitter.ignore_src & POT_2) != POT_2)
                return CHAN_MIN_VALUE;
             return CHAN_MIN_VALUE + step * gui.aux5;
    }
    return 0;
}
s32 CHAN_ReadRawInput(int channel)
{
    return CHAN_ReadInput(channel);
}

void CHAN_SetSwitchCfg(const char *str)
{
    if(strcmp(str, "3x4") == 0) {
        Transmitter.ignore_src &= ~SWITCH_3x4;
    } else if(strcmp(str, "3x3") == 0) {
        Transmitter.ignore_src &= ~SWITCH_3x3;
    } else if(strcmp(str, "3x2") == 0) {
        Transmitter.ignore_src &= ~SWITCH_3x2;
    } else if(strcmp(str, "3x1") == 0) {
        Transmitter.ignore_src &= ~SWITCH_3x1;
    } else if(strcmp(str, "2x2") == 0) {
        Transmitter.ignore_src &= ~SWITCH_2x2;
    } else if(strcmp(str, "2x1") == 0) {
        Transmitter.ignore_src &= ~SWITCH_2x1;
    } else if(strcmp(str, "potx2") == 0) {
        Transmitter.ignore_src &= ~POT_2;
    } else if(strcmp(str, "potx1") == 0) {
        Transmitter.ignore_src &= ~POT_1;
    } else {
        Transmitter.ignore_src = ~IGNORE_MASK & ~SWITCH_STOCK;
    }
    if ((~Transmitter.ignore_src & SWITCH_3x3) == SWITCH_3x3) {
        Transmitter.ignore_src |= SWITCH_STOCK;
    }
}
