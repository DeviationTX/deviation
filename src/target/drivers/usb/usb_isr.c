#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/msc.h>

#include "common.h"

#include "devo_usb.h"

void __attribute__((__used__)) usb_wakeup_isr(void)
{
    usbd_poll(usbd_dev);
}

void __attribute__((__used__)) usb_lp_can_rx0_isr(void)
{
    usbd_poll(usbd_dev);
}
