#ifndef _DTX_STM32F1_JTAG_H_
#define _DTX_STM32F1_JTAG_H_
#include "common.h"

static void DisableJTAG()
{
    gpio_set_af(GPIOA, GPIO_AF14, GPIO13);
    gpio_set_af(GPIOA, GPIO_AF14, GPIO14);
    gpio_set_af(GPIOA, GPIO_AF14, GPIO15);
    gpio_set_af(GPIOB, GPIO_AF14, GPIO2);
    gpio_set_af(GPIOB, GPIO_AF14, GPIO3);
}

#endif  // _DTX_STM32F1_JTAG_H_
