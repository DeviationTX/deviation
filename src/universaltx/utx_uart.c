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
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_USART4);

    /* Setup GPIO pin PA.0 for transmit. */
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO0);
    gpio_set_af(GPIOA, GPIO_AF4, GPIO0); // AF4 ==> USART4

    /* Setup UART parameters. */
    usart_set_baudrate(USART4, 115200);
    usart_set_databits(USART4, 8);
    usart_set_stopbits(USART4, USART_CR2_STOP_1_0BIT);
    usart_set_mode(USART4, USART_MODE_TX);
    usart_set_parity(USART4, USART_PARITY_NONE);
    usart_set_flow_control(USART4, USART_FLOWCONTROL_NONE);

    /* Finally enable the USART. */
    usart_enable(USART4);
}

void UART_Stop()
{
    usart_disable(USART4);
}
