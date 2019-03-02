#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/msc.h>

#include "common.h"
#include "target/drivers/mcu/stm32/rcc.h"

#include "devo_usb.h"

usbd_device *usbd_dev;

uint8_t usbd_control_buffer[128];

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

const char USB_Product_Name[] = "DeviationTx";

void USB_Enable(unsigned use_interrupt)
{
    rcc_periph_clock_enable(RCC_GPIOA);
    GPIO_setup_input_af((struct mcu_pin){GPIOA, GPIO11 | GPIO12}, ITYPE_FLOAT, 0);

    if (HAS_PIN(USB_DETECT_PIN)) {
        rcc_periph_clock_enable(get_rcc_from_pin(USB_DETECT_PIN));
        GPIO_setup_input(USB_DETECT_PIN, ITYPE_PULLUP);
    }

    if (HAS_PIN(USB_ENABLE_PIN)) {
        rcc_periph_clock_enable(get_rcc_from_pin(USB_ENABLE_PIN));
        GPIO_setup_output(USB_ENABLE_PIN, OTYPE_PUSHPULL);
        GPIO_pin_clear(USB_ENABLE_PIN);
    } else {
        GPIO_setup_output((struct mcu_pin){GPIOA, GPIO12}, OTYPE_PUSHPULL);
        GPIO_pin_clear((struct mcu_pin){GPIOA, GPIO12});
        _msleep(100);
    }

    if (use_interrupt) {
        nvic_enable_irq(NVIC_USB_LP_CAN_RX0_IRQ);
        nvic_enable_irq(NVIC_USB_WAKEUP_IRQ);
    }
}

void USB_Disable()
{
    nvic_disable_irq(NVIC_USB_LP_CAN_RX0_IRQ);
    nvic_disable_irq(NVIC_USB_WAKEUP_IRQ);

    if (HAS_PIN(USB_ENABLE_PIN)) {
        GPIO_pin_set(USB_ENABLE_PIN);
    } else {
        gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ,
                GPIO_CNF_OUTPUT_PUSHPULL, GPIO12);
        gpio_clear(GPIOA, GPIO12);
    }
}

void USB_Poll()
{
    usbd_poll(usbd_dev);
}

