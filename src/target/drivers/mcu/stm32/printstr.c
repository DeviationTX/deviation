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

#include "common.h"
#include <libopencm3/stm32/usart.h>

void printstr(char * ptr, int len)
{
    int index;

    if (0 == (USART_CR1(UART_CFG.uart) & USART_CR1_UE))
        return; //Don't send if USART is disabled

    for(index=0; index<len; index++) {
        if (ptr[index] == '\n') {
            usart_send_blocking(UART_CFG.uart, '\r');
        }  
        usart_send_blocking(UART_CFG.uart, ptr[index]);
    }    
    return;
}
