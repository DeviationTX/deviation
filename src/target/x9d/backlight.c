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

#include <libopencm3/stm32/f2/gpio.h>
#include <libopencm3/stm32/f2/rcc.h>
#include <libopencm3/stm32/timer.h>
#include "common.h"

#define GPIO_AF_TIM10 GPIO_AF3 //From STM32 F2 Stdlib
#define GPIO_AF_TIM4  GPIO_AF2 //From STM32 F2 Stdlib

//Backlight is on GPIOB8 which is mapped to TIM10_CH1 (or TIM4_CH3)
void BACKLIGHT_Init()
{
    rcc_peripheral_enable_clock(&RCC_AHB1ENR, RCC_AHB1ENR_IOPBEN);
    //Turn off backlight
    gpio_mode_setup(GPIOB, GPIO_MODE_INPUT,
                  GPIO_PUPD_NONE, GPIO8);

    //Configure Backlight PWM
    rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_TIM4EN);
    timer_set_period(TIM4, 0x2CF);
    timer_set_prescaler(TIM4, 0);
    timer_generate_event(TIM4, TIM_EGR_UG);
    //timer_set_repetition_counter(TIM4, 0);
    timer_set_oc_mode(TIM4, TIM_OC3, TIM_OCM_PWM1);
    timer_enable_oc_preload(TIM4, TIM_OC3);

    timer_set_oc_polarity_high(TIM4, TIM_OC3);
    timer_enable_oc_output(TIM4, TIM_OC3);

    timer_enable_preload(TIM4);
}

void BACKLIGHT_Brightness(u8 brightness)
{
    timer_disable_counter(TIM4);
    if (brightness == 0) {
        // Turn off Backlight
        gpio_mode_setup(GPIOB, GPIO_MODE_INPUT,
                  GPIO_PUPD_NONE, GPIO8);
    } else if(brightness > 9) {
        // Turn on Backlight full
        gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT,
                  GPIO_PUPD_PULLUP, GPIO8);
    } else {
        gpio_mode_setup(GPIOB, GPIO_MODE_AF,
                  GPIO_PUPD_NONE, GPIO8);
        gpio_set_af(GPIOB, GPIO_AF_TIM4, GPIO8);
        u32 duty_cycle = 360 * brightness / 10;  // 720 is too bright,
        timer_set_oc_value(TIM4, TIM_OC3, duty_cycle);
        timer_enable_counter(TIM4);
    }
}

