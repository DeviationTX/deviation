/*
    This project is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Foobar is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <libopencm3/stm32/f1/gpio.h>
#include "target.h"
#include "devo8.h"

void CHAN_Init()
{
    int i;
    ADC_Init();

    /* configure channels for analog */
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO0);
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO1);
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO2);
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO3);

    // This is just to fill in dummy values
    for(i = 0; i < INP_HAS_CALIBRATION; i++) {
        Transmitter.calibration[i].max = CHAN_MAX_VALUE;
        Transmitter.calibration[i].min = CHAN_MIN_VALUE;
        Transmitter.calibration[i].zero = 0;
    }
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
        u16 max = Transmitter.calibration[channel - 1].max;
        u16 min = Transmitter.calibration[channel - 1].min;
        u16 zero = Transmitter.calibration[channel - 1].zero;
        if(! zero) {
            //If this input doesn't have a zero, calculate from max/min
            zero = ((u32)max + min) / 2;
        }
        if(value >= zero) {
            value = ((s32)value - zero) * CHAN_MAX_VALUE / max;
        } else {
            value = ((s32)value - zero) * CHAN_MIN_VALUE / min;
        }
    } else {
        value = value ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
    }
    return value;
}
