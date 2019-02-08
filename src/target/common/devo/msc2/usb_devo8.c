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
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include "common.h"

#define __IO volatile
#include "usb_core.h"
#include "usb_bot.h"
#include "usb_istr.h"
#define LE_WORD(x)		((x)&0xFF),((x)>>8)

void USB_Init();
void USB_Istr(void);

void (*pEpInt_IN[7])(void) = {};
void (*pEpInt_OUT[7])(void) = {};
const DEVICE_PROP *Device_Property;
const USER_STANDARD_REQUESTS *User_Standard_Requests;

extern const DEVICE_PROP MSC_Device_Property;
extern const USER_STANDARD_REQUESTS MSC_User_Standard_Requests;

void MSC_Init() {
    for (int i = 0; i < 7; i++) {
        pEpInt_IN[i] = pEpInt_OUT[i] = NOP_Process;
    }
    pEpInt_IN[0] = EP1_IN_Callback;
    pEpInt_OUT[1] = EP2_OUT_Callback;

    Device_Property = &MSC_Device_Property;
    User_Standard_Requests = &MSC_User_Standard_Requests;
}

void USB_Enable(unsigned use_interrupt)
{
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
        GPIO_CNF_OUTPUT_PUSHPULL, GPIO10);
        gpio_set(GPIOB, GPIO10);
    //rcc_set_usbpre(RCC_CFGR_USBPRE_PLL_CLK_DIV1_5);
    rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_USBEN);
    USB_Init();
    if(use_interrupt) {
        nvic_enable_irq(NVIC_USB_LP_CAN_RX0_IRQ);
    }
}

void USB_Disable()
{
    gpio_set(GPIOB, GPIO10);
    nvic_disable_irq(NVIC_USB_LP_CAN_RX0_IRQ);
}

void USB_HandleISR()
{
    USB_Istr();
}

void MSC_Enable()
{
    MSC_Init();
    USB_Enable(1);
}

void MSC_Disable()
{
    USB_Disable();
}

/*
void USB_Connect()
{
    //Wait for button release
    while((ScanButtons() & 0x02) == 0)
        ;
    USB_Enable(1);
    while((ScanButtons() & 0x02) != 0)
        ;
    //Wait for button release
    while((ScanButtons() & 0x02) == 0)
        ;
    USB_Disable(1);
}
*/

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
        printf("Hello\n");

        USB_Enable();
	//gpio_set(GPIOB, GPIO10);
	SPI_FlashBlockWriteEnable(1);
	while (1) {
		if(PWR_CheckPowerSwitch())
			PWR_Shutdown();
        }
}
#endif
