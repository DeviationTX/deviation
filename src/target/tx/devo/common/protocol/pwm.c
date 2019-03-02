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
#ifdef MODULAR
  //Allows the linker to properly relocate
  #define DEVO_Cmds PROTO_Cmds
  #pragma long_calls
#endif

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

#include "common.h"
#include "config/model.h"
#include "../ports.h"
#include "target/drivers/mcu/stm32/rcc.h"
#include "target/drivers/mcu/stm32/dma.h"
#include "target/drivers/mcu/stm32/nvic.h"
#include "target/drivers/mcu/stm32/tim.h"
#include "target/drivers/mcu/stm32/exti.h"

#ifndef DISABLE_PWM

void PWM_Initialize()
{
    if (PWM_TIMER.pin.pin == GPIO_USART1_TX) {
        UART_Stop();
    }
    rcc_periph_clock_enable(get_rcc_from_port(PWM_TIMER.tim));
    rcc_periph_clock_enable(get_rcc_from_pin(PWM_TIMER.pin));
    rcc_periph_clock_enable(get_rcc_from_port(PWM_DMA.dma));
    rcc_periph_clock_enable(RCC_AFIO);

    // connect timer compare output to pin
    GPIO_setup_output_af(PWM_TIMER.pin, OTYPE_PUSHPULL, PWM_TIMER.tim);

    rcc_periph_reset_pulse(RST_TIMx(PWM_TIMER.tim));

    // Timer global mode: No divider, Alignment edge, Direction up, auto-preload buffered
    timer_set_mode(PWM_TIMER.tim, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);

    /* timer updates each microsecond */
    timer_set_prescaler(PWM_TIMER.tim, TIM_FREQ_MHz(PWM_TIMER.tim) - 1);

    /* compare output setup. compare register must match i/o pin */
    timer_set_oc_mode(PWM_TIMER.tim, TIM_OCx(PWM_TIMER.ch), TIM_OCM_FORCE_HIGH);  // output force high

    // further specific initialization in PPM_Enable and PXX_Enable
}


void PWM_Stop()
{
    if (PPMin_Mode())
        return;

    timer_disable_counter(PWM_TIMER.tim);
    nvic_disable_irq(get_nvic_dma_irq(PWM_DMA));
    dma_disable_transfer_complete_interrupt(PWM_DMA.dma, PWM_DMA.stream);
    dma_disable_channel(PWM_DMA.dma, PWM_DMA.stream);
    rcc_periph_clock_disable(get_rcc_from_port(PWM_TIMER.tim));

    if (PWM_TIMER.pin.pin == GPIO_USART1_TX) {
        UART_Initialize();
    }
}

void PPM_Enable(unsigned active_time, volatile u16 *pulses, u8 num_pulses, u8 polarity)
{
    if (!pulses || num_pulses < 1) return;

    // Setup DMA
    DMA_stream_reset(PWM_DMA);
    dma_set_peripheral_address(PWM_DMA.dma, PWM_DMA.stream, (u32) &TIM_ARR(PWM_TIMER.tim));
    dma_set_memory_address(PWM_DMA.dma, PWM_DMA.stream, (u32) pulses);
    dma_set_number_of_data(PWM_DMA.dma, PWM_DMA.stream, num_pulses);
    dma_set_read_from_memory(PWM_DMA.dma, PWM_DMA.stream);
    dma_enable_memory_increment_mode(PWM_DMA.dma, PWM_DMA.stream);
    dma_set_peripheral_size(PWM_DMA.dma, PWM_DMA.stream, DMA_SxCR_PSIZE_16BIT);
    dma_set_memory_size(PWM_DMA.dma, PWM_DMA.stream, DMA_SxCR_MSIZE_16BIT);
    dma_set_priority(PWM_DMA.dma, PWM_DMA.stream, DMA_CCR_PL_VERY_HIGH);
    DMA_channel_select(PWM_DMA);
    DMA_enable_stream(PWM_DMA);    // dma ready to go

    // Setup timer for PPM
    timer_set_oc_value(PWM_TIMER.tim, TIM_OCx(PWM_TIMER.ch), active_time);
    timer_set_period(PWM_TIMER.tim, 22500);
    if (polarity == PPM_POLARITY_NORMAL)
        timer_set_oc_polarity_low(PWM_TIMER.tim, TIM_OCx(PWM_TIMER.ch));        // output active low
    else
        timer_set_oc_polarity_high(PWM_TIMER.tim, TIM_OCx(PWM_TIMER.ch));        // output active high
    timer_set_oc_mode(PWM_TIMER.tim, TIM_OCx(PWM_TIMER.ch), TIM_OCM_PWM1);  // output active when timer below compare
    timer_enable_oc_output(PWM_TIMER.tim, TIM_OCx(PWM_TIMER.ch));           // enable OCx to pin
    timer_enable_break_main_output(PWM_TIMER.tim);               // master output enable
    timer_set_dma_on_update_event(PWM_TIMER.tim);
    timer_enable_irq(PWM_TIMER.tim, TIM_DIER_CCxDE(PWM_TIMER.ch));  // enable timer dma request (despite function name)
    timer_generate_event(PWM_TIMER.tim, TIM_EGR_UG);      // Generate update event to start DMA
    timer_enable_counter(PWM_TIMER.tim);
}

#define PXX_PKT_BYTES 18
#define PXX_PKT_BITS  (20 * 8 + 32)  // every 5th bit might be escaped
static const u8 pxx_flag[] = {15, 23, 23, 23, 23, 23, 23, 15};  // 7e
static u8 pxx_bits[PXX_PKT_BITS];
void PXX_Enable(u8 *packet)
{
    if (!packet) return;

    u8 pxx_bit = 1 << 7;
    u8 pxx_ones_count = 0;
    u8 *pc = pxx_bits;

    // flag bytes are not bit-stuffed
    memcpy(pc, pxx_flag, 8);
    pc += 8;
    // set pulse length for each bit of payload bytes
    for (int i=0; i < PXX_PKT_BYTES * 8; i++) {
        if (*packet & pxx_bit) {
            // bit to send is 1
            *pc++ = 23;
            pxx_ones_count += 1;
            if (pxx_ones_count == 5) {
                // stuff 0 bit after 5 ones
                *pc++ = 15;
                pxx_ones_count = 0;
            } 
        } else {
            // bit to send is 0
            *pc++ = 15;
            pxx_ones_count = 0;
        }

        pxx_bit >>= 1;
        if (pxx_bit == 0) {
            pxx_bit = 1 << 7;
            packet++;
        }
    }
    memcpy(pc, pxx_flag, 8);
    pc += 8;

    // Setup DMA
    DMA_stream_reset(PWM_DMA);
    dma_set_peripheral_address(PWM_DMA.dma, PWM_DMA.stream, (u32) &TIM_ARR(PWM_TIMER.tim));
    dma_set_memory_address(PWM_DMA.dma, PWM_DMA.stream, (u32) pxx_bits);
    dma_set_number_of_data(PWM_DMA.dma, PWM_DMA.stream, pc - pxx_bits + 1);
    dma_set_read_from_memory(PWM_DMA.dma, PWM_DMA.stream);
    dma_enable_memory_increment_mode(PWM_DMA.dma, PWM_DMA.stream);
    dma_set_peripheral_size(PWM_DMA.dma, PWM_DMA.stream, DMA_SxCR_PSIZE_16BIT);
    dma_set_memory_size(PWM_DMA.dma, PWM_DMA.stream, DMA_SxCR_MSIZE_8BIT);
    dma_set_priority(PWM_DMA.dma, PWM_DMA.stream, DMA_CCR_PL_VERY_HIGH);
    dma_enable_transfer_complete_interrupt(PWM_DMA.dma, PWM_DMA.stream);
    DMA_channel_select(PWM_DMA);
    DMA_enable_stream(PWM_DMA);    // dma ready to go

    // configure timer for PXX
    timer_set_period(PWM_TIMER.tim, 0);                          // hold timer stopped
    timer_set_oc_value(PWM_TIMER.tim, TIM_OCx(PWM_TIMER.ch), 8);           // 8 us high for PXX
    timer_enable_oc_output(PWM_TIMER.tim, TIM_OCx(PWM_TIMER.ch));          // enable OCx to pin
    timer_enable_break_main_output(PWM_TIMER.tim);               // master output enable
    timer_set_dma_on_update_event(PWM_TIMER.tim);
    timer_enable_irq(PWM_TIMER.tim, TIM_DIER_CCxDE(PWM_TIMER.ch));          // enable timer dma request
    timer_set_oc_mode(PWM_TIMER.tim, TIM_OCx(PWM_TIMER.ch), TIM_OCM_PWM1);  // output active while counter below compare

    nvic_set_priority(get_nvic_dma_irq(PWM_DMA), 3);    // DMA interrupt on transfer complete
    nvic_enable_irq(get_nvic_dma_irq(PWM_DMA));

    timer_enable_counter(PWM_TIMER.tim);
    timer_generate_event(PWM_TIMER.tim, TIM_EGR_UG);             // Generate event to start DMA
}

#endif //DISABLE_PWM
#pragma long_calls_off
