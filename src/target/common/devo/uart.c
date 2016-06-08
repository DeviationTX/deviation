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
#include "common.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

void UART_Initialize()
{
    /* Enable clocks for GPIO port A (for GPIO_USART1_TX) and USART1. */
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN);
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_USART1EN);

    /* Setup GPIO pins to use USART1 */
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
                    GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,
                  GPIO_USART1_RX);

    /* Setup UART parameters. */
#ifdef BUILDTYPE_DEV
    usart_set_baudrate(USART1, 115200);
#else
    usart_set_baudrate(USART1, 9600);
#endif
    usart_set_databits(USART1, 8);
    usart_set_stopbits(USART1, USART_STOPBITS_1);
    usart_set_parity(USART1, USART_PARITY_NONE);
    usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
    usart_set_mode(USART1, USART_MODE_TX_RX);

    /* Finally enable the USART. */
    usart_enable(USART1);

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
    usart_disable(USART1);
#ifdef AUDIO_UART5
    usart_disable(UART5);
#endif
}
