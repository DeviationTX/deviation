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
#include <libopencm3/cm3/nvic.h>
#include "common.h"

#define __IO volatile
#include "usb_core.h"
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
void (*pEpInt_IN[7])(void) = {};
void (*pEpInt_OUT[7])(void) = {};
DEVICE_PROP *Device_Property;
USER_STANDARD_REQUESTS *User_Standard_Requests;

extern void (*MSC_pEpInt_IN[7])(void);
extern void (*MSC_pEpInt_OUT[7])(void);
extern DEVICE_PROP MSC_Device_Property;
extern USER_STANDARD_REQUESTS MSC_User_Standard_Requests;

extern void (*HID_pEpInt_IN[7])(void);
extern void (*HID_pEpInt_OUT[7])(void);
extern DEVICE_PROP HID_Device_Property;
extern USER_STANDARD_REQUESTS HID_User_Standard_Requests;

void MSC_Init() {
    memcpy(pEpInt_IN, MSC_pEpInt_IN, sizeof(pEpInt_IN));
    memcpy(pEpInt_OUT, MSC_pEpInt_OUT, sizeof(pEpInt_OUT));
    Device_Property = &MSC_Device_Property;
    User_Standard_Requests = &MSC_User_Standard_Requests;
}

#ifdef MODULAR
void (*_HID_Init)() = NULL;
#else
extern void HID_Init();
#endif
#if 0
void HID_Init() {
    memcpy(pEpInt_IN, HID_pEpInt_IN, sizeof(pEpInt_IN));
    memcpy(pEpInt_OUT, HID_pEpInt_OUT, sizeof(pEpInt_OUT));
    Device_Property = &HID_Device_Property;
    User_Standard_Requests = &HID_User_Standard_Requests;
}
#endif
void USB_Enable(u8 type, u8 use_interrupt)
{
    if (type == 0) {
        //Mass Storage
        MSC_Init();
    } else if (type == 1) {
#ifndef MODULAR
        HID_Init();
#else
        if(_HID_Init)
            _HID_Init();
#endif
    }
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
