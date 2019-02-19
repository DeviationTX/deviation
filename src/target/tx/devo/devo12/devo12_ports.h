#ifndef _DEVO12_PORTS_H_
#define _DEVO12_PORTS_H_

#include "hardware.h"
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

//PWM Port
#define _PWM_PIN                   GPIO8
#define _PWM_EXTI                  EXTI8
#define _PWM_TIM_OC                TIM_OC1
#define _PWM_TIM_DIER_DMAEN        TIM_DIER_CC1DE
//End PWM Port
#endif //_DEVO12_PORTS_H_
