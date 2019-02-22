#ifndef _RADIOLINK_HARDWARE_H_
#define _RADIOLINK_HARDWARE_H_

#include "target/drivers/mcu/stm32/gpio.h"

#define ROTARY_PIN0      ((struct mcu_pin){GPIOC, GPIO13})
#define ROTARY_PIN1      ((struct mcu_pin){GPIOC, GPIO14})
#define ROTARY_PRESS_PIN ((struct mcu_pin){GPIOC, GPIO15})
#define ROTARY_ISR       exti15_10_isr

#endif  // _RADIOLINK_HARDWARE_H_
