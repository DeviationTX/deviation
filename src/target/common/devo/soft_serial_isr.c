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

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

#include "common.h"
#include "devo.h"

#ifndef MODULAR
extern u8 in_byte;
extern u8 data_byte;
extern u8 bit_pos;
extern sser_callback_t *soft_rx_callback;

#define SSER_BIT_TIME       1250   // 17.36 us at 72MHz
// rising edge ISR
void __attribute__((__used__)) exti15_10_isr(void)
{
    exti_reset_request(EXTI10);

    if (in_byte) return;

    // start bit detected
    in_byte = 1;
    bit_pos = 0;
    data_byte = 0;
    nvic_disable_irq(NVIC_EXTI15_10_IRQ);

    //Configure timer for 1/2 bit time to start
    timer_set_period(TIM6, SSER_BIT_TIME / 2);
    nvic_enable_irq(NVIC_TIM6_IRQ);
    timer_enable_counter(TIM6);
}

static void next_byte() {
    in_byte = 0;
    timer_disable_counter(TIM6);
    nvic_enable_irq(NVIC_EXTI15_10_IRQ);
}

// bit timer ISR
void __attribute__((__used__)) tim6_isr(void) {
    timer_clear_flag(TIM6, TIM_SR_UIF);

    u16 value = gpio_get(GPIOA, _USART_GPIO_USART_RX);

    if (in_byte == 1) {  // first interrupt after edge, check start bit
        if (value) {
            in_byte = 2;
            timer_set_period(TIM6, SSER_BIT_TIME);
        } else {
            next_byte(); // error in start bit, start looking again again
        }
        return;
    }

    if (bit_pos == 8) {
        if (!value && soft_rx_callback) soft_rx_callback(data_byte);  // invoke callback if stop bit valid
        next_byte();
    }

    if (!value) data_byte |= (1 << bit_pos);
    bit_pos += 1;
}
#endif
