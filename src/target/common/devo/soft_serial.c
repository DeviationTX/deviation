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
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/usart.h>
 
#include "common.h"
#include "devo.h"


// soft serial receiver for s.port data
// receive inverted data (high input volgate = 0, low is 1)
// 57600 bps, no stop, 8 data bits, 1 stop bit, lsb first
// 1) Enable rising edge interrupt on RX line to search for start bit
// 2) On rising edge, set timer for half a bit time to get to bit center
// 3) Verify start bit, set timer for interrupts at full bit time
// 4) Collect 8 data bits, verify stop bit
// 5) Process byte, then repeat

#ifndef MODULAR

static u8 in_byte;
static u8 data_byte;
static u8 bit_pos;

void SSER_Initialize()
{
#if _PWM_PIN == GPIO_USART1_TX
    UART_Stop();  // disable USART1 for GPIO PA9 & PA10 (Trainer Tx(PA9) & Rx(PA10))
#endif
    in_byte = 0;
    bit_pos = 0;
    data_byte = 0;

    /* Enable GPIOA clock. */
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN);

    // Set RX pin mode to pull up
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, _USART_GPIO_USART_RX);
    gpio_set(GPIOA, _USART_GPIO_USART_RX);
    
    // Interrupt on input rising edge to find start bit
    exti_select_source(EXTI10, GPIOA);
    exti_set_trigger(EXTI10, EXTI_TRIGGER_RISING);
    exti_enable_request(EXTI10);

    // Configure bit timer
    rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_TIM6EN);
    timer_reset(TIM6);
    nvic_set_priority(NVIC_TIM6_IRQ, 8);
    timer_set_prescaler(TIM6, 0);
    timer_enable_irq(TIM6, TIM_DIER_UIE);
}

static sser_callback_t *rx_callback;
void SSER_StartReceive(sser_callback_t *isr_callback)
{
    rx_callback = isr_callback;

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

#define SSER_BIT_TIME       1250   // 17.36 us at 72MHz
// rising edge ISR
void exti15_10_isr(void)
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
void tim6_isr(void) {
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
        if (!value && rx_callback) rx_callback(data_byte);  // invoke callback if stop bit valid
        next_byte();
    }

    if (!value) data_byte |= (1 << bit_pos);
    bit_pos += 1;
}
#endif //MODULAR
