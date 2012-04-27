/*
    This project is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Foobar is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <libopencm3/stm32/f1/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/f1/gpio.h>
#include "target.h"

#include <stdio.h>
#include <string.h>

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
    usart_set_mode(USART1, USART_MODE_TX_RX);
    usart_set_parity(USART1, USART_PARITY_NONE);
    usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);

    /* Finally enable the USART. */
    usart_enable(USART1);
}

#if 0
/* Doesnt work :( */
int _write(int file, char *ptr, int len)
{
    int i;

    if (file == 1) {
        for (i = 0; i < len; i++)
            usart_send_blocking(USART1, ptr[i]);
        return i;
    }
    errno = EIO;
    return -1;
    }
#endif

void SignOn()
{
    u8 * pBLString = (u8*)0x08001000;
    u8 Power =  *((u8*)0x08001007);
    u8 mfgdata[6];
    u8 tmp[8];
    
    printf("\n\rOpen Whatever\n\r");
    /* Check CPU type */

    memcpy(tmp, pBLString, 7);
    tmp[7] = 0;
    
    printf("BootLoader    : '%s'\n\r",tmp);
    printf("Power         : '%s'\n\r",Power == 0 ? "100mW" : "10mW" );
    printf("SPI Flash     : '%X'\n\r",(unsigned int)SPIFlash_ReadID());
    CYRF_GetMfgData(mfgdata);
    printf("CYRF Mfg Data : '%02X %02X %02X %02X %02X %02X'\n\r",
            mfgdata[0],
            mfgdata[1],
            mfgdata[2],
            mfgdata[3],
            mfgdata[4],
            mfgdata[5]);
    
}


