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
#include <libopencm3/stm32/usart.h>

#include "common.h"
#include "devo.h"

#include "target/drivers/mcu/stm32/rcc.h"

// soft serial receiver for s.port data
// receive inverted data (high input volgate = 0, low is 1)
// 57600 bps, no stop, 8 data bits, 1 stop bit, lsb first
// 1) Enable rising edge interrupt on RX line to search for start bit
// 2) On rising edge, set timer for half a bit time to get to bit center
// 3) Verify start bit, set timer for interrupts at full bit time
// 4) Collect 8 data bits, verify stop bit
// 5) Process byte, then repeat

#ifndef MODULAR

u8 in_byte;
u8 data_byte;
u8 bit_pos;
sser_callback_t *soft_rx_callback;

void SSER_Initialize()
{
#if _PWM_PIN == GPIO_USART1_TX
    UART_Stop();  // disable USART1 for GPIO PA9 & PA10 (Trainer Tx(PA9) & Rx(PA10))
#endif
    in_byte = 0;
    bit_pos = 0;
    data_byte = 0;

    /* Enable GPIOA clock. */
    rcc_periph_clock_enable(get_rcc_from_pin(UART_CFG.rx));

    // Set RX pin mode to pull up
    GPIO_setup_input(UART_CFG.rx, ITYPE_PULLUP);

    // Interrupt on input rising edge to find start bit
    exti_select_source(EXTI10, GPIOA);
    exti_set_trigger(EXTI10, EXTI_TRIGGER_RISING);
    exti_enable_request(EXTI10);

    // Configure bit timer
    rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_TIM6EN);
    rcc_periph_reset_pulse(RST_TIM6);
    nvic_set_priority(NVIC_TIM6_IRQ, 8);
    timer_set_prescaler(TIM6, 0);
    timer_enable_irq(TIM6, TIM_DIER_UIE);
}

void SSER_StartReceive(sser_callback_t *isr_callback)
{
    soft_rx_callback = isr_callback;

    if (isr_callback) {
        nvic_enable_irq(NVIC_EXTI15_10_IRQ);
    } else {
        nvic_disable_irq(NVIC_EXTI15_10_IRQ);
        nvic_disable_irq(NVIC_TIM6_IRQ);
    }
}

void SSER_Stop()
{
    SSER_StartReceive(NULL);
    timer_disable_counter(TIM6);
    exti_disable_request(EXTI10);
}

#endif //MODULAR
