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
                ? SPI_CR1_BR_FPCLK_DIV_8    \
                : SPI_CR1_BR_FPCLK_DIV_4,   \
            DEFAULT_SPI_SETTINGS,                 \
           })
    #endif  // SPI1_CFG
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
                ? SPI_CR1_BR_FPCLK_DIV_8          \
                : SPI_CR1_BR_FPCLK_DIV_4,         \
            DEFAULT_SPI_SETTINGS,                 \
           })
    #endif  // SPI1_CFG
    #define LCD_SPI_CFG SPI1_CFG
#endif  // LCD_SPI

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
            .rate = SPI_CR1_BR_FPCLK_DIV_16,       \
            DEFAULT_SPI_SETTINGS,                  \
           })
    #endif
    #define PROTO_SPI_CFG SPI2_CFG
    #define PROTO_RST_PIN ((struct mcu_pin){GPIOB, GPIO11})
#endif  // PROTO_SPI

#ifndef TOUCH_SPI
    #define TOUCH_SPI ((struct spi_csn) { \
        .spi = SPI1,           \
        .csn = {GPIOB, GPIO0}, \
        })
    #define TOUCH_IRQ_PIN ((struct mcu_pin) {GPIOB, GPIO5})
    #define TOUCH_COORDS_REVERSE      1
    #define TOUCH_SPI_CFG SPI1_CFG
#endif  // TOUCH_SPI

#ifndef PWM_TIMER
    #define PWM_TIMER ((struct tim_config) { \
        .tim = TIM1,             \
        .pin = {GPIOA, GPIO9},   \
        .ch = 2,                 \
        })
    #define PWM_TIMER_ISR exti9_5_isr    // Matches PA.9
#endif  // PWM_TIMER

#ifndef BACKLIGHT_TIM
    #define BACKLIGHT_TIM ((struct tim_config) { \
        .tim = TIM3,            \
        .pin = {GPIOB, GPIO1},  \
        .ch = 4,                \
        })
#endif  // BACKLIGHT_TIM

#ifndef SOUND_TIM
    #define SOUND_TIM ((struct tim_config) { \
        .tim = TIM2,           \
        .pin = {GPIOA, GPIO1}, \
        .ch = 2,               \
        })
#endif  // SOUND_TIM

#ifndef SYSCLK_TIM
    #define SYSCLK_TIM ((struct tim_config) { \
        .tim = TIM4,   \
        .ch = 1        \
        })
    #define SYSCLK_TIMER_ISR tim4_isr
#endif  // SYSCLK_TIM

#ifndef SSER_TIM
    #define SSER_TIM ((struct tim_config) { \
        .tim = TIM6,    \
        })
    #define SSER_TIM_ISR tim6_isr
    #define SSER_RX_ISR exti15_10_isr
#endif  // SSER-TIM

#ifndef USB_ENABLE_PIN
    #define USB_ENABLE_PIN ((struct mcu_pin) {GPIOB, GPIO10})
#endif

#ifndef HAPTIC_PIN
    #define HAPTIC_PIN ((struct mcu_pin) {GPIOD, GPIO2})
#endif

// Power switch configuration
#ifndef PWR_SWITCH_PIN
    #define PWR_SWITCH_PIN ((struct mcu_pin) {GPIOA, GPIO3})
#endif

#ifndef PWR_ENABLE_PIN
    #define PWR_ENABLE_PIN ((struct mcu_pin) {GPIOA, GPIO2})
#endif

// Protocol pins
#ifndef CYRF_RESET_PIN
    #define CYRF_RESET_PIN ((struct mcu_pin) {GPIOB, GPIO11})
#endif
#ifndef AVR_RESET_PIN
    #define AVR_RESET_PIN ((struct mcu_pin) {GPIO_BANK_JTCK_SWCLK, GPIO_JTCK_SWCLK})
#endif

// USB
#ifndef USB_ENABLE_PIN
    #define USB_ENABLE_PIN ((struct mcu_pin) {GPIOB, GPIO10})
#endif  // USB_ENABLE_PIN
#ifndef USB_DETECT_PIN
    #define USB_DETECT_PIN NULL_PIN
#endif  // USB_DETECT_PIN

#include "target/drivers/mcu/stm32/hardware.h"

// LED Pins
#ifndef LED_STATUS_PIN
#define LED_STATUS_PIN    NULL_PIN
#endif
#ifndef LED_RF_PIN
#define LED_RF_PIN        NULL_PIN
#endif
#ifndef LED_STORAGE_PIN
#define LED_STORAGE_PIN   NULL_PIN
#endif

#endif  // _DEVO_DEFAULT_HARDWARE_H_
