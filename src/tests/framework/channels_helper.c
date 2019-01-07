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
#include "fltk.h"
#include "mixer.h"

#define SET_INP_THROTTLE() gui.throttle = (value - CHAN_MIN_VALUE) / step
#define SET_INP_RUDDER()   gui.rudder = (value - CHAN_MIN_VALUE) / step
#define SET_INP_ELEVATOR() gui.elevator = (value - CHAN_MIN_VALUE) / step
#define SET_INP_AILERON()  gui.aileron = (value - CHAN_MIN_VALUE) / step

#define SET_INP_AUX2()  gui.aux2 = 1
#define SET_INP_AUX3()  gui.aux3 = 1
#define SET_INP_AUX4()  gui.aux4 = 1
#define SET_INP_AUX5()  gui.aux5 = 1
#define SET_INP_AUX6()  gui.aux6 = 1
#define SET_INP_AUX7()  gui.aux7 = 1

#define SET_INP_RUD_DR0()  gui.rud_dr = 0
#define SET_INP_RUD_DR1()  gui.rud_dr = 1

#define SET_INP_ELE_DR0()  gui.ele_dr = 0
#define SET_INP_ELE_DR1()  gui.ele_dr = 1

#define SET_INP_AIL_DR0()  gui.ail_dr = 0
#define SET_INP_AIL_DR1()  gui.ail_dr = 1

#define SET_INP_GEAR0()  gui.gear = 0
#define SET_INP_GEAR1()  gui.gear = 1

#define SET_INP_MIX0()  gui.mix = 0
#define SET_INP_MIX1()  gui.mix = 1
#define SET_INP_MIX2()  gui.mix = 2

#define SET_INP_FMOD0()  gui.fmod = 0
#define SET_INP_FMOD1()  gui.fmod = 1
#define SET_INP_FMOD2()  gui.fmod = 2

#define SET_INP_HOLD0()  gui.hold = 0
#define SET_INP_HOLD1()  gui.hold = 1

#define SET_INP_SWA0()
#define SET_INP_SWA1()
#define SET_INP_SWA2()

#define SET_INP_SWB0()
#define SET_INP_SWB1()
#define SET_INP_SWB2()

#define SET_INP_SWC0()
#define SET_INP_SWC1()
#define SET_INP_SWC2()

#define SET_INP_SWD0()
#define SET_INP_SWD1()
#define SET_INP_SWD2()

#define SET_INP_SWE0()
#define SET_INP_SWE1()
#define SET_INP_SWE2()

#define SET_INP_SWF0()
#define SET_INP_SWF1()
#define SET_INP_SWF2()

#define SET_INP_SWG0()
#define SET_INP_SWG1()
#define SET_INP_SWG2()

#define SET_INP_SWH0()
#define SET_INP_SWH1()
#define SET_INP_SWH2()


void TEST_CHAN_SetChannelValue(int channel, s32 value)
{
    s32 step = (CHAN_MAX_VALUE - CHAN_MIN_VALUE) / 100;
#define CHANDEF(x) case INP_##x : SET_INP_##x(); break;
    switch(channel) {
        #include "capabilities.h"
    }
#undef CHANDEF
    return;
}
