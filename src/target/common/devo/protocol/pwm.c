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

#ifndef DISABLE_PWM

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/cm3/nvic.h>

#include "common.h"
#include "config/model.h"
#include "../ports.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

void PWM_Initialize()
{
#if _PWM_PIN == GPIO_USART1_TX
    UART_Stop();
#endif

    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_TIM1EN
                                            | RCC_APB2ENR_IOPAEN
                                            | RCC_APB2ENR_AFIOEN
                                            | _RCC_AHBENR_DMAEN);

    // connect timer compare output to pin
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, _PWM_PIN);

    timer_reset(TIM1);
    // Timer global mode: No divider, Alignment edge, Direction up, auto-preload buffered
    timer_set_mode(TIM1, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);

    /* timer updates each microsecond */
    timer_set_prescaler(TIM1, 72 - 1);

    /* compare output setup. compare register must match i/o pin */
    timer_set_oc_mode(TIM1, _PWM_TIM_OC, TIM_OCM_FORCE_HIGH); // output force high
    timer_enable_preload(TIM1);
    timer_enable_oc_preload(TIM1, _PWM_TIM_OC);
    timer_set_oc_value(TIM1, _PWM_TIM_OC, 400);     // default notch time
    timer_set_period(TIM1, 22500);                  // sane default
    timer_set_oc_polarity_low(TIM1, _PWM_TIM_OC);   // notch time is low
    // interrupt on overflow (reload)
    nvic_enable_irq(NVIC_TIM1_UP_IRQ);
    nvic_set_priority(NVIC_TIM1_UP_IRQ, 1);
    timer_set_oc_mode(TIM1, _PWM_TIM_OC, TIM_OCM_PWM1); // output active while counter below compare
    timer_enable_oc_output(TIM1, _PWM_TIM_OC);          // enable OCx to pin
    timer_enable_break_main_output(TIM1);               // master output enable
    timer_enable_counter(TIM1);
}


void PWM_Stop()
{
    if (PPMin_Mode())
        return;

    timer_disable_counter(TIM1);
    nvic_disable_irq(NVIC_TIM1_UP_IRQ);
    timer_disable_oc_output(TIM1, _PWM_TIM_OC);
    rcc_peripheral_disable_clock(&RCC_APB2ENR, RCC_APB2ENR_TIM1EN | _RCC_AHBENR_DMAEN);

#if _PWM_PIN == GPIO_USART1_TX
    UART_Initialize();
#endif
}

extern volatile u16 *pwm;  // defined in pwm_rom.c
void PPM_Enable(unsigned low_time, volatile u16 *pulses)
{
    pwm = pulses;
    if (*pwm) {
        timer_set_period(TIM1, *pwm++ - 1);
        timer_set_oc_value(TIM1, _PWM_TIM_OC, low_time);
        timer_generate_event(TIM1, TIM_EGR_UG); // Force-load shadow registers

        //Create an interrupt on ARR reload
        timer_clear_flag(TIM1, TIM_SR_UIF);
        timer_enable_irq(TIM1, TIM_DIER_UIE);
    }
}

#ifndef MODULAR
#define PXX_PKT_BITS  (20 * 8 + 32)  // every 5th bit might be escaped
static u8 pxx_bits[PXX_PKT_BITS];
void PXX_Enable(u8 *packet)
{
    u8 pxx_bit = 1 << 7;
    u8 pxx_ones_count = 0;
    u8 pxx_bytes = 20;

    if (!packet) return;

    u8 *pc = pxx_bits;
    for (;;) {
        if (*packet & pxx_bit) {
            // bit to send is 1
            if (pxx_ones_count++ == 5) {
                // stuff 0 bit after 5 ones
                *pc++ = 15;
                pxx_ones_count = 0;
            } 
            *pc++ = 23;
            pxx_ones_count += 1;
        } else {
            // bit to send is 0
            *pc++ = 15;
            pxx_ones_count = 0;
        }

        pxx_bit >>= 1;
        if (pxx_bit == 0) {
            pxx_bytes -= 1;
            if (pxx_bytes == 0) { // all bytes sent
                break;
            }
            pxx_bit = 1 << 7;
            packet++;
        }
    }

    // Reconfigure timer for PXX
    timer_set_oc_value(TIM1, _PWM_TIM_OC, 8);       // 8us high for PXX
    timer_set_period(TIM1, 23);                     // default 1
    timer_set_oc_mode(TIM1, _PWM_TIM_OC, TIM_OCM_PWM1); // output active while counter below compare
    timer_enable_oc_output(TIM1, _PWM_TIM_OC);          // enable OCx to pin
    timer_enable_break_main_output(TIM1);               // master output enable

    // Setup DMA and start
    dma_channel_reset(_PWM_DMA, _PWM_DMA_CHANNEL);
    dma_set_peripheral_address(_PWM_DMA, _PWM_DMA_CHANNEL, (u32) &_PWM_TIM_ARR);  /* send data to the TIM1 period register */
    dma_set_memory_address(_PWM_DMA, _PWM_DMA_CHANNEL, (u32) pxx_bits);
    dma_set_number_of_data(_PWM_DMA, _PWM_DMA_CHANNEL, pc - pxx_bits);
    dma_set_read_from_memory(_PWM_DMA, _PWM_DMA_CHANNEL);                     /* direction is from memory to timer */
    dma_enable_memory_increment_mode(_PWM_DMA, _PWM_DMA_CHANNEL);             /* memory pointer increments, peripheral no */
    dma_set_peripheral_size(_PWM_DMA, _PWM_DMA_CHANNEL, DMA_CCR_PSIZE_16BIT);  /* _PWM_TIM_ARR is 16 bit wide in this mode */
    dma_set_memory_size(_PWM_DMA, _PWM_DMA_CHANNEL, DMA_CCR_MSIZE_8BIT);      /* source memory is 8 bit wide */
    dma_set_priority(_PWM_DMA, _PWM_DMA_CHANNEL, DMA_CCR_PL_VERY_HIGH);
    dma_enable_transfer_complete_interrupt(_PWM_DMA, _PWM_DMA_CHANNEL);
    dma_enable_channel(_PWM_DMA, _PWM_DMA_CHANNEL);    // dma ready to go
    timer_set_dma_on_update_event(TIM1);
    timer_enable_irq(TIM1, _PWM_TIM_DIER_DMAEN);       // enable timer dma request
    timer_set_oc_mode(TIM1, _PWM_TIM_OC, TIM_OCM_PWM1); // output active while counter below compare

    nvic_set_priority(_PWM_NVIC_DMA_CHANNEL_IRQ, 3);
    nvic_enable_irq(_PWM_NVIC_DMA_CHANNEL_IRQ);

    timer_enable_counter(TIM1);
    timer_generate_event(TIM1, TIM_EGR_UG);            // Force-load shadow registers
}

void _PWM_DMA_ISR(void)
{
    DMA_IFCR(_PWM_DMA) |= DMA_IFCR_CTCIF(_PWM_DMA_CHANNEL);
    dma_disable_transfer_complete_interrupt(_PWM_DMA, _PWM_DMA_CHANNEL);
    dma_disable_channel(_PWM_DMA, _PWM_DMA_CHANNEL);
    timer_set_oc_mode(TIM1, _PWM_TIM_OC, TIM_OCM_FORCE_HIGH); // output force high
    timer_disable_counter(TIM1);
}

#endif  //MODULAR

#endif //DISABLE_PWM
#pragma long_calls_off
