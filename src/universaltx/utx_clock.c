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

#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/f0/rcc.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/iwdg.h>

#include "common.h"

//The following is from an unreleased libopencm3
//We should remove it eventually

volatile u32 msecs;
volatile u32 wdg_time;
u16 (*timer_callback)(void);
volatile u8 msec_callbacks;
volatile u32 msec_cbtime[NUM_MSEC_CALLBACKS];

void CLOCK_Init()
{
    /* 48MHz / 8 => 6_000_000 counts per second */
    systick_set_clocksource(STK_CSR_CLKSOURCE_EXT);

    /* 6000000/6000 = 1000 overflows per second - every 1ms one interrupt */
    systick_set_reload(6000);
    nvic_set_priority(NVIC_SYSTICK_IRQ, 0 << 6); //Highest priority

    /* We trigger exti2 right before the watchdog fires to do a stack dump */
    nvic_set_priority(NVIC_EXTI2_3_IRQ, 0 << 6); //Highest priority

    systick_interrupt_enable();

    msecs = 0;
    msec_callbacks = 0;
    /* Start counting. */

    systick_clear();
    systick_counter_enable();

    /* Setup timer for Transmitter */
    timer_callback = NULL;
    /* Enable TIM14 clock. */
    rcc_periph_clock_enable(RCC_TIM14);

    /* Enable TIM2 interrupt. */
    nvic_enable_irq(NVIC_TIM14_IRQ);
    nvic_set_priority(NVIC_TIM14_IRQ, 1 << 6); //High priority

    timer_disable_counter(TIM14);
    /* Reset TIM14 peripheral. */
    timer_reset(TIM14);

    /* Timer global mode:
     * - No divider
     * - Alignment edge
     * - Direction up
     */
    timer_set_mode(TIM14, TIM_CR1_CKD_CK_INT,
                   TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);

    /* timer updates each microsecond */
    timer_set_prescaler(TIM14, 48 - 1);
    timer_set_period(TIM14, 65535);

    /* Disable preload. */
    timer_disable_preload(TIM14);

    /* Continous mode. */
    timer_continuous_mode(TIM14);

    /* Disable outputs. */
    timer_disable_oc_output(TIM14, TIM_OC1);

    /* Enable CCP1 */
    timer_disable_oc_clear(TIM14, TIM_OC1);
    timer_disable_oc_preload(TIM14, TIM_OC1);
    timer_set_oc_slow_mode(TIM14, TIM_OC1);
    timer_set_oc_mode(TIM14, TIM_OC1, TIM_OCM_FROZEN);

    /* Disable CCP1 interrupt. */
    timer_disable_irq(TIM14, TIM_DIER_CC1IE);

    timer_enable_counter(TIM14);

    /* wait for system to start up and stabilize */
    while(msecs < 100)
        ;
}

void CLOCK_StartTimer(unsigned us, u16 (*cb)(void))
{
    if(! cb)
        return;
    timer_callback = cb;
    /* Counter enable. */
    unsigned t = timer_get_counter(TIM14);
    /* Set the capture compare value for OC1. */
    timer_set_oc_value(TIM14, TIM_OC1, us + t);

    timer_clear_flag(TIM14, TIM_SR_CC1IF);
    timer_enable_irq(TIM14, TIM_DIER_CC1IE);
}

void CLOCK_StartWatchdog()
{
    iwdg_set_period_ms(3000);
    iwdg_start();

    wdg_time = msecs;
    nvic_clear_pending_irq(NVIC_EXTI2_3_IRQ);
    nvic_enable_irq(NVIC_EXTI2_3_IRQ);
}

void CLOCK_ResetWatchdog()
{
    iwdg_reset();
    wdg_time = msecs;
}
void CLOCK_StopTimer() {
    timer_disable_irq(TIM14, TIM_DIER_CC1IE);
    timer_callback = NULL;
}

void tim14_isr()
{
    if(timer_callback) {
#ifdef TIMING_DEBUG
        debug_timing(4, 0);
#endif
        unsigned us = timer_callback();
#ifdef TIMING_DEBUG
        debug_timing(4, 1);
#endif
        timer_clear_flag(TIM14, TIM_SR_CC1IF);
        if (us) {
            timer_set_oc_value(TIM14, TIM_OC1, us + TIM_CCR1(TIM14));
            return;
        }
    }
    CLOCK_StopTimer();
}

u32 CLOCK_getms()
{
    return msecs;
}

void CLOCK_SetMsecCallback(int cb, u32 msec)
{
    msec_cbtime[cb] = msecs + msec;
    msec_callbacks |= (1 << cb);
}

void CLOCK_ClearMsecCallback(int cb)
{
    msec_callbacks &= ~(1 << cb);
}


void sys_tick_handler(void)
{
	msecs++;
        //if(msecs - wdg_time > 2000) {
        //    nvic_set_pending_irq(NVIC_EXTI2_3_IRQ);
        //    return;
        //}
        if(msec_callbacks & (1 << MEDIUM_PRIORITY)) {
            if (msecs == msec_cbtime[MEDIUM_PRIORITY]) {
                //medium priority tasks execute in interrupt and main loop context
                priority_ready |= 1 << MEDIUM_PRIORITY;
                msec_cbtime[MEDIUM_PRIORITY] = msecs + MEDIUM_PRIORITY_MSEC;
            }
        }
        if(msec_callbacks & (1 << LOW_PRIORITY)) {
            if (msecs == msec_cbtime[LOW_PRIORITY]) {
                //Low priority tasks execute in the main loop
                priority_ready |= 1 << LOW_PRIORITY;
                msec_cbtime[LOW_PRIORITY] = msecs + LOW_PRIORITY_MSEC;
            }
        }
}
