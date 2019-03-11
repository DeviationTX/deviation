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
#include "target/tx/devo/common/devo.h"

void CHAN_Init()
{
    ADC_Init();

    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN);
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPCEN);
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
    s32 value;
    if (channel < INP_HAS_CALIBRATION) {
        return ADC_ReadRawInput(channel);
    }

    switch (channel) {
    case INP_SWA0:     value = gpio_get(GPIOC, GPIO0); break;
    case INP_SWA1:     value = !gpio_get(GPIOC, GPIO0); break;
    case INP_SWB0:     value = gpio_get(GPIOC, GPIO1); break;
    case INP_SWB1:     value = !gpio_get(GPIOC, GPIO1); break;
    case INP_SWC0:     value = !gpio_get(GPIOC, GPIO2); break;
    case INP_SWC1:     value = (gpio_get(GPIOC, GPIO2) && gpio_get(GPIOC, GPIO3)); break;
    case INP_SWC2:     value = !gpio_get(GPIOC, GPIO3); break;
    case INP_SWD0:     value = gpio_get(GPIOC, GPIO6); break;
    case INP_SWD1:     value = !gpio_get(GPIOC, GPIO6); break;
    case INP_SWE0:     value = !gpio_get(GPIOC, GPIO4); break;
    case INP_SWE1:     value = (gpio_get(GPIOC, GPIO4) && gpio_get(GPIOC, GPIO5)); break;
    case INP_SWE2:     value = !gpio_get(GPIOC, GPIO5); break;
    case INP_SWF0:     value = gpio_get(GPIOC, GPIO7); break;
    case INP_SWF1:     value = !gpio_get(GPIOC, GPIO7); break;
    case INP_SWG0:     value = !gpio_get(GPIOC, GPIO8); break;
    case INP_SWG1:     value = (gpio_get(GPIOC, GPIO8) && gpio_get(GPIOC, GPIO9)); break;
    case INP_SWG2:     value = !gpio_get(GPIOC, GPIO9); break;
    case INP_SWH0:     value = gpio_get(GPIOA, GPIO8); break;
    case INP_SWH1:     value = !gpio_get(GPIOA, GPIO8); break;
    default:           value = 0;
    }
    return value;
}
