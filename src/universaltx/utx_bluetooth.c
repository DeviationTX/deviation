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
u8 state;

static inline void BT_ResetPtr()
{
    ptr = bt_buf;
}

void BT_Initialize()
{
    /* Enable clocks for GPIO port A (for GPIO_USART1_TX) and USART1. */
    rcc_periph_clock_enable(RCC_GPIOA);
#ifdef DISCOVERY
    rcc_periph_clock_enable(RCC_USART3);
    rcc_periph_clock_enable(RCC_GPIOC);
    nvic_enable_irq(NVIC_USART3_4_IRQ);
#else
    rcc_periph_clock_enable(RCC_USART2);
    nvic_enable_irq(NVIC_USART2_IRQ);
#endif

    PORT_mode_setup(BT_STATE, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN);
    PORT_mode_setup(BT_KEY, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE);
    PORT_pin_clear(BT_KEY);

    /* Setup GPIO pin for Tx/Rx */
    PORT_mode_setup(BT_TX, GPIO_MODE_AF, GPIO_PUPD_NONE);
    PORT_mode_setup(BT_RX, GPIO_MODE_AF, GPIO_PUPD_NONE);
    gpio_set_output_options(BT_RX.port, GPIO_OTYPE_OD, GPIO_OSPEED_25MHZ, BT_RX.pin);
    gpio_set_af(BT_TX.port, GPIO_AF1, BT_TX.pin | BT_RX.pin);

    /* Setup UART parameters. */
    usart_set_baudrate(BTUART, 9600);
    usart_set_databits(BTUART, 8);
    usart_set_stopbits(BTUART, USART_CR2_STOP_1_0BIT);
    usart_set_mode(BTUART, USART_MODE_TX_RX);
    usart_set_parity(BTUART, USART_PARITY_NONE);
    usart_set_flow_control(BTUART, USART_FLOWCONTROL_NONE);
    usart_enable_rx_interrupt(BTUART);

    BT_ResetPtr();
    state = 0;
    /* Finally enable the USART. */
    usart_enable(BTUART);
}

void BT_Test()
{
    usleep(1000000);
    PORT_pin_set(BT_KEY);
    usleep(100000);
    BT_ResetPtr();
    usart_send_blocking(BTUART, 'A');
    usart_send_blocking(BTUART, 'T');
    usart_send_blocking(BTUART, '\r');
    usart_send_blocking(BTUART, '\n');
    while(1) {
        char tmp[32];
        while(ptr == bt_buf || *(ptr-1) != '\n') {
        }
        int len = ptr - bt_buf;
        memcpy(tmp, (char *)bt_buf, len);
        BT_ResetPtr();
        tmp[len] = 0;
        printf("%s", tmp);
        if((tmp[0] == 'O' && tmp[1] == 'K') || (tmp[0] == 'F' && tmp[1] == 'A')) {
            printf("CmdDone\n");
            break;
        }
    }
    PORT_pin_clear(BT_KEY);
    usleep(150000);
}

void BT_HandleInput()
{
    u8 newstate = PORT_pin_get(BT_STATE);
    char tmp[sizeof(bt_buf)];

    if (newstate != state) {
        state = newstate;
        printf("Changed state to: %d\n", state);
    }
    if(state && ptr != bt_buf && *(ptr-1) == '\r') {
        int len = ptr - bt_buf;
        memcpy(tmp, (char *)bt_buf, len);
        tmp[len] = 0;
        BT_ResetPtr();
        if(strcmp(tmp, "PROTOCOL\r") == 0) {
            printf("PROTOCOL Cmd\n");
        }
    }
}

void BT_ISR(void)
{
    if (((USART_CR1(BTUART) & USART_CR1_RXNEIE) != 0) &&
        ((USART_ISR(BTUART) & USART_ISR_RXNE) != 0))
    {
        /* Retrieve the data from the peripheral. */
        *ptr = usart_recv(BTUART);
        if ((ptr - bt_buf) <  (int)sizeof(bt_buf) -1) {
            ptr++;
        }
        //printf("%c", *ptr);
    }
}
