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


#include "target.h"

#include <libopencm3/stm32/systick.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/f1/rcc.h>
#include <libopencm3/stm32/nvic.h>


u32 msecs;
void (*timer_callback)(void);

void CLOCK_Init()
{
    /* 72MHz / 8 => 9000000 counts per second */
    systick_set_clocksource(STK_CTRL_CLKSOURCE_AHB_DIV8);

    /* 9000000/9000 = 1000 overflows per second - every 1ms one interrupt */
    systick_set_reload(9000);

    systick_interrupt_enable();

    msecs = 0;
    /* Start counting. */
    systick_counter_enable();

    /* Setup timer for Transmitter */
    timer_callback = NULL;
    /* Enable TIM2 clock. */
    rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_TIM4EN);

    /* Enable TIM2 interrupt. */
    nvic_enable_irq(NVIC_TIM4_IRQ);
    nvic_set_priority(NVIC_TIM4_IRQ, 1);

    timer_disable_counter(TIM4);
    /* Reset TIM2 peripheral. */
    timer_reset(TIM4);

    /* Timer global mode:
     * - No divider
     * - Alignment edge
     * - Direction up
     */
    timer_set_mode(TIM4, TIM_CR1_CKD_CK_INT,
                   TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);

    /* timer updates each microsecond */
    timer_set_prescaler(TIM4, 72);

    /* Enable preload. */
    timer_disable_preload(TIM4);

    /* Continous mode. */
    timer_continuous_mode(TIM4);

    /* Disable outputs. */
    timer_disable_oc_output(TIM4, TIM_OC1);
    timer_disable_oc_output(TIM4, TIM_OC2);
    timer_disable_oc_output(TIM4, TIM_OC3);
    timer_disable_oc_output(TIM4, TIM_OC4);

    /* Enable commutation interrupt. */
    timer_enable_irq(TIM4, TIM_DIER_UIE);


}

void CLOCK_StartTimer(u16 us, void (*cb)(void))
{
    /* Counter enable. */
    timer_disable_counter(TIM4);
    timer_set_period(TIM4, us);
    timer_callback = cb;
    timer_enable_counter(TIM4);
}

void CLOCK_StopTimer() {
    timer_disable_counter(TIM4);
}

void tim4_isr()
{
    timer_clear_flag(TIM4, TIM_SR_UIF);
    if(timer_callback) {
        timer_callback();
    }
}

u32 CLOCK_getms()
{
    return msecs;
}

void sys_tick_handler(void)
{
	msecs++;
}
