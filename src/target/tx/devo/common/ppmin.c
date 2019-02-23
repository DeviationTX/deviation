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

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/exti.h>

#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/iwdg.h>
#include "common.h"
#include "devo.h"
#include "target/drivers/mcu/stm32/rcc.h"
#include "target/drivers/mcu/stm32/tim.h"
#include "target/drivers/mcu/stm32/exti.h"
#include "target/drivers/mcu/stm32/nvic.h"
//#include "interface.h"
#include "mixer.h"
#include "config/model.h"

#define PPMIn_prescaler (TIM_FREQ_MHz(PWM_TIMER.tim) / 2 - 1)  // 72MHz / (35+1) = 2MHz = 0.5uSecond
#define PPMIn_period 65535                                     // max value of u16

/*
(1) use TIMx : set the unit same as "ppmout.c" for count the ppm-input signal, "uSecond (72MHz / 36) = 2MHz = 0.5uSecond"
(2) set GPIO PA10 input mode as external trigger 
(3) set GPIO PA10 into source(EXTI10) interrupt(NVIC_EXTI15_10_IRQ) to trigger-call "exti15_10_isr()" function
(4) transfer value for mixer.c   
    (4.1) "ppmin_num_channels"
    (4.2) "Channels[i]" or "raw[i+1]" : each channel value (volatile s32 Channels[NUM_OUT_CHANNELS];)
          Channels[i]
                      = ((ppmChannels[i]*2MHz:uSecond - 1.5mSecond)/(1.0mSecond))*(CHAN_MAX_VALUE-CHAN_MIN_VALUE)
                      = (ppmChannels[i]-3000)*10
    (4.3) "ppmSync"
*/

void PPMin_TIM_Init()
{
    /* Enable TIMx clock. */
    rcc_periph_clock_enable(get_rcc_from_port(PWM_TIMER.tim));
  
    /* No Enable TIM interrupt. */
    // nvic_enable_irq(NVIC_TIMx_IRQ(PWM_TIMER.tim));
    // nvic_set_priority(NVIC_TIMx_IRQ(PWM_TIMER.tim), 16); //High priority

    /* Reset TIM peripheral. */
    timer_disable_counter(PWM_TIMER.tim);
    rcc_periph_reset_pulse(RST_TIMx(PWM_TIMER.tim));

    /* Timer global mode:
     * - No divider
     * - Alignment edge
     * - Direction up
     */
    timer_set_mode(PWM_TIMER.tim, TIM_CR1_CKD_CK_INT,
               TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);

       /* Reset prescaler value.  timer updates each uSecond */
    timer_set_prescaler(PWM_TIMER.tim, PPMIn_prescaler);     // uSecond (72MHz / (35+1) = 2MHz = 0.5uSecond
    timer_set_period(PWM_TIMER.tim, PPMIn_period);           // 3300uSecond= 2MHz*6600times,  TIM_prescaler=0.5uSecond

    /* Disable preload. */
    timer_disable_preload(PWM_TIMER.tim);

    /* Continous mode. */
    timer_continuous_mode(PWM_TIMER.tim);

    /* Disable outputs. */
    timer_disable_oc_output(PWM_TIMER.tim, TIM_OC1);
    timer_disable_oc_output(PWM_TIMER.tim, TIM_OC2);
    timer_disable_oc_output(PWM_TIMER.tim, TIM_OC3);
    timer_disable_oc_output(PWM_TIMER.tim, TIM_OC4);

    /* -- OC1 configuration -- */
    /* Configure global mode of line 1. */
    /* Enable CCP1 */
    timer_disable_oc_clear(PWM_TIMER.tim, TIM_OC1);
    timer_disable_oc_preload(PWM_TIMER.tim, TIM_OC1);
    timer_set_oc_slow_mode(PWM_TIMER.tim, TIM_OC1);
    timer_set_oc_mode(PWM_TIMER.tim, TIM_OC1, TIM_OCM_FROZEN);

    /* Enable commutation interrupt. */
    //  timer_enable_irq(PWM_TIMER.tim, TIM_DIER_CC1IE);
    /* Disable CCP1 interrupt. */
    timer_disable_irq(PWM_TIMER.tim, TIM_DIER_CC1IE);

    /* Counter enable. */
    timer_disable_counter(PWM_TIMER.tim);
}

void PPMin_Init()
{
    if (PWM_TIMER.pin.pin == GPIO_USART1_TX) {
        UART_Stop();  // disable USART1 for GPIO PA9 & PA10 (Trainer Tx(PA9) & Rx(PA10))
    }
    rcc_periph_clock_enable(get_rcc_from_port(PWM_TIMER.tim));
    rcc_periph_clock_enable(get_rcc_from_pin(PWM_TIMER.pin));
    rcc_periph_clock_enable(RCC_AFIO);

    /* Enable EXTI interrupt. */
    nvic_enable_irq(NVIC_EXTIx_IRQ(PWM_TIMER.pin));

    /* Set GPIO0 (in GPIO port A) to 'input float'. */
    GPIO_setup_input_af(PWM_TIMER.pin, ITYPE_FLOAT, PWM_TIMER.tim);
    
    /* Configure the EXTI subsystem. */
    exti_select_source(EXTIx(PWM_TIMER.pin), PWM_TIMER.pin.port);
    exti_set_trigger(EXTIx(PWM_TIMER.pin), EXTI_TRIGGER_RISING);
    exti_disable_request(EXTIx(PWM_TIMER.pin));
}
/* ===get PPM===
(1) capture  ppmSync (for ppm-timing > MIN_PPMin_Sync : 3300uSecond)
(2) count channels and set to  "ppmin_num_channels"  (include Syn-singal)
(3) get  each channel value and set to  "Channels[i]" or "raw[i+1]"
            "Channels[i]" or "raw[i+1]"
                    = ((ppmChannels[i]*2MHz:uSecond - 1.5mSecond)/(1.0mSecond))*(CHAN_MAX_VALUE-CHAN_MIN_VALUE)
                    = (ppmChannels[i]-3000)*10
(4) continue count channels and compare  "num_channels",  if not equal => disconnect(no-Sync) and re-connect (re-Sync)
===get PPM===*/

volatile u8 ppmSync = 0;     //  the ppmSync for mixer.c,  0:ppm-Not-Sync , 1:ppm-Got-Sync
volatile s32 ppmChannels[MAX_PPM_IN_CHANNELS];    //  [0...ppmin_num_channels-1] for each channels width, [ppmin_num_channels] for sync-signal width
volatile u8 ppmin_num_channels;     //  the ppmin_num_channels for mixer.c

void PPMin_Stop()
{
    nvic_disable_irq(NVIC_EXTIx_IRQ(PWM_TIMER.pin));
    exti_disable_request(EXTIx(PWM_TIMER.pin));
    timer_disable_counter(PWM_TIMER.tim);
    ppmSync = 0;
}

void PPMin_Start()
{
    if (Model.protocol == PROTOCOL_PPM)
        CLOCK_StopTimer();
    PPMin_Init();
    ppmSync = 0;
    timer_enable_counter(PWM_TIMER.tim);
    nvic_enable_irq(NVIC_EXTIx_IRQ(PWM_TIMER.pin));
    exti_enable_request(EXTIx(PWM_TIMER.pin));
    ppmSync = 0;
}
