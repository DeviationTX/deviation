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

#endif  // _DEVO_DEFAULT_HARDWARE_H_
