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
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/f2/rcc.h>
#include <libopencm3/stm32/f2/rtc.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/iwdg.h>

#include "common.h"
#include "rtc.h"
#include "../common/devo/devo.h"

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
volatile u32 msecTimer1;
volatile u32 msecTimer2;
volatile u32 wdg_time;
u16 (*timer_callback)(void);
volatile u8 msec_callbacks;
volatile u32 msec_cbtime[NUM_MSEC_CALLBACKS];

void CLOCK_Init()
{
    /* 60MHz / 8 => 7500000 counts per second */
    systick_set_clocksource(STK_CTRL_CLKSOURCE_AHB_DIV8);

    /* 7500000/7500 = 1000 overflows per second - every 1ms one interrupt */
    systick_set_reload(7500);
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
    /* Enable TIM5 clock. */
    rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_TIM5EN);

    /* Enable TIM5 interrupt. */
    nvic_enable_irq(NVIC_TIM5_IRQ);
    nvic_set_priority(NVIC_TIM5_IRQ, 16); //High priority

    timer_disable_counter(TIM5);
    /* Reset TIM5 peripheral. */
    timer_reset(TIM5);

    /* Timer global mode:
     * - No divider
     * - Alignment edge
     * - Direction up
     */
    timer_set_mode(TIM5, TIM_CR1_CKD_CK_INT,
                   TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);

    /* timer updates each microsecond */
    timer_set_prescaler(TIM5, 60 - 1);
    timer_set_period(TIM5, 65535);

    /* Disable preload. */
    timer_disable_preload(TIM5);

    /* Continous mode. */
    timer_continuous_mode(TIM5);

    /* Disable outputs. */
    timer_disable_oc_output(TIM5, TIM_OC1);
    timer_disable_oc_output(TIM5, TIM_OC2);
    timer_disable_oc_output(TIM5, TIM_OC3);
    timer_disable_oc_output(TIM5, TIM_OC4);

    /* Enable CCP1 */
    timer_disable_oc_clear(TIM5, TIM_OC1);
    timer_disable_oc_preload(TIM5, TIM_OC1);
    timer_set_oc_slow_mode(TIM5, TIM_OC1);
    timer_set_oc_mode(TIM5, TIM_OC1, TIM_OCM_FROZEN);

    /* Disable CCP1 interrupt. */
    timer_disable_irq(TIM5, TIM_DIER_CC1IE);

    timer_enable_counter(TIM5);

    /* Enable EXTI1 interrupt. */
    /* We are enabling only the interrupt
     * We'll manually trigger this via set_pending_interrupt
     */
    nvic_enable_irq(NVIC_EXTI1_IRQ);
    nvic_set_priority(NVIC_EXTI1_IRQ, 64); //Medium priority
    /* Enable DMA Channel1 with same priority as EXTI1 */
    //FIXME
    //nvic_enable_irq(NVIC_DMA1_STREAM1_IRQ);
    //nvic_set_priority(NVIC_DMA1_STREAM1_IRQ, 65); //Medium priority

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
    u16 t = timer_get_counter(TIM5);
    /* Set the capture compare value for OC1. */
    timer_set_oc_value(TIM5, TIM_OC1, us + t);

    timer_clear_flag(TIM5, TIM_SR_CC1IF);
    timer_enable_irq(TIM5, TIM_DIER_CC1IE);
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
    timer_disable_irq(TIM5, TIM_DIER_CC1IE);
    timer_callback = NULL;
}

void tim5_isr()
{
    if(timer_callback) {
#ifdef TIMING_DEBUG
        debug_timing(4, 0);
#endif
        u16 us = timer_callback();
#ifdef TIMING_DEBUG
        debug_timing(4, 1);
#endif
        timer_clear_flag(TIM5, TIM_SR_CC1IF);
        if (us) {
            timer_set_oc_value(TIM5, TIM_OC1, us + TIM_CCR1(TIM5));
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


void exti1_isr()
{
    //ADC_StartCapture();
    //ADC completion will trigger update
    ADC_Filter();
    medium_priority_cb();
}

void sys_tick_handler(void)
{
	msecs++;
	if(msecTimer1)
		msecTimer1--;
	if(msecTimer2)
		msecTimer2--;
        if(msecs - wdg_time > 2000) {
            nvic_set_pending_irq(NVIC_EXTI2_IRQ);
            return;
        }
        if(msec_callbacks & (1 << MEDIUM_PRIORITY)) {
            if (msecs == msec_cbtime[MEDIUM_PRIORITY]) {
                //medium priority tasks execute in interrupt and main loop context
                nvic_set_pending_irq(NVIC_EXTI1_IRQ);
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

// initialize RTC
void RTC_Init()
{
    
    rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_PWREN);
    pwr_disable_backup_domain_write_protect();
    rcc_osc_on(LSE);
    rcc_wait_for_osc_ready(LSE);
    RCC_BDCR |= RCC_BDCR_SRC_LSE; //Set source to LSE
    RCC_BDCR |= RCC_BDCR_RTCEN;   //Enable RTC
    rtc_wait_for_synchro();
    rtc_set_prescaler(255, 127);
}

static const u16 daysInYear[2][13] = { { 0,31,59,90,120,151,181,212,243,273,304,334,365},
                                       { 0,31,60,91,121,152,182,213,244,274,305,335,366} };
// set date value (deviation epoch = seconds since 1.1.2012, 00:00:00)
void RTC_SetValue(u32 value)
{
    value += 4382 * 60 * 60 * 24; //convert date from 1.1.2012, 00:00:00 to 1.1.2000, 00:00:00
    uint32_t date = 0, time = 0;
    const uint32_t SEC  = value % 60;
    const uint32_t MIN  = (value / 60) % 60;
    const uint32_t HR   = (value / 60 / 60) % 24;
    uint32_t       DAYS = (value / 60 / 60 / 24);
    uint32_t       DAY  = 0;
    uint32_t       YEAR = (4*DAYS) / 1461; // = days/365.25
    uint32_t       LEAP = (YEAR % 4 == 0) ? 1 : 0;
    uint32_t    WEEKDAY = DAYS / 7 + 7; //1/1/2000 was a Saturday
    uint32_t      MONTH = 0;
    //Convert time to bcd
    time |= (SEC % 10) <<  0; //seconds ones
    time |= (SEC / 10) <<  4; //seconds tens
    time |= (MIN % 10) <<  8; //minutes ones
    time |= (MIN / 10) << 12; //minutes tens
    time |= (HR  % 10) << 16; //hours ones
    time |= (HR  / 10) << 20; //hours tens
    //Convert date to bcd
    DAYS -= (uint32_t)(YEAR * 365 + YEAR / 4);
    DAYS -= (DAYS > daysInYear[LEAP][2]) ? 1 : 0;    //leap year correction for RTC_STARTYEAR
    for (MONTH=0; MONTH<12; MONTH++) {
        if (DAYS < daysInYear[LEAP][MONTH + 1]) break;
    }
    DAY = DAYS - daysInYear[LEAP][MONTH];
    date |= (DAY   % 10) <<  0; //date in ones
    date |= (DAY   / 10) <<  4; //date in tens
    date |= (MONTH % 10) <<  8; //month in ones
    date |= (MONTH / 10) << 12; //month in tens
    date |= (WEEKDAY)    << 13; //weekday
    date |= (YEAR  % 10) << 16; //year in ones
    date |= (YEAR  / 10) << 20; //year in tens
    //Unlock
    rtc_unlock();
    //Enter Init mode
    RTC_ISR = 0xFFFFFFFF;
    for(int i = 0; i < 0x10000; i++)
        if((RTC_ISR & RTC_ISR_INITF) == 0)
            break;
    //SetDate
    RTC_DR = date;
    RTC_TR = time;
    // Exit Init mode
    RTC_ISR &= (uint32_t)~RTC_ISR_INIT;
    //Wait for synch
    rtc_wait_for_synchro();
    //Lock
    rtc_lock();
}

// get date value (deviation epoch = seconds since 1.1.2012, 00:00:00)
u32 RTC_GetValue()
{
    u32 value = 0;
    uint32_t time = RTC_TR;
    uint32_t date = RTC_DR;

    const uint32_t YEAR  = (((date >> 20) & 0x0f) * 10) + ((date >> 16) & 0x0f) - 12;
    const uint32_t MONTH = (((date >> 12) & 0x01) * 10) + ((date >>  8) & 0x0f);
    const uint32_t DAY   = (((date >>  4) & 0x03) * 10) + ((date >>  0) & 0x0f);
    const uint32_t HOUR  = (((time >> 20) & 0x03) * 10) + ((time >> 16) & 0x0f);
    const uint32_t MIN   = (((time >> 12) & 0x07) * 10) + ((time >>  8) & 0x0f);
    const uint32_t SEC   = (((time >>  4) & 0x07) * 10) + ((time >>  0) & 0x0f);

    value += (DAY-1 + daysInYear[YEAR%4 == 0 ? 1 : 0][MONTH-1] + YEAR * 365 + YEAR/4 + ((YEAR != 0 && MONTH > 2) ? 1 : 0)) * (60*60*24);
    value += HOUR*60*60 + MIN*60+SEC;
    return value;
}

void rtc_gettime(struct gtm * t)
{
    uint32_t time = RTC_TR;
    uint32_t date = RTC_DR;

    const uint32_t YEAR  = (((date >> 20) & 0x0f) * 10) + ((date >> 16) & 0x0f);
    const uint32_t MONTH = (((date >> 12) & 0x01) * 10) + ((date >>  8) & 0x0f);
    const uint32_t DAY   = (((date >>  4) & 0x03) * 10) + ((date >>  0) & 0x0f);
    const uint32_t HOUR  = (((time >> 20) & 0x03) * 10) + ((time >> 16) & 0x0f);
    const uint32_t MIN   = (((time >> 12) & 0x07) * 10) + ((time >>  8) & 0x0f);
    const uint32_t SEC   = (((time >>  4) & 0x07) * 10) + ((time >>  0) & 0x0f);

    t->tm_hour = HOUR;
    t->tm_min  = MIN;
    t->tm_sec  = SEC;
    t->tm_year = YEAR + 100;
    t->tm_mon  = MONTH;
    t->tm_mday = DAY;
}

