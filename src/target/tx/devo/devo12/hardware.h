#ifndef _DEVO12_HARDWARE_H_
#define _DEVO12_HARDWARE_H_

#include "target/drivers/mcu/stm32/gpio.h"

// ADC overrides
#define ADC_OVERSAMPLE_WINDOW_COUNT 10
#define ADC_CFG ((struct adc_config) {       \
    .adc = ADC3,                             \
    .prescalar = RCC_CFGR_ADCPRE_PCLK2_DIV6, \
    .sampletime = ADC_SMPR_SMP_13DOT5CYC,   \
    })
#define ADC_DMA ((struct dma_config) { \
    .dma = DMA2,                       \
    .stream = DMA_CHANNEL5,            \
    })

// PWM overrides
#define PWM_DMA ((struct dma_config) { \
    .dma = DMA1,                       \
    .stream = DMA_CHANNEL2,            \
    })
#define _PWM_DMA_ISR                dma1_channel2_isr

#endif  // _DEVO12_HARDWARE_H_
