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
#include <libopencm3/cm3/nvic.h>

#include "common.h"
#include "target/tx/devo/common/devo.h"
#include "target/drivers/mcu/stm32/tim.h"

extern volatile u32 msecs;
extern volatile u32 wdg_time;

extern u16 (*timer_callback)(void);
extern volatile u8 msec_callbacks;
extern volatile u32 msec_cbtime[NUM_MSEC_CALLBACKS];

void __attribute__((__used__)) SYSCLK_TIMER_ISR()
{
    if(timer_callback) {
#ifdef TIMING_DEBUG
        debug_timing(4, 0);
#endif
        unsigned us = timer_callback();
#ifdef TIMING_DEBUG
        debug_timing(4, 1);
#endif
        timer_clear_flag(SYSCLK_TIM.tim, TIM_SR_CC1IF);
        if (us) {
            timer_set_oc_value(SYSCLK_TIM.tim, TIM_OCx(SYSCLK_TIM.ch), us + TIM_CCR1(SYSCLK_TIM.tim));
            return;
        }
    }
    CLOCK_StopTimer();
}

void __attribute__((__used__)) exti1_isr()
{
    // medium_priority_cb();  Currently not used. If needed,
    // use exti3 for mixer updates.
    ADC_Filter();
    MIXER_CalcChannels();
    if (mixer_sync == MIX_NOT_DONE) mixer_sync = MIX_DONE;
}

void __attribute__((__used__)) sys_tick_handler(void)
{
    msecs++;
    if(msecs - wdg_time > 2000) {
        nvic_set_pending_irq(NVIC_EXTI2_IRQ);
        return;
    }
    if(msec_callbacks & (1 << MEDIUM_PRIORITY)) {
        //medium priority tasks execute in interrupt and main loop context
        if (msecs == msec_cbtime[MEDIUM_PRIORITY]) {
            // currently the mixer calculations is the only code that runs under medium interrupt priority,
            // so only schedule interrupt if using periodic mixer calc (not per-protocol mixer calc)
            // If any other code added in medium priority interrupt handler, move the following line to
            // exti1_isr before the CLOCK_UpdateMixers() line.
            // overload mixer_sync to fit in 7e build - NULL indicates mixer run by timer
            if (mixer_sync == MIX_TIMER) nvic_set_pending_irq(NVIC_EXTI1_IRQ);
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
            unsigned ms = SOUND_Callback();
            if(! ms)
                msec_callbacks &= ~(1 << TIMER_SOUND);
            else
                msec_cbtime[TIMER_SOUND] = msecs + ms;
        }
    }
}
