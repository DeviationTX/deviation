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

#include "common.h"
#include "target/drivers/mcu/stm32/rcc.h"

#define TO_PIN(...) (struct mcu_pin) { __VA_ARGS__ }

#ifndef STOCK_INPUTS
    #ifdef SWITCH_STOCK
        #define STOCK_INPUTS SWITCH_STOCK
    #else
        #define STOCK_INPUTS -1
    #endif
#endif

void SWITCH_Init()
{
    #define CHAN_INVERT ITYPE_PULLUP
    #define CHAN_NONINV ITYPE_PULLDOWN

    #define THREE_WAY(x, pin0, pin2, inv) \
        rcc_periph_clock_enable(get_rcc_from_pin(TO_PIN pin0)); \
        rcc_periph_clock_enable(get_rcc_from_pin(TO_PIN pin2)); \
        GPIO_setup_input(TO_PIN pin0, inv); \
        GPIO_setup_input(TO_PIN pin2, inv);
    #define TWO_WAY(x, pin, inv) \
        rcc_periph_clock_enable(get_rcc_from_pin(TO_PIN pin)); \
        GPIO_setup_input(TO_PIN pin, inv);

    SWITCHES

    #undef CHAN_INVERT
    #undef CHAN_NONINV
    #undef THREE_WAY
    #undef TWO_WAY

    Transmitter.ignore_src = ~STOCK_INPUTS;
}

s32 SWITCH_ReadRawInput(int channel)
{
    s32 value = 0;
    #define CHAN_INVERT !
    #define CHAN_NONINV
    #define THREE_WAY(ch, pin0, pin2, inv) \
        case ch##0: value = inv GPIO_pin_get(TO_PIN pin0); break; \
        case ch##1: value = !(inv GPIO_pin_get(TO_PIN pin0) || inv GPIO_pin_get(TO_PIN pin2)) ; break; \
        case ch##2: value = inv GPIO_pin_get(TO_PIN pin2); break;
    #define TWO_WAY(ch, pin, inv) \
        case ch##0: value = !inv GPIO_pin_get(TO_PIN pin); break; \
        case ch##1: value = inv GPIO_pin_get(TO_PIN pin); break;

    switch (channel) {
        SWITCHES
    }
    return value;
}
