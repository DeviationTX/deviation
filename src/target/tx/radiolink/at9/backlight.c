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

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include "common.h"

// FIXME introduce constants for Timer (TIM4), compare (TIM_Oc4)
// timer clock (RCC_APB1ENR_TIM4EN), Port (GPIOB), Pin (GPIO9)
// Port clock RCC_APB2ENR_IOPBEN

void BACKLIGHT_Init()
{
    // Pin PB9, Timer 4 channel 4
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPBEN);
    //Turn off backlight
    gpio_set_mode(GPIOB, GPIO_MODE_INPUT,
                  GPIO_CNF_INPUT_FLOAT, GPIO9);

    //Configure Backlight PWM
    rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_TIM4EN);
    timer_set_mode(TIM4, TIM_CR1_CKD_CK_INT,
                   TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_set_period(TIM4, 0x2CF);
    timer_set_prescaler(TIM4, 0);
    timer_generate_event(TIM4, TIM_EGR_UG);
    //timer_set_repetition_counter(TIM3, 0);
    timer_set_oc_mode(TIM4, TIM_OC4, TIM_OCM_PWM1);
    timer_enable_oc_preload(TIM4, TIM_OC4);

    timer_set_oc_polarity_high(TIM4, TIM_OC4);
    timer_enable_oc_output(TIM4, TIM_OC4);

    timer_enable_preload(TIM4);
}

void BACKLIGHT_Brightness(unsigned brightness)
{
    timer_disable_counter(TIM4);
    if (brightness == 0) {
        // Turn off Backlight
        gpio_set_mode(GPIOB, GPIO_MODE_INPUT,
                  GPIO_CNF_INPUT_FLOAT, GPIO9);
    } else if(brightness > 9) {
        // Turn on Backlight full
        gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, GPIO9);
        gpio_set(GPIOB, GPIO9);
    } else {
        gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO9);
        u32 duty_cycle = 720 * brightness / 10 ;
        timer_set_oc_value(TIM4, TIM_OC4, duty_cycle);
        timer_enable_counter(TIM4);
    }
}

void LCD_Contrast(unsigned contrast)
{
    (void)contrast; // dummy method for devo8. Only valid in devo10 now
}

