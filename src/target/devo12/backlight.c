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

#include <libopencm3/stm32/f1/gpio.h>
#include <libopencm3/stm32/f1/rcc.h>
#include <libopencm3/stm32/timer.h>
#include "common.h"

void BACKLIGHT_Init()
{
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN);
    //Turn off backlight
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT,
                  GPIO_CNF_INPUT_FLOAT, GPIO0);

    //Configure Backlight PWM
    rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_TIM5EN);
    timer_set_mode(TIM5, TIM_CR1_CKD_CK_INT,
                   TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_set_period(TIM5, 0x2CF);
    timer_set_prescaler(TIM5, 0);
    timer_generate_event(TIM5, TIM_EGR_UG);
    //timer_set_repetition_counter(TIM3, 0);
    timer_set_oc_mode(TIM5, TIM_OC1, TIM_OCM_PWM1);
    timer_enable_oc_preload(TIM5, TIM_OC1);

    timer_set_oc_polarity_high(TIM5, TIM_OC1);
    timer_enable_oc_output(TIM5, TIM_OC1);

    timer_enable_preload(TIM5);
}

void BACKLIGHT_Brightness(u8 brightness)
{
    timer_disable_counter(TIM5);
    if (brightness == 0) {
        // Turn off Backlight
        gpio_set_mode(GPIOA, GPIO_MODE_INPUT,
                  GPIO_CNF_INPUT_FLOAT, GPIO0);
    } else if(brightness > 9) {
        // Turn on Backlight full
        gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, GPIO0);
        gpio_set(GPIOA, GPIO0);
    } else {
        gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO0);
        u32 duty_cycle = 720 * brightness / 10 ;
        timer_set_oc_value(TIM5, TIM_OC1, duty_cycle);
        timer_enable_counter(TIM5);
    }
}

void LCD_Contrast(u8 contrast)
{
    (void)contrast; // dummy method for devo8. Only valid in devo10 now
}

