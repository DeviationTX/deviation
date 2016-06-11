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

//Duplicated in tx_buttons.c
#define IGNORE_MASK ((1 << INP_AILERON) | (1 << INP_ELEVATOR) | (1 << INP_THROTTLE) | (1 << INP_RUDDER) | (1 << INP_NONE) | (1 << INP_LAST))
#define SWITCH_3x4  ((1 << INP_SWA0) | (1 << INP_SWA1) | (1 << INP_SWA2) \
                   | (1 << INP_SWB0) | (1 << INP_SWB1) | (1 << INP_SWB2) \
                   | (1 << INP_SWC0) | (1 << INP_SWC1) | (1 << INP_SWC2) \
                   | (1 << INP_SWD0) | (1 << INP_SWD1) | (1 << INP_SWD2))
#define SWITCH_3x3  ((1 << INP_SWA0) | (1 << INP_SWA1) | (1 << INP_SWA2) \
                   | (1 << INP_SWB0) | (1 << INP_SWB1) | (1 << INP_SWB2) \
                   | (1 << INP_SWC0) | (1 << INP_SWC1) | (1 << INP_SWC2))
#define SWITCH_3x2  ((1 << INP_SWA0) | (1 << INP_SWA1) | (1 << INP_SWA2) \
                   | (1 << INP_SWB0) | (1 << INP_SWB1) | (1 << INP_SWB2))
#define SWITCH_3x1  ((1 << INP_SWA0) | (1 << INP_SWA1) | (1 << INP_SWA2))
#define SWITCH_2x2  ((1 << INP_SWE0) | (1 << INP_SWE1) \
                   | (1 << INP_SWF0) | (1 << INP_SWF1))
#define SWITCH_2x1  ((1 << INP_SWE0) | (1 << INP_SWE1))
#define SWITCH_STOCK ((1 << INP_HOLD0) | (1 << INP_HOLD1) \
                    | (1 << INP_FMOD0) | (1 << INP_FMOD1))

//Duplicated in tx_buttons.c
enum {
  SW_A0 = 23,
  SW_A2,
  SW_B0,
  SW_B2,
  SW_C0,
  SW_C2,
  SW_D0,
  SW_D2,
  SW_E0,
  SW_F0
};

#define POT_2  ((1 << INP_AUX4) | (1 << INP_AUX5))
#define POT_1  (1 << INP_AUX4)

#if HAS_EXTRA_POTS
  const u8 adc_chan_sel[NUM_ADC_CHANNELS] = {10, 12, 13, 11, 0, 4, 16, 14};
#else
  const u8 adc_chan_sel[NUM_ADC_CHANNELS] = {10, 12, 13, 11, 16, 14};
#endif

extern u32 global_extra_switches;
void CHAN_Init()
{
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPCEN);
    #if HAS_EXTRA_POTS
      rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN);
    #endif
    ADC_Init();

    /* configure channels for analog */
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO0);
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO1);
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO2);
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO3);
    #if HAS_EXTRA_POTS
      gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO0);
      gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO4);
    #endif
    /* Enable Voltage measurement */
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO4);

    /* configure switches for digital I/O */
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN,
                   GPIO10 | GPIO11);
    gpio_set(GPIOC, GPIO10 | GPIO11);
}

s32 CHAN_ReadRawInput(int channel)
{
    s32 value = 0;
    if ((~Transmitter.ignore_src & SWITCH_3x3) != SWITCH_3x3) {
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
        case INP_SWE0:     value = global_extra_switches   & 0x04;  break;
        case INP_SWE1:     value = !(global_extra_switches & 0x0c); break;
        case INP_SWF0:     value = global_extra_switches   & 0x01;  break;
        case INP_SWF1:     value = !(global_extra_switches & 0x03); break;
      }
    } else {
      switch(channel) {
        case INP_SWA0:  value = (global_extra_switches & (1 << (SW_A0 - 1))); break;
        case INP_SWA1:  value = (!(global_extra_switches & (1 << (SW_A0 - 1))) && !(global_extra_switches & (1 << (SW_A2 - 1)))); break;
        case INP_SWA2:  value = (global_extra_switches & (1 << (SW_A2 - 1))); break;
        case INP_SWB0:  value = (global_extra_switches & (1 << (SW_B0 - 1))); break;
        case INP_SWB1:  value = (!(global_extra_switches & (1 << (SW_B0 - 1))) && !(global_extra_switches & (1 << (SW_B2 - 1)))); break;
        case INP_SWB2:  value = (global_extra_switches & (1 << (SW_B2 - 1))); break;
        case INP_SWC0:  value = (global_extra_switches & (1 << (SW_C0 - 1))); break;
        case INP_SWC1:  value = (!(global_extra_switches & (1 << (SW_C0 - 1))) && !(global_extra_switches & (1 << (SW_C2 - 1)))); break;
        case INP_SWC2:  value = (global_extra_switches & (1 << (SW_C2 - 1))); break;
        case INP_SWD0:  value = (global_extra_switches & (1 << (SW_D0 - 1))); break;
        case INP_SWD1:  value = (!(global_extra_switches & (1 << (SW_D0 - 1))) && !(global_extra_switches & (1 << (SW_D2 - 1)))); break;
        case INP_SWD2:  value = (global_extra_switches & (1 << (SW_D2 - 1))); break;
        case INP_SWE0:  value = !(global_extra_switches & (1 << (SW_E0 - 1))); break;
        case INP_SWE1:  value = (global_extra_switches & (1 << (SW_E0 - 1))); break;
        case INP_SWF0:  value = !(global_extra_switches & (1 << (SW_F0 - 1))); break;
        case INP_SWF1:  value = (global_extra_switches & (1 << (SW_F0 - 1))); break;
      }
    }
    switch(channel) {
      case INP_THROTTLE: value = adc_array_raw[0]; break;  // bug fix: right vertical
      case INP_AILERON:   value = adc_array_raw[1]; break;  // bug fix: right horizon
      case INP_RUDDER: value = adc_array_raw[2]; break;  // bug fix: left horizon
      case INP_ELEVATOR:  value = adc_array_raw[3]; break;  // bug fix: left vertical
      case INP_AUX4: value = adc_array_raw[4]; break;
      case INP_AUX5:  value = adc_array_raw[5]; break;
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
    if(strcmp(str, "3x4") == 0) {
        Transmitter.ignore_src &= ~SWITCH_3x4;
    } else if(strcmp(str, "3x3") == 0) {
        Transmitter.ignore_src &= ~SWITCH_3x3;
    } else if(strcmp(str, "3x2") == 0) {
        Transmitter.ignore_src &= ~SWITCH_3x2;
    } else if(strcmp(str, "3x1") == 0) {
        Transmitter.ignore_src &= ~SWITCH_3x1;
    } else if(strcmp(str, "2x2") == 0) {
        Transmitter.ignore_src &= ~SWITCH_2x2;
    } else if(strcmp(str, "2x1") == 0) {
        Transmitter.ignore_src &= ~SWITCH_2x1;
    } else if(strcmp(str, "potx2") == 0) {
        Transmitter.ignore_src &= ~POT_2;
    } else if(strcmp(str, "potx1") == 0) {
        Transmitter.ignore_src &= ~POT_1;
    } else {
        Transmitter.ignore_src = ~IGNORE_MASK & ~SWITCH_STOCK;
    }
    if ((~Transmitter.ignore_src & SWITCH_3x3) == SWITCH_3x3) {
        Transmitter.ignore_src |= SWITCH_STOCK;
    }
}
