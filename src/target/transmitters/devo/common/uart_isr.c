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
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/cm3/nvic.h>

#include "common.h"

extern volatile u8 busy;
void __attribute__((__used__)) _USART_DMA_ISR(void)
{
    DMA_IFCR(_USART_DMA) |= DMA_IFCR_CTCIF(_USART_DMA_CHANNEL);

    dma_disable_transfer_complete_interrupt(_USART_DMA, _USART_DMA_CHANNEL);
    usart_disable_tx_dma(_USART);
    dma_disable_channel(_USART_DMA, _USART_DMA_CHANNEL);

    busy = 0;
}

extern usart_callback_t *rx_callback;
void __attribute__((__used__)) _USART_ISR(void)
{
	u8 status = USART_SR(_USART) & (USART_SR_RXNE | USART_SR_PE | USART_SR_FE | USART_SR_NE | USART_SR_ORE) ;
    u8 data = usart_recv(_USART);       // read unconditionally to reset interrupt and error flags

    if (rx_callback) rx_callback(data, status);

    // interrupt cleared by reading data register
}