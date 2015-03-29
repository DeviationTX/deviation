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

#define dbgprintf if(0)printf
#define btprintf printf
char bt_buf[30];
volatile char *wptr;
char *rptr = bt_buf;
u8 state;
/* FIXME */
const char * const RADIO_TX_POWER_VAL[TXPOWER_LAST] =
     { "100uW", "300uW", "1mW", "3mW", "10mW", "30mW", "100mW", "150mW" };

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

    wptr = bt_buf;
    rptr = bt_buf;
    state = 0;
    /* Finally enable the USART. */
    usart_enable(BTUART);
}

/*
void BT_Test()
{
    usleep(1000000);
    PORT_pin_set(BT_KEY);
    usleep(100000);
printf("here\n");
    usart_send_blocking(BTUART, 'A');
    usart_send_blocking(BTUART, 'T');
    usart_send_blocking(BTUART, '\r');
    usart_send_blocking(BTUART, '\n');
    while(1) {
        char tmp[32];
        while(ptr == bt_buf || *(ptr-1) != '\n') {
            if(ptr != bt_buf) printf("%c", *ptr);
        }
        int len = ptr - bt_buf;
        memcpy(tmp, (char *)bt_buf, len);
        tmp[len] = 0;
        printf("%s", tmp);
        if((tmp[0] == 'O' && tmp[1] == 'K') || (tmp[0] == 'F' && tmp[1] == 'A') ) {
            printf("CmdDone\n");
            break;
        }
    }
    PORT_pin_clear(BT_KEY);
    usleep(150000);
}
*/

void parse_cmd(char *tmp)
{
    if(strncmp(tmp, "GPL", 3) == 0) { //Get Protocol List
        btprintf("GPL");
        for (int i = 0; i < PROTOCOL_COUNT; i++) {
            btprintf(":");
            btprintf(ProtocolNames[i]);
        }
        btprintf("\n");
        return;
    }
    if(strncmp(tmp, "GPO", 3) == 0) { //Get Protocol Options
        btprintf("GPO:");
        const char **proto_strs = PROTOCOL_GetOptions();
        if (proto_strs) {
            int pos = 0;
            int protoidx = 0;
            while(protoidx < NUM_PROTO_OPTS) {
                if (proto_strs[pos] == NULL)
                    break;
                btprintf("::%s", proto_strs[pos]);
                while(proto_strs[++pos]) {
                    btprintf(":%s", proto_strs[pos]);
                }
                pos++;
                protoidx++;
            }
        }
        btprintf("\n");
        return;
    }
    if(strncmp(tmp, "GPV", 3) == 0) { //Get Protocol Values
        btprintf("GPV");
        const char **proto_strs = PROTOCOL_GetOptions();
        if (! proto_strs) {
            btprintf(":\n");
            return;
        }
        int protoidx = 0;
        int pos = 0;
        while(protoidx < NUM_PROTO_OPTS) {
            if(proto_strs[pos] == NULL)
                break;
            while(proto_strs[++pos])
                ;
            pos++;
            protoidx++;
        }
        for(int i = 0; i < protoidx; i++) {
            btprintf(":%d", i);
        }
        btprintf("\n");
        return;
    }
    if(strncmp(tmp, "GTL", 3) == 0) { //Get TxPower List
        btprintf("GTL");
        for (int i = 0; i < TXPOWER_LAST; i++) {
            btprintf(":%s", RADIO_TX_POWER_VAL[i]);
        }
        btprintf("\n");
        return;
    }
    if(strncmp(tmp, "GPI", 3) == 0) { //Get Channel Info (returns max_#_channels:default_#_channels
        btprintf("GPI:%s:%d:%d\n", ProtocolNames[Model.protocol], PROTOCOL_NumChannels(), PROTOCOL_DefaultNumChannels());
        return;
    }
    if(strncmp(tmp, "GMI", 3) == 0) { //Get Number of Channels
        btprintf("GMI:%d:%d:%d:%d\n", Model.num_channels, Model.tx_power, Model.fixed_id,PROTOCOL_AutoBindEnabled());
        return;
    }
    if(strncmp(tmp, "SCP:", 4) == 0) {
        for (int i = 0; i < PROTOCOL_COUNT; i++) {
            if (strcmp(tmp+4, ProtocolNames[i]) == 0) {
                Model.protocol = i;
                dbgprintf("Changed protocol to: %s\n", ProtocolNames[i]);
                return;
            }
        }
        dbgprintf("Could not chnage protocol to: '%s'\n", tmp+4);
        return;
    }
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
                        dbgprintf("Set prototocol option '%s' to %d\n", proto_strs[pos], val);
                        return;
                    }
                    while(proto_strs[++pos])
                        ;
                    pos++;
                    protoidx++;
                }
            }
        }
        dbgprintf("Couldn't parse SPV command: %s\n", tmp+4);
        return;
    }
    if(strncmp(tmp, "STP:", 4) == 0) { //Set TxPower
        unsigned val;
        if(tmp[5] == '\r') {
            val = tmp[4] - '0';
        } else {
            dbgprintf("Unknown POWER: %s\n", tmp);
            return;
        }
        if (val < TXPOWER_LAST) {
            dbgprintf("Changed TX Power to: %s\n", RADIO_TX_POWER_VAL[val]);
            Model.tx_power = val; //RF Channel
        } else {
            dbgprintf("Requested power values is outside range: %d > %d\n", val, TXPOWER_LAST-1);
        }
        return;
    }
    if(strncmp(tmp, "SNC:", 4) == 0) { //Set Number of Channels
        char *valptr;
        unsigned idx = strtol(tmp+4, &valptr, 10);
        if (!valptr) {
            dbgprintf("Couldn't read number of channels to set\n");
            return;
        }
        if (idx > (unsigned)PROTOCOL_NumChannels()) {
            dbgprintf("Illegal channel count specified\n");
            return;
        }
        Model.num_channels = idx;
        return;
    }
    if(strncmp(tmp, "SID:", 4) == 0) { //Set Number of Channels
        char *valptr;
        unsigned id = strtol(tmp+4, &valptr, 10);
        if (!valptr) {
            dbgprintf("No Fixed ID specified\n");
            return;
        }
        if(id > 999999) {
            id = 999999;
        }
        Model.fixed_id = id;
        return;
    }
}

void BT_HandleInput()
{
    char tmp[sizeof(bt_buf)];
    char *tmpptr = tmp;

    u8 newstate = PORT_pin_get(BT_STATE);

    if (newstate != state) {
        state = newstate;
        dbgprintf("Changed state to: %d\n", state);
    }
    char *_wptr = (char *)wptr;
    if(! state || rptr == _wptr ) {
        return;
    }
    char *lastwptr = _wptr-1;
    if (lastwptr < bt_buf) {
        lastwptr = bt_buf+sizeof(bt_buf)-1;
    }
    if (*lastwptr != '\r' && *lastwptr != '\n') {
        return;
    }
    while(rptr != _wptr) {
        if (*rptr == '\r' || *rptr == '\n') {
            *tmpptr = 0;
            if (tmpptr != tmp) {
                parse_cmd(tmp);
            }
            tmpptr = tmp;
        } else {
            *tmpptr++ = *rptr;
        }
        rptr++;
        if (rptr == bt_buf+sizeof(bt_buf)) {
            rptr = bt_buf;
        }
    }
}

void BT_ISR(void)
{
    if (((USART_CR1(BTUART) & USART_CR1_RXNEIE) != 0) &&
        ((USART_ISR(BTUART) & USART_ISR_RXNE) != 0))
    {
        /* Retrieve the data from the peripheral. */
        *wptr = usart_recv(BTUART);
        if (wptr+1 == bt_buf + sizeof(bt_buf)) {
            wptr = bt_buf;
        } else {
            wptr++;
        }
        //printf("%c", *ptr);
    }
}
