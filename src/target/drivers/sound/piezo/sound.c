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

#include "target/drivers/mcu/stm32/rcc.h"
#include "target/drivers/mcu/stm32/tim.h"

static u16(*Callback)();

void SOUND_Init()
{
    rcc_periph_clock_enable(get_rcc_from_port(SOUND_TIM.tim));
    GPIO_setup_output_af(SOUND_TIM.pin, OTYPE_PUSHPULL, SOUND_TIM.tim);

    timer_set_mode(SOUND_TIM.tim, TIM_CR1_CKD_CK_INT,
                   TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    /* Period */
    timer_set_period(SOUND_TIM.tim, 65535);

    /* Prescaler */
    timer_set_prescaler(SOUND_TIM.tim, TIM_FREQ_MHz(SOUND_TIM.tim) / 12 - 1);
    timer_generate_event(SOUND_TIM.tim, TIM_EGR_UG);

    /* ---- */
    /* Output compare 1 mode and preload */
    timer_set_oc_mode(SOUND_TIM.tim, TIM_OCx(SOUND_TIM.ch), TIM_OCM_PWM1);
    timer_enable_oc_preload(SOUND_TIM.tim, TIM_OCx(SOUND_TIM.ch));

    /* Polarity and state */
    timer_set_oc_polarity_low(SOUND_TIM.tim, TIM_OCx(SOUND_TIM.ch));
    // timer_enable_oc_output(SOUND_TIM.tim, TIM_OCx(SOUND_TIM.ch));

    /* Capture compare value */
    timer_set_oc_value(SOUND_TIM.tim, TIM_OCx(SOUND_TIM.ch), 0x8000);
    /* ---- */
    /* ARR reload enable */
    timer_enable_preload(SOUND_TIM.tim);

    VIBRATINGMOTOR_Init(); // Since the vibrating motor is tightly controlled by sound, we put its init() here instead of in the main()
}

void SOUND_SetFrequency(unsigned frequency, unsigned volume)
{
    if(frequency == 0) {
        volume = 0;
        frequency = 220;
    }
    if (volume == 0) {
        //We need to keep the timer running (for the vibration motor, but also in case there is a pause in the music)
        //But don't want the buzzer running
        timer_disable_oc_output(SOUND_TIM.tim, TIM_OCx(SOUND_TIM.ch));
    } else {
        timer_enable_oc_output(SOUND_TIM.tim, TIM_OCx(SOUND_TIM.ch));
    }
    /* volume is between 0 and 100 */
    /* period = 14400000 / frequency */
    /* A Period of 65535 gives a ~ 220Hz tone */
    /* The Devo buzzer reaches max-volume with a pw ~ 100us.  That is max volume */
    /* use quadratic to approximate exponential volume control */
    u32 period = 14400000 / frequency;
    /* Taylor series: x + x^2/2 + x^3/6 + x^4/24 */
    u32 duty_cycle = (period >> 1) * (u32)volume / 100 * volume / 100 * volume / 100;
    timer_set_period(SOUND_TIM.tim, period);
    timer_set_oc_value(SOUND_TIM.tim, TIM_OCx(SOUND_TIM.ch), duty_cycle);
}

void SOUND_Start(unsigned msec, u16(*next_note_cb)(), u8 vibrate)
{
    SOUND_StartWithoutVibrating(msec, next_note_cb);
    if (vibrate)
        VIBRATINGMOTOR_Start();
}

void SOUND_StartWithoutVibrating(unsigned msec, u16(*next_note_cb)())
{
    CLOCK_SetMsecCallback(TIMER_SOUND, msec);
    Callback = next_note_cb;
    timer_enable_counter(SOUND_TIM.tim);
}

void SOUND_Stop()
{
    CLOCK_ClearMsecCallback(TIMER_SOUND);
    timer_disable_counter(SOUND_TIM.tim);
    timer_disable_oc_output(SOUND_TIM.tim, TIM_OCx(SOUND_TIM.ch));
    VIBRATINGMOTOR_Stop();
}

u32 SOUND_Callback()
{
    if (Callback == NULL) {  // To allow single tone
        SOUND_Stop();
        return 0;
    }
    unsigned msec = Callback();
    if(! msec)
        SOUND_Stop();
    return msec;
}
