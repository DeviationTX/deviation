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
#include <libopencm3/usb/usbd.h>
#include "msc_bot.h"
#include "target.h"

#define LE_WORD(x)		((x)&0xFF),((x)>>8)

static const struct usb_device_descriptor dev = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0200,
	.bDeviceClass = 0,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 64,
	.idVendor = 0xFFFF,
	.idProduct = 0x0003,
	.bcdDevice = 0x0100,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 1,
};

static const struct usb_endpoint_descriptor bulk_endp[] = {{
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = MSC_BULK_IN_EP,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 0,
}, {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = MSC_BULK_OUT_EP,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 0,
}};

static const struct usb_interface_descriptor bulk_iface[] = {{
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bNumEndpoints = 2,
	.bInterfaceClass = 0x08, /* Mass Storage */
	.bInterfaceSubClass = 0x06, /* transparent SCSI */
	.bInterfaceProtocol = 0x50, /* BOT */
	.iInterface = 0,

	.endpoint = bulk_endp,
	.extralen = 0,
}};

static const struct usb_interface ifaces[] = {{
	.num_altsetting = 1,
	.altsetting = bulk_iface,
}};

static const struct usb_config_descriptor config = {
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 32,
	.bNumInterfaces = 1,
	.bConfigurationValue = 1,
	.iConfiguration = 0,
	.bmAttributes = 0xC0,
	.bMaxPower = 0x32,

	.interface = ifaces,
};

static const char *usb_strings[] = {
	"x",
	"LPCUSB",
	"Deviation",
	"DEMO",
};

static int msc_control_request(struct usb_setup_data *req, u8 **buf,
		u16 *len, void (**complete)(struct usb_setup_data *req))
{
	(void)complete;
	(void)buf;

	if (req->wIndex != 0) {
		DBG("Invalid idx %X\n\r", req->wIndex);
		return 0;
	}
	if (req->wValue != 0) {
		DBG("Invalid val %X\n\r", req->wValue);
		return 0;
	}

	switch (req->bRequest) {

	// get max LUN
	case 0xFE:
		*buf[0] = 0;		// No LUNs
		*len = 1;
		break;

	// MSC reset
	case 0xFF:
		if (req->wLength > 0) {
			return 0;
		}
		MSCBotReset();
		break;
		
	default:
		DBG("Unhandled class\n\r");
		return 0;
	}
	return 1;
}

static void msc_set_config(u16 wValue)
{
	(void)wValue;

	usbd_ep_setup(MSC_BULK_IN_EP, USB_ENDPOINT_ATTR_BULK, 64, MSCBotBulkIn);
	usbd_ep_setup(MSC_BULK_OUT_EP, USB_ENDPOINT_ATTR_BULK, 64, MSCBotBulkOut);

	usbd_register_control_callback(
				USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
				USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
				msc_control_request);
}

void usb_hp_can_tx_isr()
{
        usbd_poll();
}

void usb_lp_can_rx0_isr()
{
        usbd_poll();
}

int main(void)
{
	int i;

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

	usbd_init(&stm32f103_usb_driver, &dev, &config, usb_strings);
	usbd_register_set_config_callback(msc_set_config);
        nvic_enable_irq(NVIC_USB_HP_CAN_TX_IRQ);
        nvic_enable_irq(NVIC_USB_LP_CAN_RX0_IRQ);

	for (i = 0; i < 0x800000; i++)
		__asm__("nop");
	gpio_clear(GPIOB, GPIO10);

	while (1) {
		if(PWR_CheckPowerSwitch())
			PWR_Shutdown();
		//usbd_poll();
        }
}

