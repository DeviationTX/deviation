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
#include "mixer.h"
#include "config/tx.h"

void CHAN_Init()
{
    ADC_Init();
    SWITCH_Init();
}

s32 CHAN_ReadRawInput(int channel)
{
    if (channel <= INP_HAS_CALIBRATION) {
        return ADC_ReadRawInput(channel);
    }
    return SWITCH_ReadRawInput(channel);
}

s32 CHAN_ReadInput(int channel)
{
    if (channel <= INP_HAS_CALIBRATION) {
       return ADC_NormalizeChannel(channel);
    }
    s32 value = SWITCH_ReadRawInput(channel);
    value = value ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
    return value;
}
