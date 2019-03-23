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

void CHAN_Init()
{
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPCEN);
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN);
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPFEN);
    ADC_Init();

    /* configure switches for digital I/O */
    //gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO15);
    //gpio_set(GPIOA, GPIO15);

    //rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPBEN);
    //gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO0);
    //gpio_clear(GPIOB, GPIO0);

    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO10 | GPIO11 | GPIO12);
    gpio_set(GPIOC, GPIO10 | GPIO11 | GPIO12);

    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPDEN);
    gpio_set_mode(GPIOD, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO2 | GPIO3 | GPIO7);
    gpio_set(GPIOD, GPIO2 | GPIO3 | GPIO7);

    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPGEN);
    gpio_set_mode(GPIOG, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN,
                  GPIO6 | GPIO7 | GPIO8 | GPIO11 | GPIO13 | GPIO14 | GPIO15);
    gpio_set(GPIOG, GPIO6 | GPIO7 | GPIO8 | GPIO11 | GPIO13 | GPIO14 | GPIO15);
}

s32 CHAN_ReadRawInput(int channel)
{
    s32 value = 0;
    if (channel < INP_HAS_CALIBRATION) {
        return ADC_ReadRawInput(channel);
    }

    switch (channel) {
    case INP_MIX0:     value = ! gpio_get(GPIOC, GPIO10); break;
    case INP_MIX1:     value = (gpio_get(GPIOC, GPIO10) && gpio_get(GPIOC, GPIO12)); break;
    case INP_MIX2:     value = ! gpio_get(GPIOC, GPIO12); break;

    case INP_AIL_DR0:   value = ! gpio_get(GPIOD, GPIO3); break;
    case INP_AIL_DR1:   value = (gpio_get(GPIOD, GPIO3) && gpio_get(GPIOD, GPIO2)); break;
    case INP_AIL_DR2:   value = ! gpio_get(GPIOD, GPIO2); break;

    case INP_RUD_DR0:   value = ! gpio_get(GPIOG, GPIO13); break;
    case INP_RUD_DR1:   value = (gpio_get(GPIOG, GPIO13) && gpio_get(GPIOG, GPIO11)); break;
    case INP_RUD_DR2:   value = ! gpio_get(GPIOG, GPIO11); break;

    case INP_ELE_DR0:   value = ! gpio_get(GPIOG, GPIO15); break;
    case INP_ELE_DR1:   value = (gpio_get(GPIOG, GPIO15) && gpio_get(GPIOD, GPIO7)); break;
    case INP_ELE_DR2:   value = ! gpio_get(GPIOD, GPIO7); break;

    case INP_FMOD0:    value = ! gpio_get(GPIOG, GPIO6); break;
    case INP_FMOD1:    value = (gpio_get(GPIOG, GPIO6) && gpio_get(GPIOG, GPIO7)); break;
    case INP_FMOD2:    value = ! gpio_get(GPIOG, GPIO7); break;

    case INP_GEAR0:    value = gpio_get(GPIOC, GPIO11); break;
    case INP_GEAR1:    value = ! gpio_get(GPIOC, GPIO11); break;

    case INP_TRN0:     value = gpio_get(GPIOG, GPIO8); break;
    case INP_TRN1:     value = ! gpio_get(GPIOG, GPIO8); break;

    case INP_HOLD0:    value = gpio_get(GPIOG, GPIO14); break;
    case INP_HOLD1:    value = ! gpio_get(GPIOG, GPIO14); break;
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
    if (channel == INP_THROTTLE || channel == INP_RUDDER || channel == INP_AUX2 || channel == INP_AUX5)
        value = -value;
    return value;
}
