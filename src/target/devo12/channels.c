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
#include "../common/devo/devo.h"

const u8 adc_chan_sel[NUM_ADC_CHANNELS] = {2, 11, 10, 8, 5, 12, 6, 13, 4, 7, 16, 3};

void CHAN_Init()
{
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPCEN);
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN);
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPFEN);
    ADC_Init();

    /* Enable Voltage measurement */
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO3); //ADC123_3

    /* configure channels for analog */
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO2); //ADC123_2

    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO0); //ADC123_10
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO1); //ADC123_11
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO2); //ADC123_12
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO3); //ADC123_13

    gpio_set_mode(GPIOF, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO6); //ADC3_4
    gpio_set_mode(GPIOF, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO7); //ADC3_5
    gpio_set_mode(GPIOF, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO8); //ADC3_6
    gpio_set_mode(GPIOF, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO9); //ADC3_7
    gpio_set_mode(GPIOF, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO10); //ADC3_8

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
    switch(channel) {
    case INP_THROTTLE: value = adc_array_raw[0]; break;
    case INP_RUDDER:   value = adc_array_raw[1]; break;
    case INP_ELEVATOR: value = adc_array_raw[2]; break;
    case INP_AILERON:  value = adc_array_raw[3]; break;
    case INP_AUX6:     value = adc_array_raw[4]; break;
    case INP_AUX7:     value = adc_array_raw[5]; break;
    case INP_AUX4:     value = adc_array_raw[6]; break;
    case INP_AUX5:     value = adc_array_raw[7]; break;
    case INP_AUX2:     value = adc_array_raw[8]; break;
    case INP_AUX3:     value = adc_array_raw[9]; break;

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
    if (channel == INP_THROTTLE || channel == INP_RUDDER || channel == INP_AUX2 || channel == INP_AUX5)
        value = -value;
    return value;
}
