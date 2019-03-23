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
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include "common.h"
#include "mixer.h"
#include "config/tx.h"
#include "../common/devo.h"

#define SWITCH_2x2  (1 << INP_SWA2)
#define SWITCH_3x1  ((1 << INP_SWB0) | (1 << INP_SWB1))
#define SWITCH_NONE ((1 << INP_SWA0) | (1 << INP_SWA1) | (1 << INP_SWA2) \
                   | (1 << INP_SWB0) | (1 << INP_SWB1))


void CHAN_Init()
{
    ADC_Init();


    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPCEN);
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
    if (channel < INP_HAS_CALIBRATION) {
        return ADC_ReadRawInput(channel);
    }

    switch(channel) {
    case INP_DR0:      value = gpio_get(GPIOC, GPIO11); break;
    case INP_DR1:      value = ! gpio_get(GPIOC, GPIO11); break;
    case INP_FMOD0:    value = ! gpio_get(GPIOC, GPIO13); break;
    case INP_FMOD1:    value = (gpio_get(GPIOC, GPIO12) && gpio_get(GPIOC, GPIO13)); break;
    case INP_FMOD2:    value = ! gpio_get(GPIOC, GPIO12); break;
    case INP_MIX0:     value = ! gpio_get(GPIOC, GPIO8); break;
    case INP_MIX1:     value = (gpio_get(GPIOC, GPIO6) && gpio_get(GPIOC, GPIO8)); break;
    case INP_MIX2:     value = ! gpio_get(GPIOC, GPIO6); break;
    case INP_GEAR0:    value = gpio_get(GPIOC, GPIO7); break;
    case INP_GEAR1:    value = ! gpio_get(GPIOC, GPIO7); break;
    case INP_SWA0:     value = gpio_get(GPIOC, GPIO9); break;
    case INP_SWA1:     value = Transmitter.ignore_src == SWITCH_2x2
                             ? ! gpio_get(GPIOC, GPIO9)
                             : (! gpio_get(GPIOC, GPIO9)) && (! gpio_get(GPIOC, GPIO10));
                             break;
    case INP_SWA2: case INP_SWB0:
                       value = gpio_get(GPIOC, GPIO10); break;
    case INP_SWB1:     value = ! gpio_get(GPIOC, GPIO10); break;
    }
    return value;
}
s32 CHAN_ReadInput(int channel)
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

void CHAN_SetSwitchCfg(const char *str)
{
    if(strcmp(str, "2x2") == 0) {
        Transmitter.ignore_src = SWITCH_2x2;
    } else if(strcmp(str, "3x1") == 0) {
        Transmitter.ignore_src = SWITCH_3x1;
    } else {
        Transmitter.ignore_src = SWITCH_NONE;
    }
}
