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

#include "common.h"
#ifndef DISABLE_PWM

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/nvic.h>

#include "config/model.h"
#include "../ports.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>


static volatile u16 *pwm;
void PWM_Initialize()
{
#if _PWM_PIN == GPIO_USART1_TX
    UART_Stop();
#endif

    rcc_peripheral_enable_clock(&RCC_APB2ENR,
                                 RCC_APB2ENR_TIM1EN
                               | RCC_APB2ENR_IOPAEN
                               | RCC_APB2ENR_AFIOEN);
    timer_reset(TIM1);

    // Timer global mode: No divider, Alignment edge, Direction up, auto-preload buffered
    timer_set_mode(TIM1, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_enable_preload(TIM1);

    /* timer updates each microsecond */
    timer_set_prescaler(TIM1, 72 - 1);

    /* compare output setup. compare register must match i/o pin */
    timer_set_oc_mode(TIM1, _PWM_TIM_OC, TIM_OCM_PWM1); // output active while counter below compare
    timer_enable_oc_preload(TIM1, _PWM_TIM_OC);         // must use preload for PWM mode
    timer_set_oc_polarity_low(TIM1, _PWM_TIM_OC);       // notch time is low
    timer_set_oc_value(TIM1, _PWM_TIM_OC, 0);           // hold output inactive
    timer_set_period(TIM1, 22500);                      // sane default
    timer_generate_event(TIM1, TIM_EGR_UG);             // load shadow registers
    timer_enable_counter(TIM1);

    timer_enable_oc_output(TIM1, _PWM_TIM_OC);          // enable OCx to pin
    timer_enable_break_main_output(TIM1);               // master output enable
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,       // connect compare output to pin
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, _PWM_PIN);

    // interrupt on overflow (reload)
    nvic_enable_irq(NVIC_TIM1_UP_IRQ);
    nvic_set_priority(NVIC_TIM1_UP_IRQ, 1);
}


void PWM_Stop()
{
    if (PPMin_Mode())
        return;

    timer_disable_counter(TIM1);
    nvic_disable_irq(NVIC_TIM1_UP_IRQ);
    timer_disable_oc_output(TIM1, _PWM_TIM_OC);
    rcc_peripheral_disable_clock(&RCC_APB2ENR, RCC_APB2ENR_TIM1EN);

#if _PWM_PIN == GPIO_USART1_TX
    UART_Initialize();
#endif
}

void PPM_Enable(unsigned low_time, volatile u16 *pulses)
{
    pwm = pulses;
    if (*pwm) {
        timer_set_period(TIM1, *pwm++);
        timer_set_oc_value(TIM1, _PWM_TIM_OC, low_time);
        timer_generate_event(TIM1, TIM_EGR_UG); // Force-load shadow registers

        //Create an interrupt on ARR reload
        timer_clear_flag(TIM1, TIM_SR_UIF);
        timer_enable_irq(TIM1, TIM_DIER_UIE);
    }
}

void tim1_up_isr()
{
    timer_clear_flag(TIM1, TIM_SR_UIF);

    if (*pwm)
        timer_set_period(TIM1, *pwm++);
    else
        timer_set_oc_value(TIM1, _PWM_TIM_OC, 0); // hold output inactive
}

#endif //DISABLE_PWM
#pragma long_calls_off
