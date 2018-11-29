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

#include "common.h"
#ifndef DISABLE_PWM

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/cm3/nvic.h>

#include "../ports.h"


void _PWM_DMA_ISR(void)
{
    timer_disable_counter(TIM1);
    nvic_disable_irq(_PWM_NVIC_DMA_CHANNEL_IRQ);
    DMA_IFCR(_PWM_DMA) |= DMA_IFCR_CTCIF(_PWM_DMA_CHANNEL);
    dma_disable_transfer_complete_interrupt(_PWM_DMA, _PWM_DMA_CHANNEL);
    dma_disable_channel(_PWM_DMA, _PWM_DMA_CHANNEL);
}

#endif
