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

 Most of this code is based on the mixer from er9x developed by
 Erez Raviv <erezraviv@gmail.com>
 http://code.google.com/p/er9x/
 and the th9x project
 http://code.google.com/p/th9x/
 */

#include "common.h"
#include "mixer.h"
#include "config/model.h"

#define INPNAME_AILERON  _tr("AIL")
#define INPNAME_ELEVATOR _tr("ELE")
#define INPNAME_THROTTLE _tr("THR")
#define INPNAME_RUDDER   _tr("RUD")
#define INPNAME_AUX4     _tr("AUX4")
#define INPNAME_AUX5     _tr("AUX5")
#define INPNAME_RUD_DR   _tr("RUD DR")
#define INPNAME_ELE_DR   _tr("ELE DR")
#define INPNAME_AIL_DR   _tr("AIL DR")
#define INPNAME_GEAR     _tr("GEAR")
#define INPNAME_MIX0     _tr("MIX0")
#define INPNAME_MIX1     _tr("MIX1")
#define INPNAME_MIX2     _tr("MIX2")
#define INPNAME_FMOD0    _tr("FMODE0")
#define INPNAME_FMOD1    _tr("FMODE1")
#define INPNAME_FMOD2    _tr("FMODE2")

#define BUTNAME_TRIM_LV_NEG _tr("TRIMLV-")
#define BUTNAME_TRIM_LV_POS _tr("TRIMLV+")
#define BUTNAME_TRIM_RV_NEG _tr("TRIMRV-")
#define BUTNAME_TRIM_RV_POS _tr("TRIMRV+")
#define BUTNAME_TRIM_LH_NEG _tr("TRIMLH-")
#define BUTNAME_TRIM_LH_POS _tr("TRIMLH+")
#define BUTNAME_TRIM_RH_NEG _tr("TRIMRH-")
#define BUTNAME_TRIM_RH_POS _tr("TRIMRH+")
#define BUTNAME_TRIM_L_NEG  _tr("TRIM_L-")
#define BUTNAME_TRIM_L_POS  _tr("TRIM_L+")
#define BUTNAME_TRIM_R_NEG  _tr("TRIM_R-")
#define BUTNAME_TRIM_R_POS  _tr("TRIM_R+")
#define BUTNAME_LEFT        _tr("Left")
#define BUTNAME_RIGHT       _tr("Right")
#define BUTNAME_DOWN        _tr("Down")
#define BUTNAME_UP          _tr("Up")
#define BUTNAME_ENTER       _tr("Enter")
#define BUTNAME_EXIT        _tr("Exit")

const char *tx_stick_names[4] = {
    _tr_noop("RIGHT_H"),
    _tr_noop("LEFT_V"),
    _tr_noop("RIGHT_V"),
    _tr_noop("LEFT_H"),
};

const char *INPUT_SourceName(char *str, u8 src)
{
    u8 is_neg = MIXER_SRC_IS_INV(src);
    src = MIXER_SRC(src);

    if(! src) {
        return _tr("None");
    } else if(src <= NUM_TX_INPUTS) {
        const char *ptr = "";
        #define CHANDEF(x) case INP_##x : ptr = INPNAME_##x; break;
        switch(src) {
            #include "capabilities.h"
        };
        #undef CHANDEF
        sprintf(str, "%s%s", is_neg ? "!" : "", ptr);
    } else if(src <= NUM_INPUTS + NUM_OUT_CHANNELS) {
        sprintf(str, "%s%s%d", is_neg ? "!" : "", _tr("Ch"), src - NUM_INPUTS);
    } else {
        sprintf(str, "%s%d", _tr("Virt"), src - NUM_INPUTS - NUM_OUT_CHANNELS);
    }
    return str;
}

const char *INPUT_ButtonName(u8 button)
{
    if (! button) {
        return _tr("None");
    }
    #define BUTTONDEF(x) case BUT_##x : return BUTNAME_##x;
    switch(button) {
        #include "capabilities.h"
    };
    #undef BUTTONDEF
    return "";
}
