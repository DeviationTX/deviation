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

#include <libopencm3/cm3/nvic.h>

#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/iwdg.h>

#include "common.h"
#include "devo.h"
#include "target/drivers/mcu/stm32/tim.h"
#include "target/drivers/mcu/stm32/exti.h"
//#include "interface.h"
#include "mixer.h"
#include "config/model.h"

#define MIN_PPMin_Sync 6600   // 3300uSecond=  0.5uSecond(2MHz)*6600times,  TIM_prescaler=0.5uSecond

extern volatile u8 ppmSync;     //  the ppmSync for mixer.c,  0:ppm-Not-Sync , 1:ppm-Got-Sync
extern volatile s32 ppmChannels[MAX_PPM_IN_CHANNELS];    //  [0...ppmin_num_channels-1] for each channels width, [ppmin_num_channels] for sync-signal width
extern volatile u8 ppmin_num_channels;     //  the ppmin_num_channels for mixer.c

static u8 k[4];
static u8 j = 0;
static u8 i = 0;
static u16 t0 = 0;

void __attribute__((__used__)) exti9_5_isr(void)
{
    u16 t1 = 0;
    u16 t = 0;

    t1 = timer_get_counter(PWM_TIMER.tim);     // get the counter value
    exti_reset_request(EXTIx(PWM_TIMER.pin));         //  reset(clean) the IRQ

    t = (t1 >= t0) ? (t1-t0) : (65536+t1-t0);     // none-stop counter, compute ppm-signal width (2MHz = 0.5uSecond)
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
        ppmChannels[i] = ch;
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
