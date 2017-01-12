#ifndef _RADIOLINK_AT9_PORTS_H
#define _RADIOLINK_AT9_PORTS_H

#define FREQ_MHz 72

#define SYSCLK_TIM 3

#define _SPI_FLASH_PORT              2 //SPI2
    #define _SPI_FLASH_CSN_PIN       {GPIOB, GPIO12}
    #define _SPI_FLASH_SCK_PIN       {GPIOB, GPIO13}
    #define _SPI_FLASH_MISO_PIN      {GPIOB, GPIO14}
    #define _SPI_FLASH_MOSI_PIN      {GPIOB, GPIO15}
    
#include "../../common/devo/ports.h"

#if 0
// _SPI_PROTO_PORT defined in ports.h

#if 0 // defined in ports.h
    #define _USART                        USART1
    #define _USART_DR                     USART1_DR
    #define _USART_DMA                    DMA1
    #define _USART_DMA_CHANNEL            DMA_CHANNEL4
    #define _USART_DMA_ISR                dma1_channel4_isr
    #define _USART_NVIC_DMA_CHANNEL_IRQ   NVIC_DMA1_CHANNEL4_IRQ
    #define _USART_RCC_APB_ENR_IOP        RCC_APB2ENR
    #define _USART_RCC_APB_ENR_IOP_EN     RCC_APB2ENR_IOPAEN
    #define _USART_RCC_APB_ENR_USART      RCC_APB2ENR
    #define _USART_RCC_APB_ENR_USART_EN   RCC_APB2ENR_USART1EN
    #define _USART_GPIO                   GPIOA
    #define _USART_GPIO_USART_TX          GPIO_USART1_TX
#endif

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
#endif

#endif //_RADIOLINK_AT9_PORTS_H
