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
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/rtc.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/iwdg.h>

#include "common.h"
#include "../devo/devo.h"

// Let's abuse the preprocessor to let us specify a single
// sysclock timer value 'SYSCLK_TIM'

#define _TIM_CONCAT(x, y, z) x ## y ## z
#define TIM_CONCAT(x, y, z)  _TIM_CONCAT(x, y, z)

#define TIMx               TIM_CONCAT(TIM,             SYSCLK_TIM,)
#define RST_TIMx           TIM_CONCAT(RST_TIM,         SYSCLK_TIM,)
#define NVIC_TIMx_IRQ      TIM_CONCAT(NVIC_TIM,        SYSCLK_TIM, _IRQ)
#define RCC_APB1ENR_TIMxEN TIM_CONCAT(RCC_APB1ENR_TIM, SYSCLK_TIM, EN)

volatile u32 msecs;
volatile u32 wdg_time;
u16 (*timer_callback)(void);
volatile u8 msec_callbacks;
volatile u32 msec_cbtime[NUM_MSEC_CALLBACKS];

void _msleep(u32 msec)
{
    u32 final;
    if (msec < 2) {
        usleep(1000 * msec);
    } else {
        final = msec + msecs;
        if (final < msec) {
            while(msec + msecs < final + msec)
                CLOCK_ResetWatchdog();
        } else {
            while(msecs < final)
                CLOCK_ResetWatchdog();
        }
    }
}
void CLOCK_Init()
{
    /* 72MHz / 8 => 9000000 counts per second */
    /* 60MHz / 8 => 7500000 counts per second */
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);

    /* 9000000/9000 = 1000 overflows per second - every 1ms one interrupt */
    /* 7500000/7500 = 1000 overflows per second - every 1ms one interrupt */
    systick_set_reload((FREQ_MHz * 1000) / 8);
    nvic_set_priority(NVIC_SYSTICK_IRQ, 0x0); //Highest priority

    /* We trigger exti2 right before the watchdog fires to do a stack dump */
    nvic_set_priority(NVIC_EXTI2_IRQ, 0x01); //Highest priority

    systick_interrupt_enable();

    msecs = 0;
    msec_callbacks = 0;
    /* Start counting. */
    systick_counter_enable();

    /* Setup timer for Transmitter */
    timer_callback = NULL;
    /* Enable TIMx clock. */
    rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_TIMxEN);

    /* Enable TIMx interrupt. */
    nvic_enable_irq(NVIC_TIMx_IRQ);
    nvic_set_priority(NVIC_TIMx_IRQ, 16); //High priority

    timer_disable_counter(TIMx);
    /* Reset TIMx peripheral. */
    rcc_periph_reset_pulse(RST_TIMx);

    /* Timer global mode:
     * - No divider
     * - Alignment edge
     * - Direction up
     */
    timer_set_mode(TIMx, TIM_CR1_CKD_CK_INT,
                   TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);

    /* timer updates each microsecond */
    timer_set_prescaler(TIMx, FREQ_MHz - 1);
    timer_set_period(TIMx, 65535);

    /* Disable preload. */
    timer_disable_preload(TIMx);

    /* Continous mode. */
    timer_continuous_mode(TIMx);

    /* Disable outputs. */
    timer_disable_oc_output(TIMx, TIM_OC1);
    timer_disable_oc_output(TIMx, TIM_OC2);
    timer_disable_oc_output(TIMx, TIM_OC3);
    timer_disable_oc_output(TIMx, TIM_OC4);

    /* Enable CCP1 */
    timer_disable_oc_clear(TIMx, TIM_OC1);
    timer_disable_oc_preload(TIMx, TIM_OC1);
    timer_set_oc_slow_mode(TIMx, TIM_OC1);
    timer_set_oc_mode(TIMx, TIM_OC1, TIM_OCM_FROZEN);

    /* Disable CCP1 interrupt. */
    timer_disable_irq(TIMx, TIM_DIER_CC1IE);

    timer_enable_counter(TIMx);

    /* Enable EXTI1 interrupt for medium priority callback. */
    /* We are enabling only the interrupt
     * We'll manually trigger this via set_pending_interrupt
     */
    nvic_enable_irq(NVIC_EXTI1_IRQ);
    nvic_set_priority(NVIC_EXTI1_IRQ, 64); //Medium priority
    /* Enable DMA Channel1 with same priority as EXTI1 */
    nvic_enable_irq(_NVIC_DMA_CHANNEL_IRQ);
    nvic_set_priority(_NVIC_DMA_CHANNEL_IRQ, 65); //Medium priority

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
    unsigned t = timer_get_counter(TIMx);
    /* Set the capture compare value for OC1. */
    timer_set_oc_value(TIMx, TIM_OC1, us + t);

    timer_clear_flag(TIMx, TIM_SR_CC1IF);
    timer_enable_irq(TIMx, TIM_DIER_CC1IE);
}

void CLOCK_StartWatchdog()
{
    iwdg_set_period_ms(3000);
    iwdg_start();

    wdg_time = msecs;
    nvic_clear_pending_irq(NVIC_EXTI2_IRQ);
    nvic_enable_irq(NVIC_EXTI2_IRQ);
}

void CLOCK_ResetWatchdog()
{
    iwdg_reset();
    wdg_time = msecs;
}
void CLOCK_StopTimer() {
    timer_disable_irq(TIMx, TIM_DIER_CC1IE);
    timer_callback = NULL;
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

// Run Mixer one time.  Used by protocols that trigger mixer calc in protocol code
volatile mixsync_t mixer_sync;
void CLOCK_RunMixer(void) {
    mixer_sync = MIX_NOT_DONE;
    nvic_set_pending_irq(NVIC_EXTI1_IRQ);
}

// Run Mixer on medium priority interval.  Default behavior - no protocol code required.
void CLOCK_StartMixer() {
    mixer_sync = MIX_TIMER;
}

// initialize RTC
void RTC_Init()
{
    rtc_auto_awake(RCC_LSE, 32767); // LowSpeed External source, divided by (clock-1)=32767
}

// set date value (deviation epoch = seconds since 1.1.2012, 00:00:00)
void RTC_SetValue(u32 value)
{
    rtc_set_counter_val(value);
    //_RTC_SetDayStart(value);
}

// get date value (deviation epoch = seconds since 1.1.2012, 00:00:00)
u32 RTC_GetValue()
{
    u32 value;
    value = rtc_get_counter_val();
    //_RTC_SetDayStart(value);
    return value;
}

void _usleep(u32 x)
{
# if ((FREQ_MHz % 3) != 0)
    #error Frequency is not a multiple of 3 - need custom usleep
# else
    #define COUNT (FREQ_MHz / 3)
#endif
    asm volatile("mul %0, %0, %1;"
         "b.n _delaycmp;"
         "_delayloop:"
         "subs %0, %0, #1;"
         "_delaycmp:;"
         "cmp %0, #0;"
         "bne.n _delayloop;"
         :
         : "r" (x), "r" (COUNT)
         );
}
