#ifndef _AT9_HARDWARE_H_
#define _AT9_HARDWARE_H_

#include "target/drivers/mcu/stm32/gpio.h"

#define LCD_RESET_PIN ((struct mcu_pin){GPIOE, GPIO0})

#define FLASH_SPI ((struct spi_csn) { \
    .spi = SPI2, \
    .csn = {GPIOB, GPIO12}, \
    })
#define FLASH_SPI_CFG SPI2_CFG

#define SYSCLK_TIM ((struct tim_config) { \
    .tim = TIM3, \
    .ch = 1,     \
    })
#define SYSCLK_TIMER_ISR tim3_isr

#define BACKLIGHT_TIM ((struct tim_config) { \
    .tim = TIM4,            \
    .pin = {GPIOB, GPIO9},  \
    .ch = 4,                \
    })

#endif  // _AT9_HARDWARE_H_
