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

extern u8 busy;
u8 UART_Send(u8 *data, u16 len) {
    if (busy) return 1;
    busy = 1;

    dma_channel_reset(_USART_DMA, _USART_DMA_CHANNEL);

    dma_set_peripheral_address(_USART_DMA, _USART_DMA_CHANNEL,(u32) &_USART_DR);  /* send data to the USART data register */
    dma_set_memory_address(_USART_DMA, _USART_DMA_CHANNEL, (u32) data);
    dma_set_number_of_data(_USART_DMA, _USART_DMA_CHANNEL, len);
    dma_set_read_from_memory(_USART_DMA, _USART_DMA_CHANNEL);                     /* direction is from memory to usart */
    dma_enable_memory_increment_mode(_USART_DMA, _USART_DMA_CHANNEL);             /* memory pointer increments, peripheral no */
    dma_set_peripheral_size(_USART_DMA, _USART_DMA_CHANNEL, DMA_CCR_PSIZE_8BIT);  /* USART_DR is 8bit wide in this mode */
    dma_set_memory_size(_USART_DMA, _USART_DMA_CHANNEL, DMA_CCR_MSIZE_8BIT);      /* destination memory is also 8 bit wide */
    dma_set_priority(_USART_DMA, _USART_DMA_CHANNEL, DMA_CCR_PL_LOW);
    dma_enable_transfer_complete_interrupt(_USART_DMA, _USART_DMA_CHANNEL);

    dma_enable_channel(_USART_DMA, _USART_DMA_CHANNEL);    /* dma ready to go */
    usart_enable_tx_dma(_USART);

    return 0;
}
