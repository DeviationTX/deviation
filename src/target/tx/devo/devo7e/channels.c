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

//Duplicated in tx_buttons.c
#define SWITCH_3x2  0
#define SWITCH_2x2  ((1 << INP_SWA2) | (1 << INP_SWB2))
#define SWITCH_3x1  ((1 << INP_SWB0) | (1 << INP_SWB1) | (1 << INP_SWB2))
#define SWITCH_NONE ((1 << INP_SWA0) | (1 << INP_SWA1) | (1 << INP_SWA2) \
                   | (1 << INP_SWB0) | (1 << INP_SWB1) | (1 << INP_SWB2))

extern u32 global_extra_switches;
void CHAN_Init()
{
    ADC_Init();

    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPCEN);
    /* configure switches for digital I/O */
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN,
                   GPIO10 | GPIO11);
    gpio_set(GPIOC, GPIO10 | GPIO11);
}

s32 CHAN_ReadRawInput(int channel)
{
    s32 value = 0;
    if (channel < INP_HAS_CALIBRATION) {
        return ADC_ReadRawInput(channel);
    }

    switch(channel) {
    case INP_HOLD0:    value = gpio_get(GPIOC, GPIO11); break;
    case INP_HOLD1:    value = ! gpio_get(GPIOC, GPIO11); break;
    case INP_FMOD0:    value = gpio_get(GPIOC, GPIO10); break;
    case INP_FMOD1:    value = ! gpio_get(GPIOC, GPIO10); break;
    case INP_SWA0:     value = global_extra_switches   & 0x04;  break;
    case INP_SWA1:     value = !(global_extra_switches & 0x0c); break;
    case INP_SWA2:     value = global_extra_switches   & 0x08;  break;
    case INP_SWB0:     value = global_extra_switches   & 0x01;  break;
    case INP_SWB1:     value = !(global_extra_switches & 0x03); break;
    case INP_SWB2:     value = global_extra_switches   & 0x02;  break;
    }
    return value;
}

void CHAN_SetSwitchCfg(const char *str)
{
    if(strcmp(str, "3x2") == 0) {
        Transmitter.ignore_src = SWITCH_3x2;
    } else if(strcmp(str, "2x2") == 0) {
        Transmitter.ignore_src = SWITCH_2x2;
    } else if(strcmp(str, "3x1") == 0) {
        Transmitter.ignore_src = SWITCH_3x1;
    } else {
        Transmitter.ignore_src = SWITCH_NONE;
    }
}
