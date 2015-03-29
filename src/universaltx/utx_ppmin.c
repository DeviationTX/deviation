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
#include "config/model.h"
#include "ports.h"

#define PPMIn_prescaler 23    // 48MHz /(23+1) = 2MHz = 0.5uSecond
#define PPMIn_period 65535  // max value of u16
#define MIN_PPMin_Sync 6600   // 3300uSecond=  0.5uSecond(2MHz)*6600times,  TIM1_prescaler=0.5uSecond

/*
(1) use TIM1 : set the unit same as "ppmout.c" for count the ppm-input signal, "uSecond (48MHz / 24) = 2MHz = 0.5uSecond"
(2) set GPIO PA.0 input mode as external trigger 
(3) set GPIO PA.0 into source(EXTI0) interrupt(NVIC_EXTI_1_IRQ) to trigger-call "exti0_1_isr()" function
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
    /* Enable TIM1 clock. */
    rcc_periph_clock_enable(RCC_TIM1);
  
    /* Now Enable TIM1 interrupt. */
    // nvic_enable_irq(NVIC_TIM1_IRQ);
    // nvic_set_priority(NVIC_TIM1_IRQ, 16); //High priority

    /* Reset TIM1 peripheral. */
    timer_disable_counter(TIM1);
    timer_reset(TIM1);

    /* Timer global mode:
     * - No divider
     * - Alignment edge
     * - Direction up
     */
    timer_set_mode(TIM1, TIM_CR1_CKD_CK_INT,
               TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);

       /* Reset prescaler value.  timer updates each uSecond */
    timer_set_prescaler(TIM1, PPMIn_prescaler);     // uSecond (72MHz / (35+1) = 2MHz = 0.5uSecond
    timer_set_period(TIM1, PPMIn_period);           // 3300uSecond= 2MHz*6600times,  TIM1_prescaler=0.5uSecond

    /* Disable preload. */
    timer_disable_preload(TIM1);

    /* Continous mode. */
    timer_continuous_mode(TIM1);

    /* Disable outputs. */
    timer_disable_oc_output(TIM1, TIM_OC1);
    timer_disable_oc_output(TIM1, TIM_OC2);
    timer_disable_oc_output(TIM1, TIM_OC3);
    timer_disable_oc_output(TIM1, TIM_OC4);

    /* -- OC1 configuration -- */
    /* Configure global mode of line 1. */
    /* Enable CCP1 */
    timer_disable_oc_clear(TIM1, TIM_OC1);
    timer_disable_oc_preload(TIM1, TIM_OC1);
    timer_set_oc_slow_mode(TIM1, TIM_OC1);
    timer_set_oc_mode(TIM1, TIM_OC1, TIM_OCM_FROZEN);

    /* Enable commutation interrupt. */
    //  timer_enable_irq(TIM1, TIM_DIER_CC1IE);
    /* Disable CCP1 interrupt. */
    timer_disable_irq(TIM1, TIM_DIER_CC1IE);

    /* Counter enable. */
    timer_disable_counter(TIM1);
}

void PPMin_Init()
{
    UART_Stop();  // disable USART1 for GPIO PA9 & PA10 (Trainer Tx(PA9) & Rx(PA10))

    /* Enable GPIOA clock. */
    rcc_periph_clock_enable(RCC_GPIOA);

    /* Enable EXTI interrupt. */
    nvic_set_priority(NVIC_EXTI0_1_IRQ, 2 << 6);
    nvic_enable_irq(NVIC_EXTI0_1_IRQ);

    /* Set GPIO0 (in GPIO port A) to 'input float'. */
    PORT_mode_setup(PPM, GPIO_MODE_INPUT, GPIO_PUPD_NONE);
    
    /* Configure the EXTI subsystem. */
    exti_select_source(EXTI0, GPIOA);
    exti_set_trigger(EXTI0, EXTI_TRIGGER_RISING);
    exti_disable_request(EXTI0);
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
volatile s32 Channels[MAX_PPM_IN_CHANNELS];    //  [0...ppmin_num_channels-1] for each channels width, [ppmin_num_channels] for sync-signal width
volatile u8 ppmin_num_channels;     //  the ppmin_num_channels for mixer.c 

static u8 k[4];
static u8 j = 0;
static u8 i = 0;
static u16 t0 = 0;

void exti0_1_isr(void)
{
    u16 t1 = 0;
    u16 t = 0;
    
    t1 = timer_get_counter(TIM1);     // get the counter(TIM1) value    
    exti_reset_request(EXTI0);        //  reset(clean) the IRQ
    
    t = (t1>=t0) ? (t1-t0) : (65536+t1-t0);     // none-stop TIM1 counter, compute ppm-signal width (2MHz = 0.5uSecond)
    t0 = t1;
    
    if (!ppmSync) {      // ppm-in status : not Sync
        /*  (1) capture  pmSync (for ppm-timing > MIN_PPMin_Sync : 3300uSecond)  */
        if (t>MIN_PPMin_Sync) {    // ppm-input Sync-signal
            if (j<3) {             // set 3-times for count total channels number, k[0], k[1], k[2]
                j++;               // set for next count ppm-in total channels number
                k[j] = 0;          // initial ppm-in total channels number =0
            } else {               // accumulate 3-times total channels number k[0], k[1], k[2]
        /*  (2) count channels and set to  "ppmin_num_channels"  */
                j = 0;                          // initial ppm-in Sync counter=0 (or missed signal)
                k[0] = 0;                       // set ppm-in signal beginning k[0]=0, ignore the first count total channels number
                if (k[1]>1 && k[1]==k[2]) {     // compare total channels number k[1], k[2]
                    ppmin_num_channels = k[1];  // save number of channels found
                    ppmSync = 1;                // in-sync
                    for(int m = ppmin_num_channels; m < MAX_PPM_IN_CHANNELS; m++) {
                        Channels[m] = 0;
                    }
                    i = 0;
                }
            }
        } else {             // t<MIN_PPMin_Sync,  ppm-input each Channel-signal
            k[j]++;          // conut 3-times for total channels number, k[0], k[1], k[2].
                             // ignore the first count total channels number k[0]
        }
    } else {                // ppm-in status : Sync, 
        /*  (3) get  each channel value and set to  "Channel[i]" ,
                [0...ppmin_num_channels-1] for each Channel-signal */
        int ch = (t - (Model.ppmin_centerpw * 2))*10000 / (Model.ppmin_deltapw * 2);  //Convert input to channel value
        Channels[i] = ch;
        i++;                           // set for next count  ppm-signal width
        /*  (4) continue count channels and compare  "num_channels",
                if not equal => disconnect(no-Sync) and re-connect (re-Sync) */
        if (t>MIN_PPMin_Sync) {                   // Got the ppm-input Sync-signal 
            if ((i-1) != ppmin_num_channels) {    // Trainer disconnect (coach-trainee disconnect or noise)
                ppmSync = 0;                      // set ppm-in status to "Not Sync"
            }
            i = 0;                                // initial counter for capture next period
        }
    }
}

void PPMin_Stop()
{
    nvic_disable_irq(NVIC_EXTI0_1_IRQ);
    exti_disable_request(EXTI0);
    timer_disable_counter(TIM1);
    ppmSync = 0;
}

void PPMin_Start()
{
    PPMin_Init();
    ppmSync = 0;
    memset((void *)Channels, 0, sizeof(Channels));
    timer_enable_counter(TIM1);
    nvic_enable_irq(NVIC_EXTI0_1_IRQ);
    exti_enable_request(EXTI0);
    ppmSync = 0;
}
