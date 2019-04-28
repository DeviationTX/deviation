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

#define KEY_INP_SWA gui.rud_dr
#define KEY_INP_SWB gui.ele_dr
#define KEY_INP_SWC gui.ail_dr
#define KEY_INP_SWD gui.gear
#define KEY_INP_SWE gui.mix
#define KEY_INP_SWF gui.fmod
#define KEY_INP_SWG gui.hold
#define KEY_INP_SWH gui.trn
#define KEY_INP_RUD_DR    gui.rud_dr
#define KEY_INP_ELE_DR    gui.ele_dr
#define KEY_INP_AIL_DR    gui.ail_dr
#define KEY_INP_GEAR      gui.gear
#define KEY_INP_MIX       gui.mix
#define KEY_INP_FMOD      gui.fmod
#define KEY_INP_HOLD      gui.hold
#define KEY_INP_TRN       gui.trn
#define KEY_INP_DR        gui.dr

s32 ADC_ReadRawInput(int channel)
{
    s32 step = (CHAN_MAX_VALUE - CHAN_MIN_VALUE) / 10;
    switch (channel) {
        case 0:            return 0;
        case INP_THROTTLE: return CHAN_MIN_VALUE + step * gui.throttle;
        case INP_RUDDER:   return CHAN_MIN_VALUE + step * gui.rudder;
        case INP_ELEVATOR: return CHAN_MIN_VALUE + step * gui.elevator;
        case INP_AILERON:  return CHAN_MIN_VALUE + step * gui.aileron;

        case 5:            return CHAN_MIN_VALUE + step * gui.aux2;
        case 6:            return CHAN_MIN_VALUE + step * gui.aux3;
        case 7:            return CHAN_MIN_VALUE + step * gui.aux4;
        case 8:            return CHAN_MIN_VALUE + step * gui.aux5;
        case 9:            return CHAN_MIN_VALUE + step * gui.aux6;
        case 10:            return CHAN_MIN_VALUE + step * gui.aux7;
    }
    return 0;
}

s32 ADC_NormalizeChannel(int channel)
{
    return ADC_ReadRawInput(channel);
}

s32 SWITCH_ReadRawInput(int channel)
{
    #define TWO_WAY(inp, p1, dir) \
        case inp ## 0:  return (KEY_ ## inp % 2)  == 0 ? 1 : 0; \
        case inp ## 1:  return (KEY_ ## inp % 2)  == 1 ? 1 : 0;
    #define THREE_WAY(inp, p1, p2, dir) \
        case inp ## 0:  return (KEY_ ## inp % 3)  == 0 ? 1 : 0; \
        case inp ## 1:  return (KEY_ ## inp % 3)  == 1 ? 1 : 0; \
        case inp ## 2:  return (KEY_ ## inp % 3)  == 2 ? 1 : 0;
    switch (channel) {
        SWITCHES
    }
    return 0;
}

void CHAN_SetSwitchCfg(const char *str)
{
    (void)str;
}
