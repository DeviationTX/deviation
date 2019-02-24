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
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/cm3/nvic.h>

#include "common.h"
#ifndef DISABLE_PWM
#include "target/drivers/mcu/stm32/dma.h"
#include "target/drivers/mcu/stm32/nvic.h"

void __attribute__((__used__)) _PWM_DMA_ISR(void)
{
    timer_disable_counter(PWM_TIMER.tim);
    nvic_disable_irq(get_nvic_dma_irq(PWM_DMA));
    DMA_IFCR(PWM_DMA.dma) |= DMA_IFCR_CTCIF(PWM_DMA.stream);
    dma_disable_transfer_complete_interrupt(PWM_DMA.dma, PWM_DMA.stream);
    DMA_disable_stream(PWM_DMA);
}

#endif
