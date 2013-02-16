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
#include <libopencm3/stm32/f1/rcc.h>
#include <libopencm3/stm32/f1/gpio.h>
#include "common.h"
#include "mixer.h"
#include "config/tx.h"
#include "../common_devo/devo.h"

const u8 adc_chan_sel[NUM_ADC_CHANNELS] = {13, 12, 11, 15, 10, 4, 16, 14};

void CHAN_Init()
{
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPCEN);
    ADC_Init();

    /* configure channels for analog */
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO0);
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO1);
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO2);
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO3);
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO5);
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO4);
    /* Enable Voltage measurement */
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO4);

    /* configure switches for digital I/O */
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN,
                   GPIO6 | GPIO7 | GPIO8 | GPIO9
                   | GPIO10 | GPIO11 | GPIO12 | GPIO13);
    gpio_set(GPIOC,
                   GPIO6 | GPIO7 | GPIO8 | GPIO9
                   | GPIO10 | GPIO11 | GPIO12 | GPIO13);
}

s32 CHAN_ReadRawInput(int channel)
{
    s32 value = 0;
    switch(channel) {
    case INP_THROTTLE: value = adc_array_raw[0]; break;  // bug fix: right vertical
    case INP_AILERON:   value = adc_array_raw[1]; break;  // bug fix: right horizon
    case INP_RUDDER: value = adc_array_raw[2]; break;  // bug fix: left horizon
    case INP_ELEVATOR:  value = adc_array_raw[3]; break;  // bug fix: left vertical
    case INP_AUX4:     value = adc_array_raw[4]; break;
    case INP_AUX5:     value = adc_array_raw[5]; break;
    case INP_RUD_DR0:  value = gpio_get(GPIOC, GPIO8); break;
    case INP_RUD_DR1:  value = ! gpio_get(GPIOC, GPIO8); break;
    case INP_ELE_DR0:  value = gpio_get(GPIOC, GPIO7); break;
    case INP_ELE_DR1:  value = ! gpio_get(GPIOC, GPIO7); break;
    case INP_AIL_DR0:  value = gpio_get(GPIOC, GPIO11); break;
    case INP_AIL_DR1:  value = ! gpio_get(GPIOC, GPIO11); break;
    case INP_GEAR0:    value = gpio_get(GPIOC, GPIO6); break;
    case INP_GEAR1:    value = ! gpio_get(GPIOC, GPIO6); break;
    case INP_MIX0:     value = ! gpio_get(GPIOC, GPIO13); break;
    case INP_MIX1:     value = (gpio_get(GPIOC, GPIO12) && gpio_get(GPIOC, GPIO13)); break;
    case INP_MIX2:     value = ! gpio_get(GPIOC, GPIO12); break;
    case INP_FMOD0:    value = ! gpio_get(GPIOC, GPIO10); break;
    case INP_FMOD1:    value = (gpio_get(GPIOC, GPIO9) && gpio_get(GPIOC, GPIO10)); break;
    case INP_FMOD2:    value = ! gpio_get(GPIOC, GPIO9); break;
    }
    return value;
}
s16 CHAN_ReadInput(int channel)
{
    s32 value = CHAN_ReadRawInput(channel);
    if(channel <= INP_HAS_CALIBRATION) {
        s32 max = Transmitter.calibration[channel - 1].max;
        s32 min = Transmitter.calibration[channel - 1].min;
        s32 zero = Transmitter.calibration[channel - 1].zero;
        if(! zero) {
            //If this input doesn't have a zero, calculate from max/min
            zero = ((u32)max + min) / 2;
        }
        // Derate min and max by 1% to ensure we can get all the way to 100%
        max = (max - zero) * 99 / 100;
        min = (min - zero) * 99 / 100;
        if(value >= zero) {
            value = (value - zero) * CHAN_MAX_VALUE / max;
        } else {
            value = (value - zero) * CHAN_MIN_VALUE / min;
        }
        //Bound output
        if (value > CHAN_MAX_VALUE)
            value = CHAN_MAX_VALUE;
        if (value < CHAN_MIN_VALUE)
            value = CHAN_MIN_VALUE;
    } else {
        value = value ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
    }
    if (channel == INP_THROTTLE || channel == INP_RUDDER)
        value = -value;
    return value;
}
