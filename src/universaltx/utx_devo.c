/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2011 Stephen Caudle <scaudle@doceme.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "common.h"
#include "config/model.h"
#include "config/tx.h"
#include "protocol/interface.h"
#include "ports.h"

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/dma.h>

static void spi_pkt_handler(u8 *ptr, unsigned length);
register unsigned module_port asm ("r11");
register unsigned module_pin  asm ("r10");
//Keep both of these multiples of 2
#define MAX_PKT_SIZE 16
static u8 read_buffer[MAX_PKT_SIZE];

static volatile char sendstr[256];
volatile char *sendptr = sendstr;
static volatile u8 ready;
const char map[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd'};

static void SetModule(unsigned module);

#include "_utx_pactl.c"
#include "_utx_multimod.c"

//Main routine for acting as a DEVO slave.
//Code here converts universal TX into a simple switch and Devo TX does all the work
void utx_devo()
{
    SetModule(TX_MODULE_LAST);
    sendptr = sendstr;
    ready = 0;
    printf("Initialized\n");
    SPI_ProtoInit();
    SPI_ProtoMasterSlaveInit(read_buffer);  //Switch SPI to slave mode
    while(1) {
        if(ready) {
            char data[sizeof(sendstr)];
            u8 len = sendptr - sendstr;
            memcpy(data, sendstr, sizeof(sendstr));
            sendptr = sendstr;
            ready = 0;
            data[len] = 0;
            for(int i = 0; i < len; i++) {
                if (data[i] == '_') {
                    u8 val0 = data[i+1];
                    u8 val1 = data[i+2];
                    data[i+1] = map[(val0 >> 4)];
                    data[i+2] = map[(val0 & 0x0f)];
                    data[i+3] = map[(val1 >> 4)];
                    data[i+4] = map[(val1 & 0x0f)];
                    data[i+5] = ' ';
                    i += 5;
                }
            }
            printf("%s\n", data);
        }
    }
}

void SetModule(unsigned module) {
    Model.module =  module;
    if (module >= MULTIMOD) {
        module_port = module_enable[CYRF6936].port;
        module_pin  = module_enable[CYRF6936].pin;
    } else {
        module_port = module_enable[module].port;
        module_pin  = module_enable[module].pin;
    }
}

//The following is called whenever an SPI transfer targetted at the UniversalTx is completed
//It is called in interrupt context, so keep it short!
void spi_pkt_handler(u8 *ptr, unsigned length)
{
    ready = 1;
    //*sendptr++ = '0' + length;
    int module = TX_MODULE_LAST;
    if (length < 1)
        return;
    //*sendptr++ = '-';
    //*sendptr++ = map[ptr[0]>>4];
    //*sendptr++ = map[ptr[0]& 0x0f];
    int command = ptr[0];
    if (command == CHANGE_MODULE) {
        if (length != 2)
            return;
        module = ptr[1];
        SetModule(module);
        //*sendptr++ = '0' + module;
    }
    MULTIMOD_SwitchCommand(module, command);
}

//This interrupt will fire if either INPUT_CSN (PA.8) or PASSTHRU_CSN (PB.12) change state
void exti4_15_isr(void)
{
    if (PORT_pin_get_fast(PASSTHRU_CSN))
    {
        GPIO_BSRR(module_port) = module_pin;
    } else {
        GPIO_BRR(module_port) = module_pin;
    }
    EXTI_PR = PASSTHRU_CSN.pin;
    if (EXTI_PR & INPUT_CSN.pin) {
        if(PORT_pin_get_fast(INPUT_CSN)) {
           //*sendptr++ = 'Y';
           SET_BIT(SPI_CR1(SPI2), SPI_CR1_SSI);                //NSS = 1
           u16 len = DMA_CNDTR(DMA1, DMA_CHANNEL4);
           CLEAR_BIT(DMA_CCR(DMA1, DMA_CHANNEL4), DMA_CCR_EN); //Disable DMA
           spi_pkt_handler(read_buffer, MAX_PKT_SIZE - len);
   	   SPI2_DR = 0x5a;
        } else {
           DMA_CNDTR(DMA1, DMA_CHANNEL4) = MAX_PKT_SIZE;
           SET_BIT(DMA_CCR(DMA1, DMA_CHANNEL4), DMA_CCR_EN); //Enable DMA
           SET_BIT(SPI_CR2(SPI2), SPI_CR2_RXDMAEN);          //Rx DMA enable
           CLEAR_BIT(SPI_CR1(SPI2), SPI_CR1_SSI);                //NSS = 0
           //spi_set_nss_low(SPI2);
           //*sendptr++ = 'y';
        }
        EXTI_PR = INPUT_CSN.pin;
    }

}

