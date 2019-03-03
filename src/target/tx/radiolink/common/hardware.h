#ifndef _RADIOLINK_HARDWARE_H_
#define _RADIOLINK_HARDWARE_H_

#include "target/drivers/mcu/stm32/gpio.h"

#define ROTARY_PIN0      ((struct mcu_pin){GPIOC, GPIO13})
#define ROTARY_PIN1      ((struct mcu_pin){GPIOC, GPIO14})
#define ROTARY_PRESS_PIN ((struct mcu_pin){GPIOC, GPIO15})
#define ROTARY_ISR       exti15_10_isr

#define CYRF_RESET_PIN   ((struct mcu_pin){GPIOB, GPIO11})
#define AVR_RESET_PIN    ((struct mcu_pin){GPIOB, GPIO11})

#define LCD_RESET_PIN ((struct mcu_pin){GPIOE, GPIO0})

#define USB_ENABLE_PIN   NULL_PIN

#define PWR_ENABLE_PIN NULL_PIN
#define PWR_SWITCH_PIN NULL_PIN

#define PROTO_SPI ((struct spi_csn) { \
    .spi = SPI2,                      \
    .csn = {GPIOB, GPIO12},           \
    })
#define SPI2_CFG ((struct spi_config) {    \
    .spi = SPI2,                           \
    .sck = {GPIOB, GPIO13},                \
    .miso = {GPIOB, GPIO14},               \
    .mosi = {GPIOB, GPIO15},               \
    .rate = SPI_CR1_BR_FPCLK_DIV_16, \
    DEFAULT_SPI_SETTINGS,                  \
    })
#define PROTO_SPI_CFG SPI2_CFG
#define PROTO_RST_PIN ((struct mcu_pin){GPIOB, GPIO11})

#define ADC_OVERSAMPLE_WINDOW_COUNT 1
#define ADC_CFG ((struct adc_config) {       \
    .adc = ADC1,                             \
    .prescalar = RCC_CFGR_ADCPRE_PCLK2_DIV6, \
    .sampletime = ADC_SMPR_SMP_239DOT5CYC,   \
    })
#define ADC_DMA ((struct dma_config) { \
    .dma = DMA1,                       \
    .stream = DMA_CHANNEL1,            \
    })

#define UART_CFG ((struct uart_config) {   \
    .uart = USART1,                         \
    .tx = ((struct mcu_pin){GPIOA, GPIO9}),  \
    .rx = ((struct mcu_pin){GPIOA, GPIO10}), \
    })
#define _UART_ISR usart1_isr

#define USART_DMA ((struct dma_config) { \
    .dma = DMA1,                       \
    .stream = DMA_CHANNEL4,            \
    })
#define _USART_DMA_ISR                dma1_channel4_isr

#ifndef SYSCLK_TIM
    #define SYSCLK_TIM ((struct tim_config) { \
        .tim = TIM4,   \
        .ch = 1        \
        })
    #define SYSCLK_TIMER_ISR tim4_isr
#endif  // SYSCLK_TIM

#ifndef BACKLIGHT_TIM
    #define BACKLIGHT_TIM ((struct tim_config) { \
        .tim = TIM3,            \
        .pin = {GPIOB, GPIO1},  \
        .ch = 4,                \
        })
#endif  // BACKLIGHT_TIM

// USB
#ifndef USB_ENABLE_PIN
    #define USB_ENABLE_PIN NULL_PIN
#endif  // USB_ENABLE_PIN
#ifndef USB_DETECT_PIN
    #define USB_DETECT_PIN ((struct mcu_pin) {GPIOD, GPIO6})
#endif  // USB_DETECT_PIN

#include "target/drivers/mcu/stm32/hardware.h"

#endif  // _RADIOLINK_HARDWARE_H_
