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


volatile u16 *pwm;

void tim1_up_isr()
{
    timer_clear_flag(TIM1, TIM_SR_UIF);

    if (*pwm)
        timer_set_period(TIM1, *pwm++ - 1);
    else
        timer_set_oc_value(TIM1, _PWM_TIM_OC, 0); // hold output inactive
}

#endif
