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
#include "devo.h"

#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/f1/rcc.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/iwdg.h>

//The following is from an unreleased libopencm3
//We should remove it eventually
#if 1
void iwdg_start(void);
void iwdg_set_period_ms(u32 period);
bool iwdg_reload_busy(void);
bool iwdg_prescaler_busy(void);
void iwdg_reset(void);
#endif

volatile u32 msecs;
u16 (*timer_callback)(void);
volatile u8 msec_callbacks;
volatile u32 msec_cbtime[NUM_MSEC_CALLBACKS];

void CLOCK_Init()
{
    /* 72MHz / 8 => 9000000 counts per second */
    systick_set_clocksource(STK_CTRL_CLKSOURCE_AHB_DIV8);

    /* 9000000/9000 = 1000 overflows per second - every 1ms one interrupt */
    systick_set_reload(9000);
    nvic_set_priority(NVIC_SYSTICK_IRQ, 0x0); //Highest priority

    systick_interrupt_enable();

    msecs = 0;
    msec_callbacks = 0;
    /* Start counting. */
    systick_counter_enable();

    /* Setup timer for Transmitter */
    timer_callback = NULL;
    /* Enable TIM4 clock. */
    rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_TIM4EN);

    /* Enable TIM2 interrupt. */
    nvic_enable_irq(NVIC_TIM4_IRQ);
    nvic_set_priority(NVIC_TIM4_IRQ, 16); //High priority

    timer_disable_counter(TIM4);
    /* Reset TIM4 peripheral. */
    timer_reset(TIM4);

    /* Timer global mode:
     * - No divider
     * - Alignment edge
     * - Direction up
     */
    timer_set_mode(TIM4, TIM_CR1_CKD_CK_INT,
                   TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);

    /* timer updates each microsecond */
    timer_set_prescaler(TIM4, 72 - 1);
    timer_set_period(TIM4, 65535);

    /* Disable preload. */
    timer_disable_preload(TIM4);

    /* Continous mode. */
    timer_continuous_mode(TIM4);

    /* Disable outputs. */
    timer_disable_oc_output(TIM4, TIM_OC1);
    timer_disable_oc_output(TIM4, TIM_OC2);
    timer_disable_oc_output(TIM4, TIM_OC3);
    timer_disable_oc_output(TIM4, TIM_OC4);

    /* Enable CCP1 */
    timer_disable_oc_clear(TIM4, TIM_OC1);
    timer_disable_oc_preload(TIM4, TIM_OC1);
    timer_set_oc_slow_mode(TIM4, TIM_OC1);
    timer_set_oc_mode(TIM4, TIM_OC1, TIM_OCM_FROZEN);

    /* Disable CCP1 interrupt. */
    timer_disable_irq(TIM4, TIM_DIER_CC1IE);

    timer_enable_counter(TIM4);

    /* Enable TIM3 interrupt. */
    /* We are enabling the interrupt but not the timer.
       We'll manually trigger this via set_pending_interrupt
     */
    nvic_enable_irq(NVIC_TIM3_IRQ);
    nvic_set_priority(NVIC_TIM3_IRQ, 64); //Medium priority
    /* Enable DMA Channel1 with same priority as TIM3 */
    nvic_enable_irq(NVIC_DMA1_CHANNEL1_IRQ);
    nvic_set_priority(NVIC_DMA1_CHANNEL1_IRQ, 65); //Medium priority

    /* wait for system to start up and stabilize */
    while(msecs < 100)
        ;
}

void CLOCK_StartTimer(u16 us, u16 (*cb)(void))
{
    if(! cb)
        return;
    timer_callback = cb;
    /* Counter enable. */
    u16 t = timer_get_counter(TIM4);
    /* Set the capture compare value for OC1. */
    timer_set_oc_value(TIM4, TIM_OC1, us + t);

    timer_clear_flag(TIM4, TIM_SR_CC1IF);
    timer_enable_irq(TIM4, TIM_DIER_CC1IE);
}

void CLOCK_StartWatchdog()
{
    iwdg_set_period_ms(2000);
    iwdg_start();
}

void CLOCK_ResetWatchdog()
{
    iwdg_reset();
}
void CLOCK_StopTimer() {
    timer_disable_irq(TIM4, TIM_DIER_CC1IE);
    timer_callback = NULL;
}

void tim4_isr()
{
    if(timer_callback) {
#ifdef TIMING_DEBUG
        debug_timing(4, 0);
#endif
        u16 us = timer_callback();
#ifdef TIMING_DEBUG
        debug_timing(4, 1);
#endif
        timer_clear_flag(TIM4, TIM_SR_CC1IF);
        if (us) {
            timer_set_oc_value(TIM4, TIM_OC1, us + TIM_CCR1(TIM4));
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


void tim3_isr()
{
    ADC_StartCapture();
    //ADC completion will trigger update
    //medium_priority_cb();
}

void sys_tick_handler(void)
{
	msecs++;
        if(msec_callbacks & (1 << MEDIUM_PRIORITY)) {
            if (msecs == msec_cbtime[MEDIUM_PRIORITY]) {
                //medium priority tasks execute in interrupt and main loop context
                nvic_set_pending_irq(NVIC_TIM3_IRQ);
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
        if(msec_callbacks & (1 << TIMER_SOUND)) {
            if (msecs == msec_cbtime[TIMER_SOUND]) {
                u16 ms = SOUND_Callback();
                if(! ms)
                    msec_callbacks &= ~(1 << TIMER_SOUND);
                else
                    msec_cbtime[TIMER_SOUND] = msecs + ms;
            }
        }
}
