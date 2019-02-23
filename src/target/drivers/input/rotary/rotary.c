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
#include "target/drivers/mcu/stm32/exti.h"
#include "target/drivers/mcu/stm32/nvic.h"

// To use interpretation of rapid rotary movement as long press
// this timeout should be less than long press timeout, 100ms
#define ROTARY_TIMEOUT 50

volatile int rotary = 0;
static u32 last_rotary_clock = 0;

void ROTARY_Init()
{
    // This assumes all rotary pins are handled by the same interrupt
    ctassert(ROTARY_PIN0.port == ROTARY_PIN1.port, All_rotary_pins_0_1_must_be_on_same_port);
    ctassert(ROTARY_PIN0.port == ROTARY_PRESS_PIN.port, rotary_pins_0_ent_must_be_on_same_port);
    ctassert(NVIC_EXTIx_IRQ(ROTARY_PIN0) == NVIC_EXTIx_IRQ(ROTARY_PIN1), rotary_pins_0_1_must_be_on_same_irq);
    ctassert(NVIC_EXTIx_IRQ(ROTARY_PIN0) == NVIC_EXTIx_IRQ(ROTARY_PRESS_PIN), rotary_pins_0_ent_must_be_on_same_irq);

    // All pins guaranteed to be on the same port
    rcc_periph_clock_enable(get_rcc_from_pin(ROTARY_PIN0));

    /*Rotary */
    GPIO_setup_input(ROTARY_PIN0, ITYPE_PULLUP);
    GPIO_setup_input(ROTARY_PIN1, ITYPE_PULLUP);
    GPIO_setup_input(ROTARY_PRESS_PIN, ITYPE_PULLUP);

    nvic_enable_irq(NVIC_EXTIx_IRQ(ROTARY_PIN0));
    nvic_set_priority(NVIC_EXTIx_IRQ(ROTARY_PIN0), 66);  // Medium priority
    exti_select_source(EXTIx(ROTARY_PIN0) | EXTIx(ROTARY_PIN1), ROTARY_PIN0.port);
    exti_set_trigger(EXTIx(ROTARY_PIN0) | EXTIx(ROTARY_PIN1), EXTI_TRIGGER_BOTH);
    exti_enable_request(EXTIx(ROTARY_PIN0) | EXTIx(ROTARY_PIN1));
}

u32 ROTARY_Scan()
{
    int last_rotary;
    u32 result = 0;

    result |= !GPIO_pin_get(ROTARY_PRESS_PIN) ? CHAN_ButtonMask(BUT_ENTER) : 0;

    last_rotary = rotary;
    if (last_rotary) {
        u32 rotary_clock = CLOCK_getms();
        // To prevent rotary to generate button clicks too frequently we register
        // an event in 'result' not more often than every ROTARY_TIMEOUT msec
        if (rotary_clock > last_rotary_clock) {
            result |= last_rotary > 0 ? CHAN_ButtonMask(BUT_DOWN) : CHAN_ButtonMask(BUT_UP);
            last_rotary_clock = rotary_clock + ROTARY_TIMEOUT;
        }
        rotary = 0;
    }

    return result;
}

