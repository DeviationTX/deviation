#ifndef _RADIOLINK_AT9_PORTS_H
#define _RADIOLINK_AT9_PORTS_H

#define FREQ_MHz 72

#define SYSCLK_TIM 3

#define _SPI_FLASH_PORT              2 //SPI2
    #define _SPI_FLASH_CSN_PIN       {GPIOB, GPIO12}
    #define _SPI_FLASH_SCK_PIN       {GPIOB, GPIO13}
    #define _SPI_FLASH_MISO_PIN      {GPIOB, GPIO14}
    #define _SPI_FLASH_MOSI_PIN      {GPIOB, GPIO15}

#define _USART USART1
#define ADC_OVERSAMPLE_WINDOW_COUNT 1
    #define _ADC                    ADC1
    #define _RCC_APB2ENR_ADCEN      RCC_APB2ENR_ADC1EN
    #define _RCC_APB2RSTR_ADCRST    RCC_APB2RSTR_ADC1RST
    #define _ADC_SMPR_SMP_XXDOT5CYC ADC_SMPR_SMP_239DOT5CYC
    #define _DMA                    DMA1
    #define _DMA_CHANNEL            DMA_CHANNEL1
    #define _RCC_AHBENR_DMAEN       RCC_AHBENR_DMA1EN
    #define _DMA_ISR                dma1_channel1_isr
    #define _DMA_IFCR_CGIF          DMA_IFCR_CGIF1
    #define _NVIC_DMA_CHANNEL_IRQ   NVIC_DMA1_CHANNEL1_IRQ

#endif //_RADIOLINK_AT9_PORTS_H
