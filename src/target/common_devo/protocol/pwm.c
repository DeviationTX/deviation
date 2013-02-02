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
#include <libopencm3/stm32/f1/rcc.h>
#include <libopencm3/stm32/f1/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/nvic.h>

#include "common.h"
#include "../ports.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

/* FIXME: The PWM with an output on CH2 (PA9) does not work with the
          code below.  As I was unable to find a fix the current
          implementation is to bit-bang the signal instead
*/

static volatile u16 *pwm;
void PWM_Initialize()
{
#if _PWM_PIN == GPIO_USART1_TX
    UART_Stop();
#endif
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN);
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, _PWM_PIN);
    gpio_clear(GPIOA, _PWM_PIN);
    return;
#if 0
    rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB2ENR_TIM1EN);
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN);
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_AFIOEN);

    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_TIM1_CH2 | GPIO_TIM1_CH3);
    nvic_enable_irq(NVIC_TIM1_UP_IRQ);
    nvic_set_priority(NVIC_TIM1_UP_IRQ, 1);

    timer_reset(TIM1);

    /* Timer global mode:
     * - No divider
     * - Alignment edge
     * - Direction up
     */
    timer_set_mode(TIM1, TIM_CR1_CKD_CK_INT,
                   TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);

    /* timer updates each microsecond */
    timer_set_prescaler(TIM1, 72 - 1);
    timer_set_period(TIM1, 32000);
    timer_generate_event(TIM1, TIM_EGR_UG);

    timer_continuous_mode(TIM1);
    /* ---- */
    /* Output compare 1 mode and preload */
    timer_set_oc_mode(TIM1, TIM_OC3, TIM_OCM_PWM1);
    timer_enable_oc_preload(TIM1, TIM_OC3);

    /* Polarity and state */
    timer_set_oc_polarity_high(TIM1, TIM_OC3);
    timer_enable_oc_output(TIM1, TIM_OC3);

    /* Capture compare value */
    timer_set_oc_value(TIM1, TIM_OC3, 16000);
    timer_set_oc_value(TIM1, TIM_OC3, 16000);
    /* ---- */
    /* ARR reload enable */
    timer_enable_preload(TIM1);

    timer_enable_counter(TIM1);
#endif
}
void PWM_Stop()
{
    if (RCC_APB1ENR & RCC_APB2ENR_TIM1EN) {
        rcc_peripheral_disable_clock(&RCC_APB1ENR, RCC_APB2ENR_TIM1EN);
#if _PWM_PIN == GPIO_USART1_TX
        UART_Initialize();
#endif
    }
}

void PWM_Set(int val)
{
    if(val)
        gpio_set(GPIOA, _PWM_PIN);
    else
        gpio_clear(GPIOA, _PWM_PIN);
}

void PPM_Enable(u16 low_time, volatile u16 *pulses)
{
    timer_disable_counter(TIM1);
    timer_set_oc_value(TIM1, TIM_OC2, low_time);
    pwm = pulses;
    if (*pwm) {
        timer_set_period(TIM1, *pwm++);
        //Force-load new period
        timer_generate_event(TIM1, TIM_EGR_UG);
        timer_enable_counter(TIM1);
        //Create an interrupt on ARR reload
        timer_clear_flag(TIM1, TIM_SR_UIF);
        timer_enable_irq(TIM1, TIM_DIER_UIE);
    }
}

#if 0
void tim1_up_isr()
{
    timer_clear_flag(TIM1, TIM_SR_UIF);
    if(*pwm)
        timer_set_period(TIM1, *pwm++);
    else
        timer_disable_counter(TIM1);
}
#endif
#pragma long_calls_off
