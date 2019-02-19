#ifndef _DEVO_DEFAULT_HARDWARE_H_
#define _DEVO_DEFAULT_HARDWARE_H_

#include "target/drivers/mcu/stm32/gpio.h"

#ifndef ADC_CFG
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
#endif

#ifndef USART_DMA
    #define USART_DMA ((struct dma_config) { \
        .dma = DMA1,                       \
        .stream = DMA_CHANNEL4,            \
        })
    #define _USART_DMA_ISR                dma1_channel4_isr
#endif

#ifndef PWM_DMA
    #define PWM_DMA ((struct dma_config) { \
        .dma = DMA1,                       \
        .stream = DMA_CHANNEL3,            \
        })
    #define _PWM_DMA_ISR                dma1_channel3_isr
#endif

#ifndef UART_CFG
    #define UART_CFG ((struct uart_config) {   \
        .uart = USART1,                         \
        .tx = ((struct mcu_pin){GPIOA, GPIO9}),  \
        .rx = ((struct mcu_pin){GPIOA, GPIO10}), \
        })
    #define _UART_ISR usart1_isr
#endif

#ifndef AUDIO_UART_CFG
    #define AUDIO_UART_CFG ((struct uart_config) {   \
        .uart = UART5,                         \
        .tx = ((struct mcu_pin){GPIOC, GPIO12}),  \
        .rx = ((struct mcu_pin){GPIOD, GPIO2}), \
        })
#endif

#ifndef FLASH_SPI
    #define FLASH_SPI ((struct spi_csn) { \
        .spi = SPI1, \
        .csn = {GPIOB, GPIO2}, \
        })
    // On STM32F1, LCD is on SPI1->APB2@72MHz, ClockDiv=4, Fspi=18MHz
    // On STM32F1, LCD is on SPI1->APB2@72MHz, ClockDiv=8, Fspi=9MHz
    #ifndef HAS_OLED_DISPLAY
        #define HAS_OLED_DISPLAY 0
    #endif
    #ifndef SPI1_CFG
        #define SPI1_CFG ((struct spi_config) {   \
            .spi = SPI1,                          \
            .sck = {GPIOA, GPIO5},                \
            .miso = {GPIOA, GPIO6},               \
            .mosi = {GPIOA, GPIO7},               \
            .rate = HAS_OLED_DISPLAY              \
                ? SPI_CR1_BAUDRATE_FPCLK_DIV_8    \
                : SPI_CR1_BAUDRATE_FPCLK_DIV_4,   \
            DEFAULT_SPI_SETTINGS,                 \
           })
    #endif  //SPI1_CFG
    #define FLASH_SPI_CFG SPI1_CFG
#endif  // FLASH_SPI

#ifndef LCD_SPI
    #define LCD_SPI ((struct spi_csn) { \
        .spi = SPI1,                    \
        .csn = {GPIOB, GPIO0},          \
        })
    #define LCD_SPI_MODE ((struct mcu_pin) {GPIOC, GPIO5})
    #ifndef SPI1_CFG
        #define SPI1_CFG ((struct spi_config) {   \
            .spi = SPI1,                          \
            .sck = {GPIOA, GPIO5},                \
            .miso = {GPIOA, GPIO6},               \
            .mosi = {GPIOA, GPIO7},               \
            .rate = HAS_OLED_DISPLAY              \
                ? SPI_CR1_BAUDRATE_FPCLK_DIV_8    \
                : SPI_CR1_BAUDRATE_FPCLK_DIV_4,   \
            DEFAULT_SPI_SETTINGS,                 \
           })
    #endif  //SPI1_CFG
    #define LCD_SPI_CFG SPI1_CFG
#endif // LCD_SPI

// FIXME
#ifndef _SPI_PROTO_PORT
    #define _SPI_PROTO_PORT          2 //SPI2
    #define _SPI_PROTO_RST_PIN       {GPIOB, GPIO11}
    #define _SPI_PROTO_CSN_PIN       {GPIOB, GPIO12}
    #define _SPI_PROTO_SCK_PIN       {GPIOB, GPIO13}
    #define _SPI_PROTO_MISO_PIN      {GPIOB, GPIO14}
    #define _SPI_PROTO_MOSI_PIN      {GPIOB, GPIO15}
#endif
#ifndef EMULATOR
static const struct mcu_pin PROTO_RST_PIN   = _SPI_PROTO_RST_PIN;
static const struct mcu_pin PROTO_CSN_PIN   = _SPI_PROTO_CSN_PIN;
static const struct mcu_pin PROTO_SCK_PIN   = _SPI_PROTO_SCK_PIN;
static const struct mcu_pin PROTO_MISO_PIN  = _SPI_PROTO_MISO_PIN;
static const struct mcu_pin PROTO_MOSI_PIN  = _SPI_PROTO_MOSI_PIN;
#endif

#ifndef PROTO_SPI
    #define PROTO_SPI ((struct spi_csn) { \
        .spi = SPI2,                      \
        .csn = {GPIOB, GPIO12},           \
        })
    #ifndef SPI2_CFG 
        #define SPI2_CFG ((struct spi_config) {    \
            .spi = SPI2,                           \
            .sck = {GPIOB, GPIO13},                \
            .miso = {GPIOB, GPIO14},               \
            .mosi = {GPIOB, GPIO15},               \
            .rate = SPI_CR1_BAUDRATE_FPCLK_DIV_16, \
            DEFAULT_SPI_SETTINGS,                  \
           })
    #endif
    #define PROTO_SPI_CFG SPI2_CFG
#endif  // PROTO_SPI
#endif  // _DEVO_DEFAULT_HARDWARE_H_
