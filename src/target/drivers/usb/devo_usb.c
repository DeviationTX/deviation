#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/msc.h>

#include "common.h"

#include "devo_usb.h"

usbd_device *usbd_dev;

uint8_t usbd_control_buffer[128];

#ifdef ENABLE_MODULAR
void (*_HID_Init)() = NULL;
#endif

const struct usb_device_descriptor dev_descr = {
    .bLength = USB_DT_DEVICE_SIZE,
    .bDescriptorType = USB_DT_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = 0,
    .bDeviceSubClass = 0,
    .bDeviceProtocol = 0,
    .bMaxPacketSize0 = 64,
    .idVendor = 0x0483,
    .idProduct = 0x5720,
    .bcdDevice = 0x0200,
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 3,
    .bNumConfigurations = 1,
};

const char *usb_strings[USB_STRING_COUNT] = {
    "STMicroelectronics",
    "STM32 Mass Storage",
    "STM32 1.0"
};

void USB_Enable(unsigned use_interrupt)
{
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_OTGFS);

    GPIO_setup_input_af((struct mcu_pin){GPIOA, GPIO11 | GPIO12}, ITYPE_FLOAT, 0);

    rcc_periph_clock_enable(RCC_GPIOB);
    GPIO_setup_output((struct mcu_pin){GPIOB, GPIO10}, OTYPE_PUSHPULL);
    gpio_clear(GPIOB, GPIO10);

    if (use_interrupt) {
        nvic_enable_irq(NVIC_USB_LP_CAN_RX0_IRQ);
        nvic_enable_irq(NVIC_USB_WAKEUP_IRQ);
    }
}

void USB_Disable()
{
    gpio_set(GPIOB, GPIO10);
    nvic_disable_irq(NVIC_USB_LP_CAN_RX0_IRQ);
    nvic_disable_irq(NVIC_USB_WAKEUP_IRQ);
}

void USB_Poll()
{
    usbd_poll(usbd_dev);
}

void usb_wakeup_isr(void)
{
    usbd_poll(usbd_dev);
}

void usb_lp_can_rx0_isr(void)
{
    usbd_poll(usbd_dev);
}
