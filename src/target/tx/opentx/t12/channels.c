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


extern u32 global_extra_switches;
void CHAN_Init()
{
    ADC_Init();

    rcc_periph_clock_enable(RCC_GPIOE);  // FIXME: Move to PORT_Init()
    /* configure switches for digital I/O */
    gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP,
                   GPIO5);
    gpio_mode_setup(GPIOD, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP,
                   GPIO11 | GPIO14);
    gpio_mode_setup(GPIOE, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP,
                   GPIO0 | GPIO1 | GPIO2 | GPIO7 | GPIO13 | GPIO14 | GPIO15);
}

s32 CHAN_ReadRawInput(int channel)
{
    s32 value = 0;
    switch (channel) {
        case INP_THROTTLE: value = adc_array_raw[0]; break;  // bug fix: right vertical
        case INP_AILERON:   value = adc_array_raw[1]; break;  // bug fix: right horizon
        case INP_RUDDER: value = adc_array_raw[3]; break;  // bug fix: left horizon
        case INP_ELEVATOR:  value = adc_array_raw[2]; break;  // bug fix: left vertical
        case INP_AUX4:     value = adc_array_raw[4]; break;
        case INP_AUX5:     value = adc_array_raw[5]; break;
        // SWA
        case INP_SWA0:     value = !gpio_get(GPIOE, GPIO7); break;
        case INP_SWA1:     value = (gpio_get(GPIOE, GPIO7) && gpio_get(GPIOE, GPIO13)); break;
        case INP_SWA2:     value = !gpio_get(GPIOE, GPIO13); break;
        // SWB
        case INP_SWB0:     value = !gpio_get(GPIOE, GPIO15); break;
        case INP_SWB1:     value = (gpio_get(GPIOE, GPIO15) && gpio_get(GPIOA, GPIO5)); break;
        case INP_SWB2:     value = !gpio_get(GPIOA, GPIO5); break;
        // SWC
        case INP_SWC0:     value = !gpio_get(GPIOD, GPIO11); break;
        case INP_SWC1:     value = (gpio_get(GPIOD, GPIO11) && gpio_get(GPIOE, GPIO0)); break;
        case INP_SWC2:     value = !gpio_get(GPIOE, GPIO0); break;
        // SWD
        case INP_SWD0:     value = !gpio_get(GPIOE, GPIO1); break;
        case INP_SWD1:     value = (gpio_get(GPIOE, GPIO1) && gpio_get(GPIOE, GPIO2)); break;
        case INP_SWD2:     value = !gpio_get(GPIOE, GPIO2); break;
        // SWF
        case INP_SWF0:     value = !!gpio_get(GPIOE, GPIO14); break;
        case INP_SWF1:     value = !gpio_get(GPIOE, GPIO14); break;
        // SWH
        case INP_SWH0:     value = !!gpio_get(GPIOD, GPIO14); break;
        case INP_SWH1:     value = !gpio_get(GPIOD, GPIO14); break;
    }
    return value;
}

s32 CHAN_ReadInput(int channel)
{
    s32 value = CHAN_ReadRawInput(channel);
    if (channel <= INP_HAS_CALIBRATION) {
        s32 max = Transmitter.calibration[channel - 1].max;
        s32 min = Transmitter.calibration[channel - 1].min;
        s32 zero = Transmitter.calibration[channel - 1].zero;
        if (!zero) {
            // If this input doesn't have a zero, calculate from max/min
            zero = ((u32)max + min) / 2;
        }
        // Derate min and max by 1% to ensure we can get all the way to 100%
        max = (max - zero) * 99 / 100;
        min = (min - zero) * 99 / 100;
        if (value >= zero) {
            value = (value - zero) * CHAN_MAX_VALUE / max;
        } else {
            value = (value - zero) * CHAN_MIN_VALUE / min;
        }
        // Bound output
        if (value > CHAN_MAX_VALUE)
            value = CHAN_MAX_VALUE;
        if (value < CHAN_MIN_VALUE)
            value = CHAN_MIN_VALUE;
    } else {
        value = value ? CHAN_MAX_VALUE : CHAN_MIN_VALUE;
    }
    if (channel == INP_THROTTLE || channel == INP_AILERON || channel == INP_AUX4)
        value = -value;
    return value;
}

void CHAN_SetSwitchCfg(const char *str)
{
    // FIXME: Do something here?
    (void)str;
}
