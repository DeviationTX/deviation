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
#include "config/model.h"
#include "protocol/interface.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

volatile char *ptr;
volatile char bt_buf[20];
u8 state;
/* FIXME */
const char * const RADIO_TX_POWER_VAL[TXPOWER_LAST] =
     { "100uW", "300uW", "1mW", "3mW", "10mW", "30mW", "100mW", "150mW" };

static inline void BT_ResetPtr()
{
    ptr = bt_buf;
}

void BT_Initialize()
{
    /* Enable clocks for GPIO port A (for GPIO_USART1_TX) and USART1. */
    rcc_periph_clock_enable(RCC_GPIOA);
#if DISCOVERY
    rcc_periph_clock_enable(RCC_USART3);
    rcc_periph_clock_enable(RCC_GPIOC);
#else
    rcc_periph_clock_enable(RCC_USART2);

#endif
    nvic_set_priority(BT_IRQ, 3 << 6);
    nvic_enable_irq(BT_IRQ);

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
        //char *proto = ProtocolNames[Model.protocol];
        //int protolen = strlen(proto);
        //if(memcmp(tmp, proto, protolen) == 0 && tmp[protolen] ==':') {
        if(strncmp(tmp, "SPV:", 4) == 0) {
            char *valptr;
            int idx = strtol(tmp+4, &valptr, 10);
            if (valptr && valptr[0] == ':' && idx < NUM_PROTO_OPTS) {
                int val = strtol(valptr+1, &valptr, 10);
                if(valptr != NULL) {
                    const char **proto_strs = PROTOCOL_GetOptions();
                    int protoidx = 0;
                    int pos = 0;
                    while(protoidx < NUM_PROTO_OPTS) {
                        if(proto_strs[pos] == NULL)
                            break;
                        if (idx == protoidx) {
                            Model.proto_opts[idx] = val;
                            printf("Set prototocol option '%s' to %d\n", proto_strs[pos], val);
                            return;
                        }
                        while(proto_strs[++pos])
                            ;
                        pos++;
                        protoidx++;
                    }
                }
            }
            printf("Couldn't parse SPV command: %s\n", tmp+4);
            return;
        }
        if(strncmp(tmp, "STP:", 4) == 0) {
            unsigned val;
            if(tmp[5] == '\r') {
                val = tmp[4] - '0';
            } else {
                printf("Unknown POWER: %s\n", tmp);
                return;
            }
            if (val < TXPOWER_LAST) {
                printf("Changed TX Power to: %s\n", RADIO_TX_POWER_VAL[val]);
                Model.tx_power = val; //RF Channel
            } else {
                printf("Requested power values is outside range: %d > %d\n", val, TXPOWER_LAST-1);
            }
            return;
        }
        if(strncmp(tmp, "SCP:", 4) == 0) {
            tmp[len-1] = 0;
            for (int i = 0; i < PROTOCOL_COUNT; i++) {
                if (strcmp(tmp+4, ProtocolNames[i]) == 0) {
                    Model.protocol = i;
                    printf("Changed protocol to: %s\n", ProtocolNames[i]);
                    return;
                }
            }
            printf("Could not chnage protocol to: '%s'\n", tmp+4);
            return;
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
