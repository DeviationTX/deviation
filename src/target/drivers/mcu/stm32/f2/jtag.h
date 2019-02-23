#ifndef _DTX_STM32F1_JTAG_H_
#define _DTX_STM32F1_JTAG_H_
#include "common.h"
  
static void DisableJTAG()
{
    PORT_pin_setup_pushpull_af(((struct mcu_pin){GPIOA, GPIO13}), GPIO_AF14);
    PORT_pin_setup_pushpull_af(((struct mcu_pin){GPIOA, GPIO14}), GPIO_AF14);
    PORT_pin_setup_pushpull_af(((struct mcu_pin){GPIOA, GPIO15}), GPIO_AF14);
    PORT_pin_setup_pushpull_af(((struct mcu_pin){GPIOB, GPIO2}), GPIO_AF14);
    PORT_pin_setup_pushpull_af(((struct mcu_pin){GPIOB, GPIO3}), GPIO_AF14);
}

#endif  // _DTX_STM32F1_JTAG_H_
