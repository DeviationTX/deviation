#ifndef _DEVO12_PORTS_H_
#define _DEVO12_PORTS_H_

//ADC overrides
#define ADC_OVERSAMPLE_WINDOW_COUNT 10
#define _ADC                   ADC3
#define _RCC_APB2ENR_ADCEN     RCC_APB2ENR_ADC3EN
#define _RCC_APB2RSTR_ADCRST   RCC_APB2RSTR_ADC3RST
#define _DMA                   DMA2
#define _DMA_CHANNEL           DMA_CHANNEL5
#define _RCC_AHBENR_DMAEN      RCC_AHBENR_DMA2EN
#define _DMA_ISR               dma2_channel4_5_isr
#define _DMA_IFCR_CGIF         DMA_IFCR_CGIF5
#define _NVIC_DMA_CHANNEL_IRQ  NVIC_DMA2_CHANNEL4_5_IRQ
//End ADC

//Power Switch configuration
#define _PWRSW_PORT               GPIOA
#define _PWRSW_PIN                GPIO4
#define _PWRSW_RCC_APB2ENR_IOPEN  RCC_APB2ENR_IOPAEN

#define _PWREN_PORT                GPIOE
#define _PWREN_PIN                 GPIO0
#define _PWREN_RCC_APB2ENR_IOPEN   RCC_APB2ENR_IOPEEN
//End Power switch configuration

//Sound port
#define _SOUND_PORT                GPIOB
#define _SOUND_PIN                 GPIO0
#define _SOUND_RCC_APB1ENR_TIMEN   RCC_APB1ENR_TIM3EN
#define _SOUND_TIM_OC              TIM_OC3
#define _SOUND_TIM                 TIM3
//End Sound port

//Touch pins
#define _TOUCH_PORT                GPIOC
#define _TOUCH_PIN                 GPIO4
#define _TOUCH_IRQ_PIN             GPIO5
#define _TOUCH_RCC_APB2ENR_IOPEN   RCC_APB2ENR_IOPCEN
#define _TOUCH_COORDS_REVERSE      0
//End Touch pins

//PWM Port
#define _PWM_PIN GPIO8
//End PWM Port
#endif //_DEVO12_PORTS_H_
