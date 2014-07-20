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
#include <libopencm3/cm3/nvic.h>
#include "common.h"
#include "ports.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

volatile char *ptr;
volatile char bt_buf[20];

void BT_Initialize()
{
    /* Enable clocks for GPIO port A (for GPIO_USART1_TX) and USART1. */
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_USART2);

    PORT_mode_setup(BT_STATE, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN);

    /* Setup GPIO pin for Tx/Rx */
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2 | GPIO3);
    gpio_set_af(GPIOA, GPIO_AF1, GPIO2 | GPIO3); // AF4 ==> USART4

    /* Setup UART parameters. */
    usart_set_baudrate(USART2, 9600);
    usart_set_databits(USART2, 8);
    usart_set_stopbits(USART2, USART_CR2_STOP_1_0BIT);
    usart_set_mode(USART2, USART_MODE_TX_RX);
    usart_set_parity(USART2, USART_PARITY_NONE);
    usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);
    usart_enable_rx_interrupt(USART2);

    ptr = bt_buf;
    nvic_enable_irq(NVIC_USART2_IRQ);
    /* Finally enable the USART. */
    usart_enable(USART2);
}

void BT_HandleInput()
{
    if(! PORT_pin_get(BT_STATE)) {
        return;
    }
    if(ptr != bt_buf) {
        if(*(ptr-1) != '\r' || *ptr != '\r') {
            return;
        }
        if((ptr - bt_buf) > (int)strlen("PROTOCOL") && strncmp((char *)bt_buf, "PROTOCOL", (ptr - bt_buf)) == 0) {
            ptr = bt_buf;
            printf("PROTOCOL\n");
        }
    }
}

void usart2_isr(void)
{
    if (((USART_CR1(USART2) & USART_CR1_RXNEIE) != 0) &&
        ((USART_ISR(USART2) & USART_ISR_RXNE) != 0))
    {
        /* Indicate that we got data. */
        gpio_toggle(GPIOD, GPIO12);

        /* Retrieve the data from the peripheral. */
        *ptr = usart_recv(USART2);
        printf("%c", *ptr);
        if ((ptr - bt_buf) <  (int)sizeof(bt_buf) -1) {
            ptr++;
        }
    }
}
