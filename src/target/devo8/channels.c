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
#include <libopencm3/stm32/f1/gpio.h>
#include "target.h"
#include "mixer.h"
#include "config/tx.h"
#include "devo8.h"

const char *tx_input_str[NUM_TX_INPUTS] = {
    "AIL",
    "ELE",
    "THR",
    "RUD",
    "RUD_D/R",
    "ELE_D/R",
    "AIL D/R",
    "GEAR",
    "MIX0",
    "MIX1",
    "MIX2",
    "FMODE0",
    "FMODE1",
    "FMODE2",
};

const char *tx_stick_names[4] = {
    "RIGHT_H",
    "LEFT_V",
    "RIGHT_V",
    "LEFT_H",
};

void CHAN_Init()
{
    ADC_Init();

    /* configure channels for analog */
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO0);
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO1);
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO2);
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO3);

}

s16 CHAN_ReadInput(int channel)
{
    u16 value;
    switch(channel) {
    case INP_THROTTLE: value = ADC1_Read(13); break;
    case INP_RUDDER:   value = ADC1_Read(11); break;
    case INP_ELEVATOR: value = ADC1_Read(10); break;
    case INP_AILERON:  value = ADC1_Read(12); break;
    }
    if(channel <= INP_HAS_CALIBRATION) {
        s32 max = Transmitter.calibration[channel - 1].max;
        s32 min = Transmitter.calibration[channel - 1].min;
        s32 zero = Transmitter.calibration[channel - 1].zero;
        if(! zero) {
            //If this input doesn't have a zero, calculate from max/min
            zero = ((u32)max + min) / 2;
        }
        max -= zero;
        min -= zero;
        if(value >= zero) {
            value = ((s32)value - zero) * CHAN_MAX_VALUE / max;
        } else {
            value = ((s32)value - zero) * CHAN_MIN_VALUE / min;
        }
    } else {
        value = value ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
    }
    if (channel == INP_THROTTLE)
        value = -value;
    return value;
}
