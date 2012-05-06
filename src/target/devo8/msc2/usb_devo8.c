/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
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
#include <stdlib.h>
#include <libopencm3/stm32/f1/rcc.h>
#include <libopencm3/stm32/f1/gpio.h>
#include <libopencm3/stm32/nvic.h>
#include "target.h"

#define LE_WORD(x)		((x)&0xFF),((x)>>8)

void USB_Init();
void USB_Istr(void);

/*
void usb_hp_can_tx_isr()
{
        USB_Istr();
}
*/

void usb_lp_can_rx0_isr()
{
        USB_Istr();
}
void USB_Initialize() {
    rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_USBEN);
    USB_Init();  
    //nvic_enable_irq(NVIC_USB_HP_CAN_TX_IRQ);
    nvic_enable_irq(NVIC_USB_LP_CAN_RX0_IRQ);
}
#ifdef USB_TEST
int main(void)
{
	PWR_Init();
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
		GPIO_CNF_OUTPUT_OPENDRAIN, GPIO10);
	gpio_set(GPIOB, GPIO10);
	Delay(0x2710);
	LCD_Init();
        SPIFlash_Init();
        UART_Initialize();

	LCD_Clear(0x0000);
        LCD_PrintStringXY(40, 10, "Hello\n");
        printf("Hello\n\r");

        USB_Initialize();
	//gpio_set(GPIOB, GPIO10);
	SPI_FlashBlockWriteEnable(1);
	while (1) {
		if(PWR_CheckPowerSwitch())
			PWR_Shutdown();
        }
}
#endif
