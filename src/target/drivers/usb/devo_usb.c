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

void USB_Enable(unsigned type, unsigned use_interrupt)
{
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
        GPIO_CNF_OUTPUT_PUSHPULL, GPIO10);
    gpio_clear(GPIOB, GPIO10);

    rcc_periph_clock_enable(RCC_OTGFS);

    switch(type) {
        case USB_MSC:
            MSC_Init();
            break;
        case USB_HID:
#ifndef ENABLE_MODULAR
            HID_Init();
#else
            if(_HID_Init)
                _HID_Init();
#endif
            break;
    }

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


/* USB */
static void wait_press_release(u32 press)
{
    printf("Wait %s\n", press ? "Press" : "Release");
    while(1) {
        CLOCK_ResetWatchdog();
        u32 buttons = ScanButtons();
        if (CHAN_ButtonIsPressed(buttons, BUT_ENTER) == press)
            break;
        if(PWR_CheckPowerSwitch())
            PWR_Shutdown();
    }
    printf("%sed\n", press ? "Press" : "Release");
}

static void wait_press() {
    wait_press_release(CHAN_ButtonMask(BUT_ENTER));
}

static void wait_release()
{
    wait_press_release(0);
}

void USB_Connect()
{
    USB_Enable(USB_MSC, 1);
    //Disable USB Exit
    while(1) {
        if(PWR_CheckPowerSwitch())
            PWR_Shutdown();
    }
    wait_release();
    wait_press();
    wait_release();
    USB_Disable();
}
