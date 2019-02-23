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

#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/dac.h>
#include <libopencm3/stm32/i2c.h>

#include "common.h"
#include "target/common/stm32/nvic.h"
#include "target/common/stm32/dma.h"
#include "target/common/stm32/rcc.h"

void dma1_stream5_isr(void)
{
    PORT_pin_toggle(LED_RED_PIN);  // FIXME
    // Do not load data into interrupt handler
    if (load_audio == AUDIO_DONE) {
        // All done
        dma_clear_interrupt_flags(DMA1, DMA_STREAM5, DMA_HTIF);
        dma_clear_interrupt_flags(DMA1, DMA_STREAM5, DMA_TCIF);
        dma_disable_stream(AUDIODAC_DMA.dma, AUDIODAC_DMA.stream);
        dac_disable(CHANNEL_1);
        timer_disable_counter(AUDIODAC_TIM.tim);
        return;
    }
    if (dma_get_interrupt_flag(DMA1, DMA_STREAM5, DMA_HTIF)) {
        load_audio = AUDIO_FIRST_HALF;
        dma_clear_interrupt_flags(DMA1, DMA_STREAM5, DMA_HTIF);
    } else {
        load_audio = AUDIO_SECOND_HALF;
        dma_clear_interrupt_flags(DMA1, DMA_STREAM5, DMA_TCIF);
    }
}

