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
#include "devo.h"

static u16(*Callback)();

void SOUND_Init()
{
    rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_TIM2EN);
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO1);

    timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT,
                   TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    /* Period */
    timer_set_period(TIM2, 65535);

    /* Prescaler */
    timer_set_prescaler(TIM2, 5);
    timer_generate_event(TIM2, TIM_EGR_UG);

    /* ---- */
    /* Output compare 1 mode and preload */
    timer_set_oc_mode(TIM2, TIM_OC2, TIM_OCM_PWM1);
    timer_enable_oc_preload(TIM2, TIM_OC2);

    /* Polarity and state */
    timer_set_oc_polarity_low(TIM2, TIM_OC2);
    timer_enable_oc_output(TIM2, TIM_OC2);

    /* Capture compare value */
    timer_set_oc_value(TIM2, TIM_OC2, 0x8000);
    /* ---- */
    /* ARR reload enable */
    timer_enable_preload(TIM2);

    VIBRATINGMOTOR_Init(); // Since the vibrating motor is tightly controlled by sound, we put its init() here instead of in the main()
}

void SOUND_SetFrequency(u16 frequency, u8 volume)
{
    if (volume == 0) {
        //We need to keep the timer running (for the vibration motor, but also in case there is a pause in the music)
        //But don't want the buzzer running
        timer_disable_oc_output(TIM2, TIM_OC2);
    } else {
        timer_enable_oc_output(TIM2, TIM_OC2);
    }
    /* volume is between 0 and 100 */
    /* period = 14400000 / frequency */
    /* A Period of 65535 gives a ~ 220Hz tone */
    /* The Devo buzzer reaches max-volume with a pw ~ 100us.  That is max volume */
    /* use quadratic to approximate exponential volume control */
    u32 period = 14400000 / frequency;
    /* Taylor series: x + x^2/2 + x^3/6 + x^4/24 */
    u32 duty_cycle = volume == 100 ? (period >> 1) : (u32)volume * volume * volume * 12 / 10000;
    timer_set_period(TIM2, period);
    timer_set_oc_value(TIM2, TIM_OC2, duty_cycle);
}

void SOUND_Start(u16 msec, u16(*next_note_cb)())
{
    SOUND_StartWithoutVibrating(msec, next_note_cb);
    VIBRATINGMOTOR_Start();
}

void SOUND_StartWithoutVibrating(u16 msec, u16(*next_note_cb)())
{
    CLOCK_SetMsecCallback(TIMER_SOUND, msec);
    Callback = next_note_cb;
    timer_enable_counter(TIM2);
}

void SOUND_Stop()
{
    CLOCK_ClearMsecCallback(TIMER_SOUND);
    timer_disable_counter(TIM2);
    VIBRATINGMOTOR_Stop();
}

u32 SOUND_Callback()
{
    if (Callback == NULL) {  // To allow single tone
        SOUND_Stop();
        return 0;
    }
    u16 msec = Callback();
    if(! msec)
        SOUND_Stop();
    return msec;
}
