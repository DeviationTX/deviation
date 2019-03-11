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

    switch (channel) {
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
