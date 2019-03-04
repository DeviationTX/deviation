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
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>

#include "common.h"
#include "target/drivers/mcu/stm32/tim.h"
#include "target/drivers/mcu/stm32/exti.h"
#include "target/drivers/mcu/stm32/nvic.h"

#ifndef MODULAR
extern u8 in_byte;
extern u8 data_byte;
extern u8 bit_pos;
extern sser_callback_t *soft_rx_callback;

#define SSER_BIT_TIME       (((TIM_FREQ_MHz(SSER_TIM.tim) * 1736) + 50) / 100)   // 17.36 us : 1250@72MHz, 1042@60MHz
// rising edge ISR
void __attribute__((__used__)) SSER_RX_ISR(void)
{
    exti_reset_request(EXTIx(UART_CFG.rx));

    if (in_byte) return;

    // start bit detected
    in_byte = 1;
    bit_pos = 0;
    data_byte = 0;
    nvic_disable_irq(NVIC_EXTIx_IRQ(UART_CFG.rx));

    //Configure timer for 1/2 bit time to start
    timer_set_period(SSER_TIM.tim, SSER_BIT_TIME / 2);
    nvic_enable_irq(get_nvic_irq(SSER_TIM.tim));
    timer_enable_counter(SSER_TIM.tim);
}

static void next_byte() {
    in_byte = 0;
    timer_disable_counter(SSER_TIM.tim);
    nvic_enable_irq(NVIC_EXTIx_IRQ(UART_CFG.rx));
}

// bit timer ISR
void __attribute__((__used__)) SSER_TIM_ISR(void) {
    timer_clear_flag(SSER_TIM.tim, TIM_SR_UIF);

    u16 value = GPIO_pin_get(UART_CFG.rx);

    if (in_byte == 1) {  // first interrupt after edge, check start bit
        if (value) {
            in_byte = 2;
            timer_set_period(SSER_TIM.tim, SSER_BIT_TIME);
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

#ifdef HAS_SSER_TX
extern volatile u8 sser_transmitting;
void __attribute__((__used__)) SSER_TX_DMA_ISR(void)
{
    timer_disable_counter(SSER_TX_TIM.tim);
    nvic_disable_irq(NVIC_DMA2_STREAM1_IRQ);
    dma_clear_interrupt_flags(SSER_TX_DMA.dma, SSER_TX_DMA.stream, DMA_TCIF);
    dma_disable_transfer_complete_interrupt(SSER_TX_DMA.dma, SSER_TX_DMA.stream);
    dma_disable_stream(SSER_TX_DMA.dma, SSER_TX_DMA.stream);
    sser_transmitting = 0;
}
#endif  // HAS_SSER_TX

#endif  // MODULAR
