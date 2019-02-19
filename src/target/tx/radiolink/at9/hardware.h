#ifndef _AT9_HARDWARE_H_
#define _AT9_HARDWARE_H_

#include "target/drivers/mcu/stm32/gpio.h"

#define LCD_RESET_PIN ((struct mcu_pin){GPIOE, GPIO0})

#define FLASH_SPI ((struct spi_csn) { \
    .spi = SPI2, \
    .csn = {GPIOB, GPIO12}, \
    })
#define FLASH_SPI_CFG SPI2_CFG

#endif  // _AT9_HARDWARE_H_
