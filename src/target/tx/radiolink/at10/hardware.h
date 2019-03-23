#ifndef _AT10_HARDWARE_H_
#define _AT10_HARDWARE_H_

#include "target/drivers/mcu/stm32/gpio.h"

#define LED_STATUS_PIN ((struct mcu_pin){GPIOA, GPIO13})
#define LED_RF_PIN ((struct mcu_pin){GPIOC, GPIO12})
#define LED_STORAGE_PIN NULL_PIN

#define ADC_CHANNELS { \
    ADC_CHAN(GPIOA, GPIO0, CHAN_INVERT),  /* ADC123_0 - INP_AIL */ \
    ADC_CHAN(GPIOA, GPIO1, CHAN_INVERT),  /* ADC123_1 - INP_ELE */ \
    ADC_CHAN(GPIOA, GPIO2, CHAN_INVERT),  /* ADC123_2 - INP_THR */ \
    ADC_CHAN(GPIOA, GPIO3, CHAN_INVERT),  /* ADC123_3 - INP_RUD */ \
    ADC_CHAN(GPIOA, GPIO5, CHAN_NONINV),  /* ADC12_5  - INP_AUX4 */ \
    ADC_CHAN(GPIOA, GPIO6, CHAN_NONINV),  /* ADC12_6  - INP_AUX5 */ \
    ADC_CHAN(GPIOA, GPIO7, CHAN_NONINV),  /* ADC12_7  - INP_AUX6 */ \
    ADC_CHAN(GPIOB, GPIO0, CHAN_NONINV),  /* ADC12_8  - INP_AUX7 */ \
    ADC_CHAN(GPIOA, GPIO4, CHAN_NONINV),  /* ADC12_4  - INP_AUX8 */ \
    ADC_CHAN(0, 16, CHAN_NONINV),         /* TEMPERATURE */ \
    ADC_CHAN(GPIOB, GPIO1, CHAN_NONINV),  /* ADC12_9   */ \
    }

#define SWITCHES \
    TWO_WAY(INP_SWA, (GPIOC, GPIO0), CHAN_INVERT) \
    TWO_WAY(INP_SWB, (GPIOC, GPIO1), CHAN_INVERT) \
    THREE_WAY(INP_SWC, (GPIOC, GPIO2), (GPIOC, GPIO3), CHAN_INVERT) \
    TWO_WAY(INP_SWD, (GPIOC, GPIO6), CHAN_INVERT) \
    THREE_WAY(INP_SWE, (GPIOC, GPIO4), (GPIOC, GPIO5), CHAN_INVERT) \
    TWO_WAY(INP_SWF, (GPIOC, GPIO7), CHAN_INVERT) \
    THREE_WAY(INP_SWG, (GPIOC, GPIO8), (GPIOC, GPIO9), CHAN_INVERT) \
    TWO_WAY(INP_SWH, (GPIOA, GPIO8), CHAN_INVERT)

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

#endif  // _AT10_HARDWARE_H_
