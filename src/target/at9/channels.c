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

//Order is MODE1: AIL, ELE, THR, RUD, LeftDial, Right Dial, Left Shoulder, Right Shoulder, Vdd, Voltage
//                PA3  PA1, PA2, PA0, PA5,      PA6,        PB0            PA4,                 PB1
const u8 adc_chan_sel[NUM_ADC_CHANNELS] =  
                 {3,   1,   2,   0,   5,        6,          8,             4,              16,  9};

void CHAN_Init()
{
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN);
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPBEN);
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPCEN);
    ADC_Init();

    /* configure channels for analog */
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO0);
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO1);
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO2);
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO3);
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO4);
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO5);
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO6);
    /* Enable Voltage measurement */
    gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO0);
    gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO1);

    /* configure switches for digital I/O */
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN,
                   GPIO0 | GPIO1 | GPIO2 | GPIO3 | GPIO4 |
                   GPIO5 | GPIO6 | GPIO7 | GPIO8 | GPIO9);
    gpio_set(GPIOC,
                   GPIO0 | GPIO1 | GPIO2 | GPIO3 | GPIO4 |
                   GPIO5 | GPIO6 | GPIO7 | GPIO8 | GPIO9);
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO8);
    gpio_set(GPIOA, GPIO8);
}

s32 CHAN_ReadRawInput(int channel)
{
    s32 value = 0;
    switch(channel) {
    case INP_AILERON:  value = adc_array_raw[0]; break;  // bug fix: right vertical
    case INP_ELEVATOR: value = adc_array_raw[1]; break;  // bug fix: right horizon
    case INP_THROTTLE: value = adc_array_raw[2]; break;  // bug fix: left horizon
    case INP_RUDDER:   value = adc_array_raw[3]; break;  // bug fix: left vertical
    case INP_AUX4:     value = adc_array_raw[4]; break;
    case INP_AUX5:     value = adc_array_raw[5]; break;
    case INP_AUX6:     value = adc_array_raw[6]; break;
    case INP_AUX7:     value = adc_array_raw[7]; break;
    case INP_SWA0:     value = gpio_get(GPIOC, GPIO0); break;
    case INP_SWA1:     value = ! gpio_get(GPIOC, GPIO0); break;
    case INP_SWB0:     value = gpio_get(GPIOC, GPIO1); break;
    case INP_SWB1:     value = ! gpio_get(GPIOC, GPIO1); break;
    case INP_SWC0:     value = ! gpio_get(GPIOC, GPIO2); break;
    case INP_SWC1:     value = (gpio_get(GPIOC, GPIO2) && gpio_get(GPIOC, GPIO3)); break;
    case INP_SWC2:     value = ! gpio_get(GPIOC, GPIO3); break;
    case INP_SWD0:     value = gpio_get(GPIOC, GPIO6); break;
    case INP_SWD1:     value = ! gpio_get(GPIOC, GPIO6); break;
    case INP_SWE0:     value = ! gpio_get(GPIOC, GPIO4); break;
    case INP_SWE1:     value = (gpio_get(GPIOC, GPIO4) && gpio_get(GPIOC, GPIO5)); break;
    case INP_SWE2:     value = ! gpio_get(GPIOC, GPIO5); break;
    case INP_SWF0:     value = gpio_get(GPIOC, GPIO7); break;
    case INP_SWF1:     value = ! gpio_get(GPIOC, GPIO7); break;
    case INP_SWG0:     value = ! gpio_get(GPIOC, GPIO8); break;
    case INP_SWG1:     value = (gpio_get(GPIOC, GPIO8) && gpio_get(GPIOC, GPIO9)); break;
    case INP_SWG2:     value = ! gpio_get(GPIOC, GPIO9); break;
    case INP_SWH0:     value = gpio_get(GPIOA, GPIO8); break;
    case INP_SWH1:     value = ! gpio_get(GPIOA, GPIO8); break;
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
