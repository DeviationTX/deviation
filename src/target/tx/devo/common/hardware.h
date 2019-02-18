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

#endif  // _DEVO_DEFAULT_HARDWARE_H_
