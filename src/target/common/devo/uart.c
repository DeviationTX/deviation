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
    /* Enable clocks for GPIO port containing _USART and USART */
    rcc_peripheral_enable_clock(&_USART_RCC_APB_ENR_IOP,   _USART_RCC_APB_ENR_IOP_EN);
    rcc_peripheral_enable_clock(&_USART_RCC_APB_ENR_USART, _USART_RCC_APB_ENR_USART_EN);

    /* Enable DMA clock */
    rcc_peripheral_enable_clock(&RCC_AHBENR, _RCC_AHBENR_DMAEN);

    /* Setup GPIO pin GPIO_USARTX_TX on USART GPIO port for transmit.
       Set normal function to input as this is mode reverted to in half-duplex receive */
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, _USART_GPIO_USART_RX);
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, _USART_GPIO_USART_TX);
    gpio_set_mode(_USART_GPIO, GPIO_MODE_OUTPUT_50_MHZ,
                    GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, _USART_GPIO_USART_TX);

    /* Setup UART parameters. */
    UART_SetDataRate(115200);
    UART_SetFormat(8, UART_PARITY_NONE, UART_STOPBITS_1);
    usart_set_flow_control(_USART, USART_FLOWCONTROL_NONE);
    usart_set_mode(_USART, USART_MODE_TX_RX);

    /* Finally enable the USART. */
    usart_enable(_USART);

    nvic_set_priority(_USART_NVIC_DMA_CHANNEL_IRQ, 3);
    nvic_enable_irq(_USART_NVIC_DMA_CHANNEL_IRQ);

#if HAS_AUDIO_UART5
    /* Enable clocks for GPIO port C (for GPIO_UART5_TX) and UART5. */
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPCEN);
    rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_UART5EN);

    /* Setup GPIO pins to use UART5 */
    gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_50_MHZ,
                    GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_UART5_TX);

    /* Setup UART5 parameters. */
    usart_set_baudrate(UART5, 9600);
    usart_set_databits(UART5, 8);
    usart_set_stopbits(UART5, USART_STOPBITS_1);
    usart_set_parity(UART5, USART_PARITY_NONE);
    usart_set_mode(UART5, USART_MODE_TX);

    /* Finally enable the UART5. */
    usart_enable(UART5);
#endif
}

void UART_Stop()
{
    UART_SetReceive(NULL);
    nvic_disable_irq(_USART_NVIC_DMA_CHANNEL_IRQ);
    usart_set_mode(_USART, 0);
    usart_disable(_USART);
    rcc_peripheral_disable_clock(&_USART_RCC_APB_ENR_USART, _USART_RCC_APB_ENR_USART_EN);
}

void UART_SetDataRate(u32 bps)
{
    if (bps > 2000000) bps = 2000000;

    usart_set_baudrate(_USART, bps);
}

void UART_SetFormat(int bits, uart_parity parity, uart_stopbits stopbits)
{
    // STM counts parity bit as part of databits length
    // so bits and parity settings are interdependent.
    if (bits == 8) {
        if (parity == UART_PARITY_NONE) {
            usart_set_databits(_USART, 8);
            usart_set_parity(_USART, USART_PARITY_NONE);
        } else {
            usart_set_databits(_USART, 9);
            if (parity == UART_PARITY_EVEN)
                usart_set_parity(_USART, USART_PARITY_EVEN);
            else
                usart_set_parity(_USART, USART_PARITY_ODD);
        }
    } else if (bits == 7 && parity != UART_PARITY_NONE) {
        usart_set_databits(_USART, 8);
        if (parity == UART_PARITY_EVEN)
            usart_set_parity(_USART, USART_PARITY_EVEN);
        else
            usart_set_parity(_USART, USART_PARITY_ODD);
    }

    switch (stopbits) {
    case UART_STOPBITS_1:   usart_set_stopbits(_USART, USART_STOPBITS_1); break;
    case UART_STOPBITS_1_5: usart_set_stopbits(_USART, USART_STOPBITS_1_5); break;
    case UART_STOPBITS_2:   usart_set_stopbits(_USART, USART_STOPBITS_2); break;
    }
}

static volatile u8 busy;
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

void _USART_DMA_ISR(void)
{
    DMA_IFCR(_USART_DMA) |= DMA_IFCR_CTCIF(_USART_DMA_CHANNEL);

    dma_disable_transfer_complete_interrupt(_USART_DMA, _USART_DMA_CHANNEL);
    usart_disable_tx_dma(_USART);
    dma_disable_channel(_USART_DMA, _USART_DMA_CHANNEL);

    busy = 0;
}

static usart_callback_t *rx_callback;
void _USART_ISR(void)
{
	u8 status = USART_SR(_USART) & (USART_SR_RXNE | USART_SR_PE | USART_SR_FE | USART_SR_NE | USART_SR_ORE) ;
    u8 data = usart_recv(_USART);       // read unconditionally to reset interrupt and error flags

    if (rx_callback) rx_callback(data, status);

    // interrupt cleared by reading data register
}

/* Enable serial data reception by providing a callback function
   to accept received character and status. Disable by calling with argument NULL.
   Callback executes in interrupt context and must be short.
*/
void UART_SetReceive(usart_callback_t *isr_callback)
{
    rx_callback = isr_callback;

    if (isr_callback) {
        nvic_enable_irq(_USART_NVIC_USART_IRQ);
        usart_enable_rx_interrupt(_USART);
    } else {
        usart_disable_rx_interrupt(_USART);
        nvic_disable_irq(_USART_NVIC_USART_IRQ);
    }
}


void UART_SetDuplex(uart_duplex duplex)
{
    if (duplex == UART_DUPLEX_FULL)
        USART_CR3(_USART) &= ~USART_CR3_HDSEL;
    else
        USART_CR3(_USART) |= USART_CR3_HDSEL;
}
