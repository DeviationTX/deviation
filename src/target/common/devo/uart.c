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

#include <stdio.h>
#include <string.h>
#include <errno.h>

void UART_Initialize()
{
    /* Enable clocks for GPIO port A (for GPIO_USART1_TX) and USART1. */
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN);
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_USART1EN);

    /* Setup GPIO pin GPIO_USART1_TX/GPIO9 on GPIO port A for transmit. */
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
                    GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);

    /* Setup UART parameters. */
    usart_set_baudrate(USART1, 115200);
    usart_set_databits(USART1, 8);
    usart_set_stopbits(USART1, USART_STOPBITS_1);
    usart_set_mode(USART1, USART_MODE_TX);
    usart_set_parity(USART1, USART_PARITY_NONE);
    usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);

    /* Finally enable the USART. */
    usart_enable(USART1);

    nvic_set_priority(_USART_NVIC_DMA_CHANNEL_IRQ, 3);
    nvic_enable_irq(_USART_NVIC_DMA_CHANNEL_IRQ);
}

void UART_Stop()
{
    usart_disable(USART1);
}

static volatile u8 busy;
u8 UART_Send(u8 *data, u16 len) {
    if (busy) return 1;
    busy = 1;

    /* Enable DMA clock */
    // TODO ENABLED ALREADY FOR ADC? rcc_peripheral_enable_clock(&RCC_AHBENR, _RCC_AHBENR_DMAEN);

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

void _USART_DMA_ISR(void)
{
    DMA_IFCR(_USART_DMA) |= DMA_IFCR_CTCIF(_USART_DMA_CHANNEL);

    dma_disable_transfer_complete_interrupt(_USART_DMA, _USART_DMA_CHANNEL);
    usart_disable_tx_dma(_USART);
    dma_disable_channel(_USART_DMA, _USART_DMA_CHANNEL);

    busy = 0;
}

