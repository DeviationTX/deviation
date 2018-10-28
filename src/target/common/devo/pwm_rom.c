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
#ifndef DISABLE_PWM

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/nvic.h>

#include "../ports.h"


pwm_type_t pwm_type;
volatile u8 pxx_bit;
volatile u8 pxx_ones_count;
volatile u16 *pwm;
volatile u8  *pxx;

void tim1_up_isr()
{
    u8 stuffed = 0;

    timer_clear_flag(TIM1, TIM_SR_UIF);

    if (pwm_type == PWM_PPM) {
        if (*pwm)
            timer_set_period(TIM1, *pwm++ - 1);
        else
            timer_set_oc_value(TIM1, _PWM_TIM_OC, 0); // hold output inactive
        return;
    }

    // type is PWM_PXX
    if (pxx) {
        if (*pxx & pxx_bit) {
            // bit to send is 1
            if (pxx_ones_count++ == 5) {
                // stuff 0 bit after 5 ones
                width = 16;
                pxx_ones_count = 0;
                stuffed = 1;
            } else {
                width = 24;
                pxx_ones_count += 1;
            }
        } else {
            // bit to send is 0
            width = 16;
            pxx_ones_count = 0;
        }

        if (!stuffed) {
            pxx_bit >>= 1;
            if (pxx_bit == 0) {
                pxx_bit = 1 << 7;
                pxx += 1;
            }
        }

        timer_set_period(TIM1, width - 1);
        timer_set_oc_value(TIM1, _PWM_TIM_OC, width - 8);
        timer_generate_event(TIM1, TIM_EGR_UG); // Force-load shadow registers

        //Create an interrupt on ARR reload
        timer_clear_flag(TIM1, TIM_SR_UIF);
        timer_enable_irq(TIM1, TIM_DIER_UIE);
    }

}

#endif
