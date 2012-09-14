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
#include <libopencm3/stm32/f1/rcc.h>
#include <libopencm3/stm32/f1/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/nvic.h>

#include "common.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

static volatile u16 *pwm;
void PWM_Initialize()
{
    usart_disable(USART1);
    rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB2ENR_TIM1EN);

    nvic_enable_irq(NVIC_TIM1_UP_IRQ);
    nvic_set_priority(NVIC_TIM1_UP_IRQ, 1);

    timer_disable_counter(TIM1);
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
    timer_set_period(TIM1, 65535);


    /* ---- */
    /* Output compare 1 mode and preload */
    timer_set_oc_mode(TIM1, TIM_OC2, TIM_OCM_PWM1);
    timer_enable_oc_preload(TIM1, TIM_OC1);

    /* Polarity and state */
    timer_set_oc_polarity_low(TIM1, TIM_OC2);
    timer_enable_oc_output(TIM1, TIM_OC2);

    /* Capture compare value */
    timer_set_oc_value(TIM1, TIM_OC2, 0x8000);
    /* ---- */
    /* ARR reload enable */
    timer_enable_preload(TIM1);
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

void tim1_isr()
{
    timer_clear_flag(TIM1, TIM_SR_UIF);
    if(*pwm)
        timer_set_period(TIM1, *pwm++);
    else
        timer_disable_counter(TIM1);
}
