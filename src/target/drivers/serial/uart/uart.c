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
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/cm3/nvic.h>
#include "common.h"
#include "target/drivers/mcu/stm32/rcc.h"
#include "target/drivers/mcu/stm32/dma.h"
#include "target/drivers/mcu/stm32/nvic.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifdef MODULAR
  #pragma long_calls
#endif

void UART_Initialize()
{
    /* Enable clocks for GPIO port containing _USART and USART */
    rcc_periph_clock_enable(get_rcc_from_port(UART_CFG.uart));
    rcc_periph_clock_enable(get_rcc_from_pin(UART_CFG.rx));
    rcc_periph_clock_enable(get_rcc_from_pin(UART_CFG.tx));

    /* Enable DMA clock */
    rcc_periph_clock_enable(get_rcc_from_port(USART_DMA.dma));

    /* Setup GPIO pin GPIO_USARTX_TX on USART GPIO port for transmit.
       Set normal function to input as this is mode reverted to in half-duplex receive */
    GPIO_setup_input(UART_CFG.rx, ITYPE_FLOAT);
    GPIO_setup_output_af(UART_CFG.tx, OTYPE_PUSHPULL, UART_CFG.uart);

    /* Setup UART parameters. */
    UART_SetDataRate(0);
    UART_SetFormat(8, UART_PARITY_NONE, UART_STOPBITS_1);
    UART_SetDuplex(UART_DUPLEX_FULL);
    usart_set_flow_control(UART_CFG.uart, USART_FLOWCONTROL_NONE);
    usart_set_mode(UART_CFG.uart, USART_MODE_TX_RX);

    /* Finally enable the USART. */
    usart_enable(UART_CFG.uart);

    nvic_set_priority(get_nvic_dma_irq(USART_DMA), 3);
    nvic_enable_irq(get_nvic_dma_irq(USART_DMA));

#if HAS_AUDIO_UART
    /* Enable clocks for GPIO port C (for GPIO_UART5_TX) and UART5. */
    rcc_periph_clock_enable(get_rcc_from_port(AUDIO_UART_CFG.uart));
    rcc_periph_clock_enable(get_rcc_from_pin(AUDIO_UART_CFG.tx));

    /* Setup GPIO pins to use UART5 */
    GPIO_setup_output_af(AUDIO_UART_CFG.tx, OTYPE_PUSHPULL, AUDIO_UART_CFG.uart);

    /* Setup UART5 parameters. */
    usart_set_baudrate(AUDIO_UART_CFG.uart, 9600);
    usart_set_databits(AUDIO_UART_CFG.uart, 8);
    usart_set_stopbits(AUDIO_UART_CFG.uart, USART_STOPBITS_1);
    usart_set_parity(AUDIO_UART_CFG.uart, USART_PARITY_NONE);
    usart_set_mode(AUDIO_UART_CFG.uart, USART_MODE_TX);

    /* Finally enable the AUDIO_UART_CFG.uart. */
    usart_enable(AUDIO_UART_CFG.uart);
#endif
}

void UART_Stop()
{
    UART_StopReceive();
    nvic_disable_irq(get_nvic_dma_irq(USART_DMA));
    usart_set_mode(UART_CFG.uart, 0);
    usart_disable(UART_CFG.uart);
    rcc_periph_clock_disable(get_rcc_from_port(UART_CFG.uart));
    GPIO_setup_input(UART_CFG.tx, ITYPE_FLOAT);
}

void UART_SetDataRate(u32 bps)
{
    if (bps == 0) bps = 115200;
    if (bps > 2000000) bps = 2000000;

    usart_set_baudrate(UART_CFG.uart, bps);
}

void UART_SetFormat(int bits, uart_parity parity, uart_stopbits stopbits)
{
    // STM counts parity bit as part of databits length
    // so bits and parity settings are interdependent.
    if (bits == 8) {
        if (parity == UART_PARITY_NONE) {
            usart_set_databits(UART_CFG.uart, 8);
            usart_set_parity(UART_CFG.uart, USART_PARITY_NONE);
        } else {
            usart_set_databits(UART_CFG.uart, 9);
            if (parity == UART_PARITY_EVEN)
                usart_set_parity(UART_CFG.uart, USART_PARITY_EVEN);
            else
                usart_set_parity(UART_CFG.uart, USART_PARITY_ODD);
        }
    } else if (bits == 7 && parity != UART_PARITY_NONE) {
        usart_set_databits(UART_CFG.uart, 8);
        if (parity == UART_PARITY_EVEN)
            usart_set_parity(UART_CFG.uart, USART_PARITY_EVEN);
        else
            usart_set_parity(UART_CFG.uart, USART_PARITY_ODD);
    }

    switch (stopbits) {
    case UART_STOPBITS_1:   usart_set_stopbits(UART_CFG.uart, USART_STOPBITS_1); break;
    case UART_STOPBITS_1_5: usart_set_stopbits(UART_CFG.uart, USART_STOPBITS_1_5); break;
    case UART_STOPBITS_2:   usart_set_stopbits(UART_CFG.uart, USART_STOPBITS_2); break;
    }
}

volatile u8 busy;
u8 UART_Send(u8 *data, u16 len) {
    if (busy) return 1;
    busy = 1;

    DMA_stream_reset(USART_DMA);

    dma_set_peripheral_address(USART_DMA.dma, USART_DMA.stream, (u32) &USART_DR(UART_CFG.uart));  /* send data to the USART data register */
    dma_set_memory_address(USART_DMA.dma, USART_DMA.stream, (u32) data);
    dma_set_number_of_data(USART_DMA.dma, USART_DMA.stream, len);
    dma_set_read_from_memory(USART_DMA.dma, USART_DMA.stream);                     /* direction is from memory to usart */
    dma_enable_memory_increment_mode(USART_DMA.dma, USART_DMA.stream);             /* memory pointer increments, peripheral no */
    dma_set_peripheral_size(USART_DMA.dma, USART_DMA.stream, DMA_SxCR_PSIZE_8BIT);  /* USART_DR is 8bit wide in this mode */
    dma_set_memory_size(USART_DMA.dma, USART_DMA.stream, DMA_SxCR_MSIZE_8BIT);      /* destination memory is also 8 bit wide */
    dma_set_priority(USART_DMA.dma, USART_DMA.stream, DMA_CCR_PL_LOW);
    dma_enable_transfer_complete_interrupt(USART_DMA.dma, USART_DMA.stream);
    DMA_channel_select(USART_DMA);

    DMA_enable_stream(USART_DMA);    /* dma ready to go */
    usart_enable_tx_dma(UART_CFG.uart);

    return 0;
}

usart_callback_t *rx_callback;

/* Enable serial data reception by providing a callback function
   to accept received character and status. Disable by calling with argument NULL.
   Callback executes in interrupt context and must be short.
*/
void UART_StartReceive(usart_callback_t *isr_callback)
{
    rx_callback = isr_callback;

    if (isr_callback) {
        nvic_enable_irq(get_nvic_irq(UART_CFG.uart));
        usart_enable_rx_interrupt(UART_CFG.uart);
    } else {
        usart_disable_rx_interrupt(UART_CFG.uart);
        nvic_disable_irq(get_nvic_irq(UART_CFG.uart));
    }
}

void UART_StopReceive()
{
    UART_StartReceive(NULL);
}

void UART_SetDuplex(uart_duplex duplex)
{
    // no libopencm3 function for duplex
    if (duplex == UART_DUPLEX_FULL)
        USART_CR3(UART_CFG.uart) &= ~USART_CR3_HDSEL;
    else
        USART_CR3(UART_CFG.uart) |= USART_CR3_HDSEL;
}

void UART_SendByte(u8 x)
{
    usart_send_blocking(UART_CFG.uart, x);
}
