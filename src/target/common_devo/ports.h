#ifndef _PORTS_H_
#define _PORTS_H_

#ifndef _ADC
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
#endif //_ADC

//Power switch configuration
#ifndef _PWRSW_PORT
    #define _PWRSW_PORT               GPIOA
    #define _PWRSW_PIN                GPIO3
    #define _PWRSW_RCC_APB2ENR_IOPEN  RCC_APB2ENR_IOPAEN
#endif //_SWITCH_PORT
#ifndef _PWREN_PORT
    #define _PWREN_PORT                GPIOA
    #define _PWREN_PIN                 GPIO2
    #define _PWREN_RCC_APB2ENR_IOPEN   RCC_APB2ENR_IOPAEN
#endif //_PWREN_PORT

#ifndef _SOUND_PORT
    #define _SOUND_PORT                GPIOA
    #define _SOUND_PIN                 GPIO1
    #define _SOUND_RCC_APB1ENR_TIMEN   RCC_APB1ENR_TIM2EN
    #define _SOUND_TIM_OC              TIM_OC2
    #define _SOUND_TIM                 TIM2
#endif //_SOUND_PORT

#ifndef _TOUCH_PORT
    #define _TOUCH_PORT                GPIOB
    #define _TOUCH_PIN                 GPIO0
    #define _TOUCH_IRQ_PIN             GPIO5
    #define _TOUCH_RCC_APB2ENR_IOPEN   RCC_APB2ENR_IOPBEN
    #define _TOUCH_COORDS_REVERSE      1
#endif //_TOUCH_PORT

#ifndef _PWM_PIN
    #define _PWM_PIN GPIO_USART1_TX
#endif //_PWM_PIN

#endif //_PORTS_H_

